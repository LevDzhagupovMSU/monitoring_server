#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "session.hpp"
#include "monitoring/session_handler.hpp"
#include "monitoring/metrics_service.hpp"
#include "monitoring/shared_state.hpp"

namespace monitoring {
    

class Server{
    std::atomic<bool> running_ = false;

    boost::asio::io_context io_; 
    boost::asio::ip::tcp::acceptor acceptor_;

    std::string address_;
    uint16_t port_;

    SharedState session_;
    MetricsService metric_service_;
    SessionHandler ec_handler_;

    std::vector<std::thread> server_th;

    void start_accept();

    void create_session(boost::asio::io_context& io, boost::asio::ip::tcp::socket socket);
    void delete_session(Session& session);

public:
    Server(const std::string& address, uint16_t port)
        : io_(), acceptor_(io_,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address),
            port)), address_(address), port_(port),
            metric_service_(io_, session_) {};
    
    ~Server();

    void start();
    void stop();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = delete;
    Server& operator=(Server&&) = delete;

};


}
