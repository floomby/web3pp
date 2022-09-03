#pragma once

#include <boost/multiprecision/cpp_dec_float.hpp>

#include "base.h"
#include "encoder.h"
#include "net.h"
#include "utility.h"
#include "transactionHash.h"
#include "callOptions.h"
#include "units.h"
#include "transaction.h"
#include "gasEstimator.h"

#include "return-type.h"

namespace Web3 {

class Account : public std::enable_shared_from_this<Account> {
    EC_KEY *privateKey = nullptr;
    std::shared_ptr<Context> context;

   public:
    Address address;

    Account() = delete;
    inline explicit Account(const std::string &privateKeyHex, std::shared_ptr<Context> context = defaultContext) : Account(hexToBytes(privateKeyHex), context) {}

    explicit Account(Address address, std::shared_ptr<Context> context = defaultContext) : context(context), address(address) {
        if (!context) {
            throw std::runtime_error("Context must be initalized");
        }
    }

    explicit Account(const char *privateKey, std::shared_ptr<Context> context = defaultContext) : Account(hexToBytes(privateKey), context) {}

    template <typename T>
    explicit Account(const T &privateKey, std::shared_ptr<Context> context = defaultContext) : context(context) {
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
        auto str = context->buildRPCJson("eth_getBalance", "[\"0x" + getAddress() + "\",\"latest\"]");
        auto results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        return fromString(results.get<std::string>("result"));
    }

    bool canSign() const {
        return !!this->privateKey;
    }

