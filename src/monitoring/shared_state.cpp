#include "monitoring/shared_state.hpp"

#include "session.hpp"

#include <iostream>
#include <algorithm>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>


void monitoring::SharedState::join(const std::shared_ptr<Session>& session){
    std::lock_guard<std::mutex> lg(mutex_);
    sessions_.insert({session});
}

void monitoring::SharedState::leave(const Session& session){ 
    std::lock_guard<std::mutex> lg(mutex_);
    auto it = std::find_if(sessions_.begin(), sessions_.end(),[&session](const std::weak_ptr<Session>& weak){
        auto shared = weak.lock();
        return &session == shared.get();
    });
    if(it != sessions_.end())
        sessions_.erase(it);
}

std::vector<std::shared_ptr<monitoring::Session>> monitoring::SharedState::detach_all(){
    std::lock_guard<std::mutex> lg(mutex_);
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
    std::vector<std::shared_ptr<Session>> sessions;

    {
        std::lock_guard<std::mutex> lc(mutex_);
        for(const auto& weak : sessions_){
            auto shared = weak.lock();
            if(shared){
                sessions.push_back(shared);
            }
        }
    }

    for(const auto& session : sessions)
        session->send(message);
}
