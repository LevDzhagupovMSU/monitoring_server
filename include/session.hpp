#pragma once

#include "monitoring/session_handler.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket/stream.hpp>

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <string>


namespace monitoring {


class Session : public std::enable_shared_from_this<Session> {
public:
    using HandshakeHandler = std::function<void(const boost::beast::error_code&)>;
    using DisconnectHandler = std::function<void(Session&, const boost::beast::error_code&)>;
private:
    std::atomic<bool> started_ = false;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    boost::beast::websocket::stream<boost::beast::tcp_stream> w_socket_;
    boost::asio::streambuf buffer_;
    std::deque<std::shared_ptr<const std::string>> messages_deque_;
    DisconnectHandler on_disconnect_;
    monitoring::SessionHandler& ec_handler;

    void read();
    void send_next();
public:
    Session(boost::asio::io_context& io, 
            boost::asio::ip::tcp::socket socket, 
            DisconnectHandler on_disconnect, 
            SessionHandler& ec_handler);

    ~Session();

    void do_handshake(HandshakeHandler on_complete);
    void send(std::shared_ptr<const std::string> message);

    void start();
    void abort_handshake(); // закрыть сессию которая не начала работу
    void stop();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    friend class SessionHandler;
};

}
