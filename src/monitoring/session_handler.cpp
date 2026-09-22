#include <memory>
#include <iostream>

#include "server.hpp"
#include "monitoring/session_handler.hpp"
#include "monitoring/session_event.hpp"

void monitoring::SessionHandler::on_event(std::shared_ptr<Session> session, const SessionEvent& event){
    switch (event.type) {
        case monitoring::SessionEventType::handshake_error:
            std::cerr << event.ec.what() << std::endl;
            session->abort_handshake();
            break;
        case monitoring::SessionEventType::disconnected:
            std::cerr << event.ec.what() << std::endl;
            session->stop();
            break;
        case monitoring::SessionEventType::write_error:
            std::cerr << event.ec.what() << std::endl;
            session->stop();
            break;
        case monitoring::SessionEventType::read_error:
            std::cerr << event.ec.what() << std::endl;
            session->stop();
            break;
        case monitoring::SessionEventType::close_error:
            std::cerr << event.ec.what() << std::endl;
            break;
    }
}