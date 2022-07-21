#pragma once

#include "base.h"
#include "encoder.h"
#include "net.h"
#include "utility.h"
#include "transactionHash.h"

namespace Web3 {

struct Signature {
    std::vector<unsigned char> v;
    std::vector<unsigned char> r;
    std::vector<unsigned char> s;
};

class Account {
    EC_KEY *privateKey = nullptr;
    std::shared_ptr<Context> context;

   public:
    bool canSign = false;
    Address address;

    Account() = delete;
    inline explicit Account(const std::string &privateKeyHex, std::shared_ptr<Context> context = defaultContext) : Account(hexToBytes(privateKeyHex), context) {}

    explicit Account(Address address, std::shared_ptr<Context> context = defaultContext) : context(context), address(address) {
        if (!context) {
            throw std::runtime_error("Context must be initalized");
        }
    }

    template <typename T>
    explicit Account(const T &privateKey, std::shared_ptr<Context> context = defaultContext) : context(context), canSign(true) {
        if (!context) {
            throw std::runtime_error("Context must be initalized");
        }

        if (privateKey.size() != 32) {
            throw std::runtime_error("privateKey must be 32 bytes");
        }

        this->privateKey = EC_KEY_new();
        if (!this->privateKey) {
            throw std::runtime_error("Unable to create EC key");
        }

        if (!EC_KEY_set_group(this->privateKey, context->crypto.group)) {
            EC_KEY_free(this->privateKey);
            throw std::runtime_error("Unable to set EC group");
        }

        BIGNUM *tmpKey = BN_bin2bn(privateKey.data(), privateKey.size(), NULL);

        // std::cout << "Private key: ";
        // BN_print_fp(stdout, tmpKey);
        // std::cout << std::endl;

        if (!EC_KEY_set_private_key(this->privateKey, tmpKey)) {
            EC_KEY_free(this->privateKey);
            BN_free(tmpKey);
            throw std::runtime_error("Unable to set private key");
        }

        EC_POINT *pubKey = EC_POINT_new(context->crypto.group);
        if (!pubKey) {
            EC_KEY_free(this->privateKey);
            BN_free(tmpKey);
            throw std::runtime_error("Unable to create public key");
        }

        if (!EC_POINT_mul(context->crypto.group, pubKey, tmpKey, NULL, NULL, NULL)) {
            EC_KEY_free(this->privateKey);
            EC_POINT_free(pubKey);
            BN_free(tmpKey);
            throw std::runtime_error("Unable to multiply public key");
        }
        BN_free(tmpKey);

        if (!EC_KEY_set_public_key(this->privateKey, pubKey)) {
            EC_KEY_free(this->privateKey);
            EC_POINT_free(pubKey);
            throw std::runtime_error("Unable to set public key");
        }

        if (!EC_KEY_check_key(this->privateKey)) {
            EC_KEY_free(this->privateKey);
            EC_POINT_free(pubKey);
            throw std::runtime_error("Invalid private key");
        }

        unsigned char *buf = NULL;
        auto size = EC_POINT_point2buf(context->crypto.group, pubKey, POINT_CONVERSION_UNCOMPRESSED, &buf, NULL);
        std::array<unsigned char, 64> resultBuf;
        assert(size == 65);
        memcpy(resultBuf.data(), buf + 1, 64);
        // std::cout << "Public key: " << toString(resultBuf) << std::endl;
        auto kec = keccak256(resultBuf);
        std::copy(kec.begin() + 12, kec.end(), address.bytes.begin());

        EC_POINT_free(pubKey);
    }
    ~Account() {
        if (privateKey) {
            EC_KEY_free(privateKey);
        }
    }
    std::string getAddress() const {
        return toString(address.bytes);
    }
    boost::multiprecision::uint256_t getBalance() const {
        // std::string str = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_getBalance\",\"params\":[\"0x" + getAddress() + "\",\"latest\"],\"id\":5777}";
        auto str = context->buildRPCJson("eth_getBalance", "[\"0x" + getAddress() + "\",\"latest\"]");
        auto results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        return fromString(value_to<std::string>(results.at("result")));
    }
    // TODO Not sure all the error handling is correct (I am not experienced with openssl)
    Signature sign(const std::array<unsigned char, 32> &hash, bool eip2 = true, unsigned char kNonce = 0, BN_CTX *ctx = NULL) const {
        if (!canSign) {
            throw std::runtime_error("Account is not able to sign");
        }

        // The low level openssl ecdsa signature api is not well suited for key recovery (the api is deprecated anyways for "public" use).
        // Note that this uses a variation on rfc6979 deterministic signing that is compatible with eip 2 singing

        if (!ctx) {
            ctx = BN_CTX_new();
        }
        if (!ctx) {
            throw std::runtime_error("Unable to create BN_CTX");
        }

        std::vector<unsigned char> toHash;
        std::copy(hash.begin(), hash.end(), std::back_inserter(toHash));
        toHash.resize(hash.size() + 32);
        auto priv = EC_KEY_get0_private_key(privateKey);
        if (!priv) {
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to get private key");
        }
        BN_bn2binpad(priv, toHash.data() + hash.size(), 32);
        // The probability that a one byte nonce is insufficient to find a valid s is vanishingly small at (1/2)^256.
        toHash.push_back(kNonce);
        auto kBytes = keccak256(toHash);
        auto k = BN_bin2bn(kBytes.data(), kBytes.size(), NULL);
        if (!k) {
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to create BN k");
        }
        auto h = BN_bin2bn(hash.data(), hash.size(), NULL);
        if (!h) {
            BN_free(k);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to create BN h");
        }

        auto order = EC_GROUP_get0_order(context->crypto.group);
        if (!order) {
            BN_free(h);
            BN_free(k);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to get group order");
        };

        auto R = EC_POINT_new(context->crypto.group);
        if (!R) {
            BN_free(h);
            BN_free(k);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to create R");
        }

        if (!EC_POINT_mul(context->crypto.group, R, k, NULL, NULL, ctx)) {
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to compute R");
        }

        auto k_1 = BN_mod_inverse(NULL, k, order, ctx);
        if (!k_1) {
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to compute k inverse");
        }

        auto x = BN_new();
        auto y = BN_new();
        EC_POINT_get_affine_coordinates(context->crypto.group, R, x, y, ctx);
        if (!x || !y) {
            if (x) {
                BN_free(x);
            }
            if (y) {
                BN_free(y);
            }
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to get point coordinates");
        }

        // Avoid having to set the second bit of the recovery id as per ethereum spec
        if (BN_cmp(x, context->crypto.pMinusN) == -1) {
            BN_free(x);
            BN_free(y);
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            return sign(hash, eip2, kNonce + 1, ctx);
        }

        auto parity = BN_is_odd(y) ? 1 : 0;

        // std::cout << "Parity: " << parity << std::endl;
        // std::cout << "priv: ";
        // BN_print_fp(stdout, priv);
        // std::cout << std::endl;
        // std::cout << "order: ";
        // BN_print_fp(stdout, order);
        // std::cout << std::endl;
        // std::cout << "x: ";
        // BN_print_fp(stdout, x);
        // std::cout << std::endl;
        // std::cout << "y: ";
        // BN_print_fp(stdout, y);
        // std::cout << std::endl;
        // std::cout << "h: ";
        // BN_print_fp(stdout, h);
        // std::cout << std::endl;

        if (!BN_mod_mul(y, x, priv, order, ctx)) {
            BN_free(x);
            BN_free(y);
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to compute tmp");
        }

        if (!BN_add(y, h, y)) {
            BN_free(x);
            BN_free(y);
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to compute tmp");
        }

        if (!BN_mod_mul(y, y, k_1, order, ctx)) {
            BN_free(x);
            BN_free(y);
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            throw std::runtime_error("Unable to compute tmp");
        }

        // TODO reject if s is zero
        if (BN_is_zero(y)) {
            BN_free(x);
            BN_free(y);
            EC_POINT_free(R);
            BN_free(h);
            BN_free(k);
            return this->sign(hash, eip2, kNonce + 1, ctx);
        }

        // EIP 2 check that s < n/2 + 1
        if (eip2) {
            if (!BN_rshift1(k, order)) {
                BN_free(x);
                BN_free(y);
                EC_POINT_free(R);
                BN_free(h);
                BN_free(k);
                BN_CTX_end(ctx);
                BN_CTX_free(ctx);
                throw std::runtime_error("Unable to half order");
            }
            if (BN_cmp(y, k) == 1) {
                BN_free(x);
                BN_free(y);
                EC_POINT_free(R);
                BN_free(h);
                BN_free(k);
                return this->sign(hash, eip2, kNonce + 1);
            }
        }

        Signature ret;
        unsigned char buf[32];

        BN_bn2binpad(x, buf, sizeof(buf));
        std::copy(buf, buf + 32, std::back_inserter(ret.r));

        BN_bn2binpad(y, buf, sizeof(buf));
        std::copy(buf, buf + 32, std::back_inserter(ret.s));

        // printf("r: ");
        // BN_print_fp(stdout, x);
        // printf("\n");
        // printf("s: ");
        // BN_print_fp(stdout, y);
        // printf("\n");

        // Legacy with EIP 155 protection
        auto v = integralToBytes(context->chainId * 2 + 35 + parity);
        std::copy(v.begin(), v.end(), std::back_inserter(ret.v));

        BN_free(x);
        BN_free(y);
        EC_POINT_free(R);
        BN_free(h);
        BN_free(k);
        BN_CTX_free(ctx);

        return ret;
    }
    