    Signature sign(const std::array<unsigned char, 32> &hash, bool eip2 = true, unsigned char kNonce = 0, BN_CTX *ctx = NULL) const {
        if (!canSign()) {
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
        // The probability that a one byte nonce is insufficient to find a valid s is vanishingly small at ~(1/2)^256.
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

    std::vector<unsigned char> signTx(const Transaction &tx, std::shared_ptr<Context> context = nullptr) {
        Encoder::RLPEncodeInput nonce(integralToBytes(tx.nonce));
        Encoder::RLPEncodeInput gasPrice(integralToBytes(tx.gasPrice));
        Encoder::RLPEncodeInput gasLimit(integralToBytes(tx.gasLimit));
        Encoder::RLPEncodeInput to = tx.to ? Encoder::RLPEncodeInput(*tx.to) : Encoder::RLPEncodeInput(std::vector<unsigned char>());
        Encoder::RLPEncodeInput value(integralToBytes(tx.value));
        Encoder::RLPEncodeInput chainId(integralToBytes(context ? context->chainId : defaultContext->chainId));
        Encoder::RLPEncodeInput empty(std::vector<unsigned char>({}));

        Encoder::RLPEncodeInput txrlp({ nonce, gasPrice, gasLimit, to, value, tx.data, chainId, empty, empty });
        auto txrlpEncoded = Encoder::encode(txrlp);
        auto txrlpHash = keccak256(txrlpEncoded);
        auto sig = this->sign(txrlpHash);

        // std::cout << "RLP Encoded: " << toString(txrlpEncoded) << std::endl;
        // std::cout << "RLP Hash: " << toString(txrlpHash) << std::endl;
        // std::cout << "Signature: " << sig << std::endl;

        unpadFront(sig.r);
        unpadFront(sig.s);
        unpadFront(sig.v);

        Encoder::RLPEncodeInput vEnc(sig.v);
        Encoder::RLPEncodeInput rEnc(sig.r);
        Encoder::RLPEncodeInput sEnc(sig.s);

        Encoder::RLPEncodeInput signedTx({ nonce, gasPrice, gasLimit, to, value, tx.data, vEnc, rEnc, sEnc });
        return Encoder::encode(signedTx);
    }

    size_t getTransactionCount() const {
        auto str = context->buildRPCJson("eth_getTransactionCount", "[\"0x" + this->getAddress() + "\", \"latest\"]");
        boost::property_tree::ptree results;
        try {
            results = std::make_shared<Web3::Net::SyncRPC>(context, std::move(str))->call();
        } catch (const std::exception &e) {
            throw std::runtime_error("Unable to get transaction count: " + std::string(e.what()));
        }
        if (results.get_child_optional( "error")) {
            throw std::runtime_error("Unable to get transaction count: " + results.get<std::string>("error.message"));
        }
        return std::stoul(results.get<std::string>("result"), nullptr, 16);
    }

    template <typename F>
    std::shared_ptr<std::promise<return_type_t<F>>> getTransactionCount_async(F &&func) const {
        auto str = context->buildRPCJson("eth_getTransactionCount", "[\"0x" + this->getAddress() + "\", \"latest\"]");
        auto promise = std::make_shared<std::promise<return_type_t<F>>>();
        auto handler = [func = std::move(func), promise](boost::property_tree::ptree &&results) {
            if constexpr (std::is_same_v<return_type_t<F>, void>) {
                func(std::stoul(results.get<std::string>("result"), nullptr, 16));
                promise->set_value();
            } else {
                promise->set_value(func(std::stoul(results.get<std::string>("result"), nullptr, 16)));
            }
        };
        std::make_shared<Web3::Net::AsyncRPC<decltype(handler), true>>(context, std::move(handler), std::move(str))->call();
        return promise;
    }

    Address deployedContract(size_t nonce) const {
        auto tmp = keccak256_v(Encoder::RLPEncode(this->address, nonce));
        tmp.erase(tmp.begin(), tmp.begin() + 12);
        return Address(tmp);
    }

    // This feels wrong being here and in the contract abstract class (this is what I get for allowing send to be used for contract calls)
    const boost::multiprecision::cpp_dec_float_50 gasMult = boost::multiprecision::cpp_dec_float_50(1.2);

    // FIXME This almost works as an abstraction for the code that the generator emits for async stuff
    template <typename F> std::shared_ptr<std::promise<return_type_t<F>>> send_async(const Address &to, F &&func, const CallOptions &options = {}, const std::vector<unsigned char> &data = {}) {
        auto promise = std::make_shared<std::promise<return_type_t<F>>>();

        std::shared_ptr<Account> signer;
        if (options.account) {
            signer = options.account;
        } else {
            signer = shared_from_this();
        }

        if (!signer->canSign()) throw std::runtime_error("Account is unable to sign transactions");
        if (signer->address.isZero()) throw std::runtime_error("Unable to send transaction from zero address");

        boost::multiprecision::uint256_t txValue;
        if (options.value) {
            txValue = *options.value;
        } else {
            txValue = 0;
        }

        boost::multiprecision::uint256_t gasPrice;
        if (options.gasPrice) {
            gasPrice = *options.gasPrice;
        } else {
            gasPrice = Units::gwei(10);
        }
        std::optional<boost::multiprecision::uint256_t> gasLimit = std::nullopt;
        if (options.gasLimit) {
            gasLimit = *options.gasLimit;
        }
        auto rawSender = [to, context = this->context, signer, txValue, gasPrice, data, func = std::move(func), promise](size_t nonce, const boost::multiprecision::uint256_t &gasLimit) {
            Transaction tx{nonce, gasPrice, gasLimit, to, txValue, data};
            auto signedTx = signer->signTx(tx);
            auto str = context->buildRPCJson("eth_sendRawTransaction", "[\"0x" + toString(signedTx) + "\"]");
            std::make_shared<Net::AsyncRPC<decltype(func), true>>(context, std::move(func), std::move(str), promise)->call();
        };
        auto gasGetter = [to, context = this->context, gasMult = this->gasMult, signer, txValue, gasPrice, gasLimit, data, rawSender](size_t nonce) {
            if (gasLimit || data.empty()) {
                if (!gasLimit) {
                    rawSender(nonce, 21000);
                } else {
                    rawSender(nonce, *gasLimit);
                }
            } else {
                auto handler = [rawSender, nonce, gasMult](const std::string &gasEstimation) {
                    boost::multiprecision::cpp_dec_float_50 gasF = fromString(gasEstimation).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
                    rawSender(nonce, gasF.convert_to<boost::multiprecision::uint256_t>());
                };
                GasEstimator::estimateGas_async(std::move(handler), signer->address, toString(txValue).c_str(), data, to, context);
            }
        };
        if (options.nonce) {
            gasGetter(*options.nonce);
        } else {
            signer->getTransactionCount_async(std::move(gasGetter));
        }
        return promise;
    }

    auto send(const Address &to, const CallOptions &options = {}, const std::vector<unsigned char> &data = {}) {
        std::shared_ptr<Account> signer;
        if (options.account) {
            signer = options.account;
        } else {
            signer = shared_from_this();
        }

        if (!signer->canSign()) throw std::runtime_error("Account is unable to sign transactions");
        if (signer->address.isZero()) throw std::runtime_error("Unable to send transaction from zero address");
    
        boost::multiprecision::uint256_t gasLimit;
        boost::multiprecision::uint256_t txValue;
        if (options.value) {
            txValue = *options.value;
        } else {
            txValue = 0;
        }
        
        size_t nonce;
        if (options.nonce) {
            nonce = *options.nonce;
        } else {
            nonce = signer->getTransactionCount();
        }
        if (options.gasLimit) {
            gasLimit = *options.gasLimit;
        } else {
            if (data.empty()) {
                gasLimit = 21000;
            } else {
                auto gas = GasEstimator::estimateGas(signer->address, toString(txValue).c_str(), data, to, this->context);
                boost::multiprecision::cpp_dec_float_50 gasF = fromString(gas).convert_to<boost::multiprecision::cpp_dec_float_50>() * gasMult;
                gasLimit = gasF.convert_to<boost::multiprecision::uint256_t>();
            }
        }
        boost::multiprecision::uint256_t gasPrice;
        if (options.gasPrice) {
            gasPrice = *options.gasPrice;
        } else {
            gasPrice = Units::gwei(10);
        }
        Transaction tx{nonce, gasPrice, gasLimit, to, txValue, data};
        auto signedTx = signer->signTx(tx);
        auto h = Ethereum::sendRawTransaction(toString(signedTx));
        return h.getReceipt();
    }
};

}  // namespace Web3

namespace std {
inline std::ostream &operator<<(std::ostream &os, const Web3::Signature &sig) {
    os << "v: " << Web3::toString(sig.v) << " r: " << Web3::toString(sig.r) << " s: " << Web3::toString(sig.s);
    return os;
}
}
