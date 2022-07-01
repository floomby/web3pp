#pragma once

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace Web3 {

struct Context {
    std::string host, port;
    unsigned chainId;
    boost::asio::io_context ioc;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> endpoints;
    struct {
        EC_GROUP *group = NULL;
        BIGNUM *pMinusN = NULL;
    } crypto;
    inline explicit Context(std::string host, std::string port, unsigned chainId) : host(host), port(port), chainId(chainId), ioc(), resolver(ioc), endpoints(resolver.resolve(host, port)) {
        crypto.group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        const char *pMinusN = "432420386565659656852420866390673177326";
        BN_dec2bn(&crypto.pMinusN, pMinusN);
        if (!crypto.group || !crypto.pMinusN) {
            throw std::runtime_error("Crypto context initialization failed");
        }
    }
    inline std::string hostString() const { return host + ":" + port; }
    inline void run() { ioc.run(); }
    inline std::string buildRPCJson(const std::string &method, const std::string &params) const {
        return "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(chainId) + ",\"method\":\"" + method + "\",\"params\":" + params + "}";
    }
};

#ifdef WEB3_IMPLEMENTATION
std::shared_ptr<Context> defaultContext;
#endif

extern std::shared_ptr<Context> defaultContext;
}