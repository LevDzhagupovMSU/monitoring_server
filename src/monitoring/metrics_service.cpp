#include "monitoring/metrics_service.hpp"

#include <memory>

monitoring::MetricsService::MetricsService(boost::asio::io_context& io,
                                           SharedState& state,
                                           std::chrono::milliseconds interval) : 
                    timer_(io), state_(state), interval_(interval) {};

void monitoring::MetricsService::start(){
    schedule_next_tick();
}

void monitoring::MetricsService::stop(){
    timer_.cancel();
}

void monitoring::MetricsService::schedule_next_tick(){
    timer_.expires_after(interval_);

    timer_.async_wait([this](const boost::beast::error_code& ec){
        on_timer(ec);
    });
}

void monitoring::MetricsService::on_timer(const boost::beast::error_code& ec){
    if(ec){
        return; // заглушка
    }

    auto metrics = generator_.collect();
    auto json_metrics = make_json(metrics);

    state_.publish(std::make_shared<const std::string>(json_metrics.dump()));

    schedule_next_tick();
}

nlohmann::json monitoring::MetricsService::make_json(const MetricsSnapshot& snapshot) const{
    auto timestamp_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              snapshot.collected_at.time_since_epoch()
          ).count();


    return {{"type", "metric"},
            {"version", 1},
            {"timestamp_ms", timestamp_ms},
            {"data", {
                {"cpu", {{"total_percent", snapshot.cpu_percent}}},
                {"gpu", {{"total_percent", snapshot.gpu_percent}}},
                {"memory", {
                        {"used_mb" , snapshot.memory_used_mb}, 
                        {"percent", snapshot.memory_percent}
                    }
                }
                }
            }
        };
}
