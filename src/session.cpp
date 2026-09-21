#include "session.hpp"
#include "monitoring/session_event.hpp"
#include "monitoring/session_handler.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream_base.hpp>

#include <iostream>
#include <utility>


monitoring::Session::Session(boost::asio::io_context& io, 
                            boost::asio::ip::tcp::socket socket, 
                            DisconnectHandler on_disconnect, 
                            SessionHandler& ec_handler) : 
                            strand_(boost::asio::make_strand(io)), w_socket_(std::move(socket)), on_disconnect_(on_disconnect), ec_handler(ec_handler) {};

void monitoring::Session::do_handshake(HandshakeHandler on_complete){
    boost::asio::post(strand_, [self = shared_from_this(), on_complete = std::move(on_complete)](){
        self->w_socket_.set_option(boost::beast::websocket::stream_base::timeout::suggested
                            (boost::beast::role_type::server));

        self->w_socket_.async_accept(boost::asio::bind_executor(self->strand_,
                            [self, on_complete = std::move(on_complete)](const boost::beast::error_code& ec){
            if(!ec)
                self->w_socket_.text(true);
            on_complete(ec);
        }));
    });

}

void monitoring::Session::start(){
    if(started_){
        return; // надо логировать
    }
    started_ = true;

    read();
}

void monitoring::Session::read(){
    auto self = shared_from_this();
    w_socket_.async_read(self->buffer_, boost::asio::bind_executor(self->strand_, [self](const boost::beast::error_code& ec, size_t){
        if(!self->started_)
            return;

        if(ec){
            auto type = ec == boost::beast::websocket::error::closed
              ? SessionEventType::disconnected
              : SessionEventType::read_error;


            self->ec_handler.on_event(self, {type, ec});
            return;
        }          
        self->buffer_.consume(self->buffer_.size());
        self->read();
    }));
}

void monitoring::Session::send(std::shared_ptr<const std::string> message){
    if(!started_)
        return;

    bool on_sending = !messages_deque_.empty();

    messages_deque_.push_back(std::move(message));
    if(messages_deque_.size() == 3){
        std::swap(messages_deque_[1], messages_deque_[2]);
        messages_deque_.pop_back();
        std::cerr << "Warning: message queue is full, dropping the oldest message" << std::endl;
    }

    if(!on_sending){
        send_next();
    }
}

void monitoring::Session::send_next(){
    auto cur_message = messages_deque_.front();
    w_socket_.async_write(boost::asio::buffer(*cur_message),[self = shared_from_this()](const boost::beast::error_code& ec, size_t){
        if(ec){
            self->ec_handler.on_event(self, {monitoring::SessionEventType::write_error, ec});

            return;
        }
        self->messages_deque_.pop_front();
        if(!self->messages_deque_.empty()){
            self->send_next();
        }
    });
}

void monitoring::Session::fail(const boost::beast::error_code& ec){
    if(!started_.exchange(false))
        return;

    boost::beast::error_code ignored;
    boost::beast::get_lowest_layer(w_socket_).socket().close(ignored);

    on_disconnect_(*this, ec);
}

void monitoring::Session::stop(){
    if(!started_.exchange(false))
        return;

    on_disconnect_(*this, {});
    
    boost::beast::error_code ec;
    w_socket_.close({boost::beast::websocket::close_code::normal, "Shutting down"}, ec);
    if(ec){
        ec_handler.on_event(shared_from_this(), {monitoring::SessionEventType::close_error, ec});
    }
}


monitoring::Session::~Session() = default;
