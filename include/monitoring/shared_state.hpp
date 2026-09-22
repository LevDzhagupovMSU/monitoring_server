#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

namespace monitoring {

class Session;

// Реестр подключённых сессий и единая точка рассылки данных.
class SharedState {
    std::set<std::weak_ptr<Session>, std::owner_less<std::weak_ptr<Session>>> sessions_;
    std::mutex mutex_;
public:
    void join(const std::shared_ptr<Session>& session);
    void leave(const Session& session);
    std::vector<std::shared_ptr<Session>> detach_all();

    // Передаёт одно и то же сообщение всем активным клиентам.
    void publish(std::shared_ptr<const std::string> message);
};

} // namespace monitoring
