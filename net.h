#pragma once

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

#include "base.h"
#include "utility.h"

namespace Web3 {
namespace Net {

inline void fail(const boost::beast::error_code &ec, char const *what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

class RPCRequest {
   protected:
    std::shared_ptr<Context> context_;
    std::string json_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;
    boost::beast::flat_buffer buffer_;

    inline void init() {
        req_.version(11);
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

    inline boost::json::value call() {
        if (!context_->useSsl) {
            boost::asio::ip::tcp::socket socket(context_->ioContext);
            boost::asio::connect(socket, context_->endpoints.begin(), context_->endpoints.end());

            boost::beast::http::write(socket, req_);
            boost::beast::http::read(socket, buffer_, res_);
            return boost::json::parse(res_.body());
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
        std::cout << res_.body() << std::endl;
        auto ret = boost::json::parse("{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"0x0000000000000000000000000000000000000000000000000000000000000005\"}");

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

        return ret;
    }
};

// TODO Add ssl to this
template <typename F, bool ParseResult = false>
class AsyncRPC : public std::enable_shared_from_this<AsyncRPC<F, ParseResult>>, RPCRequest {
    F func;
    std::unique_ptr<boost::beast::tcp_stream> stream;
    std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> sslStream;

   public:
    explicit AsyncRPC(std::shared_ptr<Context> context, F &&func, std::string &&rpcJson) : func(std::move(func)) {
        context_ = context;
        json_ = std::move(rpcJson);
        init();
        if (context_->useSsl) {
            sslStream = std::make_unique<boost::beast::ssl_stream<boost::beast::tcp_stream>>(boost::asio::make_strand(context_->ioContext), context_->sslContext);
            if (!SSL_set_tlsext_host_name(sslStream->native_handle(), context_->host.c_str())) {
                boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
                std::cerr << ec.message() << "\n";
                return;
            }
        } else {
            stream = std::make_unique<boost::beast::tcp_stream>(boost::asio::make_strand(context->ioContext));
        }
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
        if (ec) return fail(ec, "connect");

        if (context_->useSsl) {
            boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
            sslStream->async_handshake(boost::asio::ssl::stream_base::client, boost::beast::bind_front_handler(&AsyncRPC::on_handshake, this->shared_from_this()));
        } else {
            stream->expires_after(std::chrono::seconds(30));
            boost::beast::http::async_write(*stream, req_, boost::beast::bind_front_handler(&AsyncRPC::on_write, this->shared_from_this()));
        }
    }

    void on_handshake(boost::beast::error_code ec) {
        if (ec) return fail(ec, "handshake");

        boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
        boost::beast::http::async_write(*sslStream, req_, boost::beast::bind_front_handler(&AsyncRPC::on_write, this->shared_from_this()));
    }

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "write");

        if (context_->useSsl) {
            boost::beast::http::async_read(*sslStream, buffer_, res_, boost::beast::bind_front_handler(&AsyncRPC::on_read, this->shared_from_this()));
        } else {
            boost::beast::http::async_read(*stream, buffer_, res_, boost::beast::bind_front_handler(&AsyncRPC::on_read, this->shared_from_this()));
        }
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) return fail(ec, "read");

        // std::clog << "Body: " << res_.body() << std::endl;
        if constexpr (ParseResult) {
            try {
                func(boost::json::parse(res_.body()));
            } catch (const std::exception &e) {
                std::cerr << "Error running async callback: " << e.what() << std::endl;
            }
        } else {
            func(res_.body());
        }

        if (context_->useSsl) {
            boost::beast::get_lowest_layer(*sslStream).expires_after(std::chrono::seconds(30));
            sslStream->async_shutdown(boost::beast::bind_front_handler(&AsyncRPC::on_shutdown, this->shared_from_this()));
            return;
        }
        stream->socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if (ec && ec != boost::beast::errc::not_connected) return fail(ec, "shutdown");
    }

    void on_shutdown(boost::beast::error_code ec) {
        if (ec) return fail(ec, "shutdown");
    }
};

}  // namespace Net
}  // namespace Web3