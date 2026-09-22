#include "session.hpp"
#include "monitoring/session_event.hpp"
#include "monitoring/session_handler.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <iostream>
#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/role.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream_base.hpp>

#include <iostream>
#include <thread>
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
    boost::asio::post(strand_, [message, self = shared_from_this()](){
        if(!self->started_)
            return;

        bool on_sending = !self->messages_deque_.empty();
        
        self->messages_deque_.push_back(std::move(message));
        if(self->messages_deque_.size() == 3){
            std::swap(self->messages_deque_[1], self->messages_deque_[2]);
            self->messages_deque_.pop_back();
            std::cerr << "Warning: message queue is full, dropping the oldest message" << std::endl;
        }

        if(!on_sending)
            self->send_next();
    });
}

void monitoring::Session::send_next(){
    boost::asio::dispatch(strand_, [self = shared_from_this()](){
        auto cur_message = self->messages_deque_.front();

        self->w_socket_.async_write(boost::asio::buffer(*cur_message),
                              boost::asio::bind_executor(self->strand_, [self, cur_message](const boost::beast::error_code& ec, size_t){
            if(ec){
                self->ec_handler.on_event(self, {monitoring::SessionEventType::write_error, ec});

                return;
            }

            self->messages_deque_.pop_front();
            if(!self->messages_deque_.empty()){
                self->send_next();
            }
        }));
    });
}

void monitoring::Session::abort_handshake(){
    boost::asio::dispatch(
        strand_,
        [self = shared_from_this()] {
            if (self->started_) {
                return;
            }

            boost::beast::error_code ec;
            boost::beast::get_lowest_layer(
                self->w_socket_).socket().close(ec);

            self->on_disconnect_(*self, ec);
        });
}

void monitoring::Session::stop(){
    boost::asio::post(strand_, [self = shared_from_this()](){
        if(!self->started_.exchange(false))
            return;

        self->on_disconnect_(*self, {});

        self->w_socket_.async_close({boost::beast::websocket::close_code::normal, "Shutting down"},
            boost::asio::bind_executor(self->strand_, [self](const boost::beast::error_code& ec){
                if(ec && ec != boost::asio::error::operation_aborted)
                    self->ec_handler.on_event(self,
                                {monitoring::SessionEventType::close_error,ec});
            }));
    });
}


monitoring::Session::~Session() = default;
