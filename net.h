#pragma once

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include "base.h"
#include "utility.h"
#include "return-type.h"

namespace Web3 {

template <class T> struct is_promise_shared_ptr : std::false_type {};
template <class T> struct is_promise_shared_ptr<std::shared_ptr<std::promise<T>>> : std::true_type {};

namespace Net {

class RPCRequest {
   protected:
    std::shared_ptr<Context> context_;
    std::string json_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;
    boost::beast::flat_buffer buffer_;

    inline void init() {
        req_.version(12);
        req_.target("/");
        req_.method(boost::beast::http::verb::post);
        req_.set(boost::beast::http::field::host, context_->hostString());
        req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req_.set(boost::beast::http::field::accept, "*/*");
        req_.set(boost::beast::http::field::content_type, "application/json");
        req_.body() = json_;
        req_.prepare_payload();
    }
   public:
    RPCRequest() = default;
    inline RPCRequest(std::shared_ptr<Context> context, std::string &&rpcJson) : context_(context), json_(rpcJson) {
        init();
    }
    // Move technically doesn't need to be deleted, but for right now I want to have it error if I try and move it on accident
    RPCRequest(RPCRequest &&other) = delete;
    RPCRequest(const RPCRequest &other) = delete;
};

class SyncRPC : public std::enable_shared_from_this<SyncRPC>, RPCRequest {
   public:
    inline explicit SyncRPC(std::shared_ptr<Context> context, std::string &&rpcJson) : RPCRequest(context, std::move(rpcJson)) {}

    inline boost::property_tree::ptree call() {
        if (!context_->useSsl) {
            boost::asio::ip::tcp::socket socket(context_->ioContext);
            boost::asio::connect(socket, context_->endpoints.begin(), context_->endpoints.end());

            boost::beast::http::write(socket, req_);
            boost::beast::http::read(socket, buffer_, res_);
            boost::iostreams::stream<boost::iostreams::array_source> stream(res_.body().c_str(), res_.body().size());
            boost::property_tree::ptree results;
            boost::property_tree::read_json(stream, results);
            return results;
        }

        auto stream = std::make_shared<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(context_->ioContext, context_->sslContext);

        if (!SSL_set_tlsext_host_name(stream->native_handle(), context_->host.c_str())) {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            throw boost::beast::system_error{ec};
        }

        boost::beast::get_lowest_layer(*stream).connect(*context_->endpoints.begin());

        // Perform the SSL handshake
        stream->handshake(boost::asio::ssl::stream_base::client);

        boost::beast::http::write(*stream, req_);
        boost::beast::http::read(*stream, buffer_, res_);

        boost::iostreams::stream<boost::iostreams::array_source> stream2(res_.body().c_str(), res_.body().size());
        boost::property_tree::ptree results;
        boost::property_tree::read_json(stream2, results);

        // Idk the right way to do this?? Need to pull wireshark out to figure this out I think
        stream->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        // asynchronously and gracefully shutdown the connection so we can return now even if the server is slow to close the stream
        // stream->lowest_layer().cancel();
        // stream->async_shutdown(
        //     [stream](const boost::system::error_code &ec) {
        //         if (ec) {
        //             std::cerr << "Error shutting down SSL stream: " << ec.message() << std::endl;
        //         }
        // });

        return results;
    }
};

template <typename P, typename F, bool ParseResult = false>
class AsyncRPC : public std::enable_shared_from_this<AsyncRPC<P, F, ParseResult>>, RPCRequest {
    P topLevelPromise;
    F func;
    std::unique_ptr<boost::beast::tcp_stream> stream;
    std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> sslStream;
    std::shared_ptr<std::promise<return_type_t<F>>> promise;

   public:
    explicit AsyncRPC(P top, std::shared_ptr<Context> context, F &&func, std::string &&rpcJson) : topLevelPromise(top), func(std::move(func)) {
        static_assert(is_promise_shared_ptr<P>::value, "Must pass in a shared pointer to top level promise for exception handling");
        context_ = context;
        json_ = std::move(rpcJson);
        init();
        if (context_->useSsl) {
            sslStream = std::make_unique<boost::beast::ssl_stream<boost::beast::tcp_stream>>(boost::asio::make_strand(context_->ioContext), context_->sslContext);
            if (!SSL_set_tlsext_host_name(sslStream->native_handle(), context_->host.c_str())) {
                boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
#ifdef NETWORKING_DEBUG
                std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
                if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
            }
        } else {
            stream = std::make_unique<boost::beast::tcp_stream>(boost::asio::make_strand(context->ioContext));
        }
    }

