#pragma once

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <memory>
#include <string>

namespace Web3 {

struct Context {
    std::string host, port;
    unsigned chainId;
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ip::basic_resolver_results<boost::asio::ip::tcp> endpoints;
    boost::asio::ssl::context sslContext;
    bool useSsl;
    std::atomic<bool> running = false;
    struct {
        EC_GROUP *group = NULL;
        BIGNUM *pMinusN = NULL;
    } crypto;
    std::thread runnerThread;
    inline explicit Context(std::string host, std::string port, unsigned chainId, bool useSsl = false)
        : host(host), port(port), chainId(chainId), ioContext(), resolver(ioContext), endpoints(resolver.resolve(host, port)), sslContext(boost::asio::ssl::context::sslv23), useSsl(useSsl) {
        crypto.group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        const char *pMinusN = "432420386565659656852420866390673177326";
        BN_dec2bn(&crypto.pMinusN, pMinusN);
        if (!crypto.group || !crypto.pMinusN) {
            throw std::runtime_error("Crypto context initialization failed");
        }
        if (useSsl) {
            sslContext.set_default_verify_paths();
            sslContext.set_verify_mode(boost::asio::ssl::verify_none);
        }
    }
    inline std::string hostString() const { return host + ":" + port; }

   private:
    inline void runner() {
        while (running) {
            try {
                ioContext.run();
            } catch (...) {
                std::exception_ptr p = std::current_exception();
                std::clog <<(p ? p.__cxa_exception_type()->name() : "null") << std::endl;
            }
        }
    }

   public:
    inline void run() {
        running = true;
        auto work = std::make_shared<boost::asio::io_service::work>(ioContext);
        runnerThread = std::thread(Context::runner, this);
    }
    inline std::string buildRPCJson(const std::string &method, const std::string &params) const {
        return "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(chainId) + ",\"method\":\"" + method + "\",\"params\":" + params + "}";
    }
    ~Context() {
        running = false;
        if (crypto.group) EC_GROUP_free(crypto.group);
        if (crypto.pMinusN) BN_free(crypto.pMinusN);
        runnerThread.join();
    }
};

#ifdef WEB3_IMPLEMENTATION
std::shared_ptr<Context> defaultContext;
#endif

extern std::shared_ptr<Context> defaultContext;
}  // namespace Web3