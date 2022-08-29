#pragma once

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#ifdef _WIN32
#include <wincrypt.h>
inline void add_windows_root_certs(boost::asio::ssl::context &ctx) {
    HCERTSTORE hStore = CertOpenSystemStore(0, "ROOT");
    if (hStore == NULL) {
        return;
    }

    X509_STORE *store = X509_STORE_new();
    PCCERT_CONTEXT pContext = NULL;
    while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
        // convert from DER to internal format
        X509 *x509 = d2i_X509(NULL, (const unsigned char **)&pContext->pbCertEncoded, pContext->cbCertEncoded);
        if (x509 != NULL) {
            X509_STORE_add_cert(store, x509);
            X509_free(x509);
        }
    }

    CertFreeCertificateContext(pContext);
    CertCloseStore(hStore, 0);

    // attach X509_STORE to boost ssl context
    SSL_CTX_set_cert_store(ctx.native_handle(), store);
}
#endif

namespace Web3 {

class Account;

struct Context {
   private:
    std::shared_ptr<boost::asio::io_service::work> work;
   public:
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
    std::list<std::shared_ptr<Account>> signers;
    inline explicit Context(std::string host, std::string port, unsigned chainId, bool useSsl = false)
        : host(host), port(port), chainId(chainId), ioContext(), resolver(ioContext), endpoints(resolver.resolve(host, port)), sslContext(boost::asio::ssl::context::tlsv12_client), useSsl(useSsl) {
        crypto.group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        const char *pMinusN = "432420386565659656852420866390673177326";
        BN_dec2bn(&crypto.pMinusN, pMinusN);
        if (!crypto.group || !crypto.pMinusN) {
            throw std::runtime_error("Crypto context initialization failed");
        }
        if (useSsl) {
#ifndef _WIN32
            sslContext.set_default_verify_paths();
#else
            add_windows_root_certs(sslContext);
#endif
            sslContext.set_verify_mode(boost::asio::ssl::verify_peer);
        }
        work = std::make_shared<boost::asio::io_service::work>(ioContext);
    }
    inline std::string hostString() const { return host + ":" + port; }

   private:
    inline void runner() {
        try {
            // while (running) {
                try {
                    ioContext.run();
                } catch (std::exception &e) {
                    std::cout << "Error: " << e.what() << std::endl;
                }
            // }
        } catch (...) {
            std::exception_ptr p = std::current_exception();
            std::clog << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;
        }
        std::cout << "Runner thread finished" << std::endl;
    }

   public:
    inline void run() {
        runnerThread = std::thread(&Context::runner, this);
    }
    inline std::string buildRPCJson(const std::string &method, const std::string &params) const {
        return "{\"jsonrpc\":\"2.0\",\"id\":" + std::to_string(chainId) + ",\"method\":\"" + method + "\",\"params\":" + params + "}";
    }
    ~Context() {
        // work.reset();
        if (crypto.group) EC_GROUP_free(crypto.group);
        if (crypto.pMinusN) BN_free(crypto.pMinusN);
        runnerThread.join();
    }
    void setPrimarySigner(std::shared_ptr<Account> account) {
        signers.push_front(account);
    }
};

#ifdef WEB3_IMPLEMENTATION
std::shared_ptr<Context> defaultContext;
#endif

extern std::shared_ptr<Context> defaultContext;
}  // namespace Web3