    explicit AsyncRPC(P top, std::shared_ptr<Context> context, F &&func, std::string &&rpcJson, std::shared_ptr<std::promise<return_type_t<F>>> promise) : AsyncRPC(top, context, std::move(func), std::move(rpcJson)) {
        static_assert(is_promise_shared_ptr<P>::value, "Must pass in a shared pointer to top level promise for exception handling");
        this->promise = promise;
    }

    void call() {
        if (context_->useSsl) {
            boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
            boost::beast::get_lowest_layer(*sslStream).async_connect(context_->endpoints, boost::beast::bind_front_handler(&AsyncRPC::on_connect, this->shared_from_this()));
        } else {
            stream->expires_after(std::chrono::seconds(30));
            stream->async_connect(context_->endpoints, boost::beast::bind_front_handler(&AsyncRPC::on_connect, this->shared_from_this()));
        }
    }

    void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
        if (ec) {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
            if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
            return;
        }

        if (context_->useSsl) {
            boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
            sslStream->async_handshake(boost::asio::ssl::stream_base::client, boost::beast::bind_front_handler(&AsyncRPC::on_handshake, this->shared_from_this()));
        } else {
            stream->expires_after(std::chrono::seconds(30));
            boost::beast::http::async_write(*stream, req_, boost::beast::bind_front_handler(&AsyncRPC::on_write, this->shared_from_this()));
        }
    }

    void on_handshake(boost::beast::error_code ec) {
        if (ec) {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
            if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
            return;
        }

        boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
        boost::beast::http::async_write(*sslStream, req_, boost::beast::bind_front_handler(&AsyncRPC::on_write, this->shared_from_this()));
    }

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
            if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
            return;
        }

        if (context_->useSsl) {
            boost::beast::http::async_read(*sslStream, buffer_, res_, boost::beast::bind_front_handler(&AsyncRPC::on_read, this->shared_from_this()));
        } else {
            boost::beast::http::async_read(*stream, buffer_, res_, boost::beast::bind_front_handler(&AsyncRPC::on_read, this->shared_from_this()));
        }
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
            if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
            return;
        };

        try {
            if constexpr (ParseResult) {
                boost::iostreams::stream<boost::iostreams::array_source> is(res_.body().data(), res_.body().size());
                boost::property_tree::ptree pt;
                boost::property_tree::read_json(is, pt);
                if constexpr (std::is_same_v<return_type_t<F>, void>) {
                    func(std::move(pt));
                    if (promise) promise->set_value();
                } else {
                    if (promise) promise->set_value(func(std::move(pt)));
                    else func(std::move(pt));
                }
            } else {
                if constexpr (std::is_same_v<return_type_t<F>, void>) {
                    func(res_.body());
                    if (promise) promise->set_value();
                } else {
                    if (promise) promise->set_value(func(res_.body()));
                    else func(res_.body());
                }
            }
        } catch (...) {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
            if (topLevelPromise) topLevelPromise->set_exception(std::current_exception());
        }

        if (context_->useSsl) {
            boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
            sslStream->async_shutdown(boost::beast::bind_front_handler(&AsyncRPC::on_shutdown, this->shared_from_this()));
            return;
        }
        stream->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected) {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
            if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
            return;
        };
    }

    void on_shutdown(boost::beast::error_code ec) {
        if (ec) {
            // Deal with non-compliant SSL implementations (There is a good reason so many are non-compliant)
            if (ec == boost::asio::ssl::error::stream_truncated) {
                ec = {};
            } else {
#ifdef NETWORKING_DEBUG
            std::clog << "Error at " << __FILE__ << ":" << __LINE__ << "  in  " << __FUNCTION__ << "  " << ec.message() << std::endl;
#endif
                if (topLevelPromise) topLevelPromise->set_exception(std::make_exception_ptr(boost::beast::system_error{ec}));
                return;
            }
        }
    }
};

}  // namespace Net
}  // namespace Web3