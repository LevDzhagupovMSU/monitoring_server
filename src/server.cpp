#include <boost/asio/strand.hpp>
#include <boost/beast/core/error.hpp>
#include <iostream>
#include <memory>
#include <thread>
#include <utility>

#include "server.hpp"
#include "monitoring/session_event.hpp"
#include "session.hpp"


void monitoring::Server::start(){
    if(running_){
        std::cout << "WARNING: Server already started" << std::endl;
        return;
    }

    running_ = true;
    start_accept();
    metric_service_.start();

    server_th = std::thread([this](){
        io_.run();
    });
}

monitoring::Server::~Server(){
    stop();
}

void monitoring::Server::stop(){
    if(!running_.exchange(false)){
        return;
    }

    boost::beast::error_code ec;
    acceptor_.close(ec);
    metric_service_.stop();

    auto sessions = session_.detach_all();
    for(auto& session : sessions){
        session->stop();
    }

    io_.stop();

    if(server_th.joinable()){
        server_th.join();
    }
}

void monitoring::Server::start_accept(){
    acceptor_.async_accept([this](const boost::system::error_code& ec, 
                        boost::asio::ip::tcp::socket socket){
        if (ec == boost::asio::error::operation_aborted) {
            return; 
        }
        if (ec) {
            std::cerr << "accept error: " << ec.message() << '\n';
            return;
        }

        create_session(io_,std::move(socket));
        start_accept();
    });
}

void monitoring::Server::create_session(boost::asio::io_context& io, boost::asio::ip::tcp::socket socket){
    auto session = std::make_shared<monitoring::Session>(io, std::move(socket), [this](Session& session, const boost::beast::error_code& ec){
        session_.leave(session);
        if (ec) {
            std::cerr << "Client disconnected with error: " << ec.message() << '\n';
        } else {
            std::cerr << "Client disconnected: server shutdown\n";
        }

    }, ec_handler_);

    session->do_handshake([this, session](const boost::beast::error_code& ec){
        if(!ec){
            session_.join(session);
            session->start();
            
            std::cout << "New client connected"<< std::endl;

            return;
        }
        ec_handler_.on_event(session, {monitoring::SessionEventType::handshake_error, ec});
    });
}