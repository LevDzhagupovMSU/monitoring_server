#pragma once

#include <chrono>

namespace monitoring {

// Один согласованный снимок состояния системы для всех клиентов.
struct MetricsSnapshot {
    double cpu_percent{};
    double gpu_percent{};
    double memory_percent{};
    double memory_used_mb{};

    std::chrono::system_clock::time_point collected_at{};
};

} // namespace monitoring
