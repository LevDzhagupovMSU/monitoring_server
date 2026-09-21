#pragma once

#include <memory>

#include "session_event.hpp"
#include <boost/beast/core/error.hpp>

namespace monitoring {

class Session;
class Server;

class SessionHandler{
public:
    void on_event(std::shared_ptr<Session> session, 
                  const SessionEvent& event);
};

}