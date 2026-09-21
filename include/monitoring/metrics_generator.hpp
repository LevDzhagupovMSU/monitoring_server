#pragma once

#include "monitoring/metrics_snapshot.hpp"

#include <cstdint>

namespace monitoring {

// Собирает системные метрики Linux из /proc и /sys.
class MetricsGenerator {
    std::uint64_t previous_cpu_total_{};
    std::uint64_t previous_cpu_idle_{};
    bool has_previous_cpu_sample_{};

public:
    MetricsGenerator() = default;

    [[nodiscard]] MetricsSnapshot collect();
};

} // namespace monitoring
