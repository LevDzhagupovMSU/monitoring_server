#pragma once

#include "monitoring/metrics_generator.hpp"
#include "monitoring/shared_state.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core/error.hpp>
#include <nlohmann/json.hpp>

#include <chrono>

namespace monitoring {

// Один периодический источник метрик для всех сессий сервера.
class MetricsService {
    boost::asio::steady_timer timer_;
    SharedState& state_;
    MetricsGenerator generator_;
    std::chrono::milliseconds interval_;

    void schedule_next_tick();
    void on_timer(const boost::beast::error_code& ec);
    [[nodiscard]] nlohmann::json make_json(const MetricsSnapshot& snapshot) const;

public:
    MetricsService(boost::asio::io_context& io,
                   SharedState& state,
                   std::chrono::milliseconds interval = std::chrono::seconds{1});

    void start();
    void stop();
};

} // namespace monitoring