    size_t getTransactionCount() const {
        auto str = context->buildRPCJson("eth_getTransactionCount", "[\"0x" + this->getAddress() + "\", \"latest\"]");
        boost::json::value results;
        try {
            results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to get transaction count: " + std::string(e.what()));
        }
        if (results.as_object().contains("error")) {
            throw std::runtime_error("Unable to get transaction count: " + value_to<std::string>(results.at("error").at("message")));
        }
        return std::stoul(value_to<std::string>(results.at("result")), nullptr, 16);
    }

    template <typename F>
    void getTransactionCount_async(F &&func) const {
        auto str = context->buildRPCJson("eth_getTransactionCount", "[\"0x" + this->getAddress() + "\", \"latest\"]");
        auto handler = [func = std::move(func)](boost::json::value &&results) {
            func(std::stoul(value_to<std::string>(results.at("result")), nullptr, 16));
        };
        std::make_shared<Web3::Net::AsyncRPC<decltype(handler), true>>(context, std::move(handler), std::move(str))->call();
    }

    Address deployedContract(size_t nonce) const {
        auto tmp = keccak256_v(Encoder::RLPEncode(this->address, nonce));
        tmp.erase(tmp.begin(), tmp.begin() + 12);
        return Address(tmp);
    }
};

}  // namespace Web3

namespace std {
inline std::ostream &operator<<(std::ostream &os, const Web3::Signature &sig) {
    os << "v: " << Web3::toString(sig.v) << " r: " << Web3::toString(sig.r) << " s: " << Web3::toString(sig.s);
    return os;
}
}
