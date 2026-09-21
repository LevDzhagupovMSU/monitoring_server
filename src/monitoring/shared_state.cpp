#include "monitoring/shared_state.hpp"

#include "session.hpp"

#include <iostream>
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>


void monitoring::SharedState::join(const std::shared_ptr<Session>& session){
    sessions_.insert({session});
}

void monitoring::SharedState::leave(const Session& session){ 
    auto it = std::find_if(sessions_.begin(), sessions_.end(),[&session](const std::weak_ptr<Session>& weak){
        auto shared = weak.lock();
        return &session == shared.get();
    });
    if(it != sessions_.end())
        sessions_.erase(it);
}

std::vector<std::shared_ptr<monitoring::Session>> monitoring::SharedState::detach_all(){
    std::vector<std::shared_ptr<Session>> result;
    result.reserve(sessions_.size());

    for(const auto& weak : sessions_){
        if(auto shared_session = weak.lock()){
            result.push_back(std::move(shared_session));
        }
    }

    sessions_.clear();
    return result;
}

void monitoring::SharedState::publish(std::shared_ptr<const std::string> message){
    for(const auto& session : sessions_){
        auto shared_session = session.lock();
        
        if(shared_session)
            shared_session->send(message);
    }
}
