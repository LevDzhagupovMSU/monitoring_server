#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "monitoring/metrics_generator.hpp"

namespace {

bool read_cpu_times(std::uint64_t& total, std::uint64_t& idle) {
    std::ifstream stat{"/proc/stat"};
    std::string line;

    if (!std::getline(stat, line)) {
        return false;
    }

    std::istringstream input{line};
    std::string name;
    std::array<std::uint64_t, 10> times{};
    input >> name;

    if (name != "cpu") {
        return false;
    }

    for (auto& time : times) {
        input >> time;
    }

    total = 0;
    for (const auto time : times) {
        total += time;
    }

    // idle и iowait означают, что процессор не выполнял полезную работу.
    idle = times[3] + times[4];
    return true;
}

double read_memory_used_mb(double& memory_percent) {
    std::ifstream meminfo{"/proc/meminfo"};
    std::string key;
    std::uint64_t value_kb{};
    std::string unit;
    std::uint64_t total_kb{};
    std::uint64_t available_kb{};

    while (meminfo >> key >> value_kb >> unit) {
        if (key == "MemTotal:") {
            total_kb = value_kb;
        } else if (key == "MemAvailable:") {
            available_kb = value_kb;
        }
    }

    if (total_kb == 0) {
        memory_percent = 0.0;
        return 0.0;
    }

    const auto used_kb = total_kb - std::min(total_kb, available_kb);
    memory_percent = static_cast<double>(used_kb) * 100.0 / total_kb;
    return static_cast<double>(used_kb) / 1024.0;
}

double read_gpu_percent() {
    namespace fs = std::filesystem;

    std::error_code error;
    const fs::path drm_path{"/sys/class/drm"};

    for (const auto& entry : fs::directory_iterator{drm_path, error}) {
        if (error || !entry.is_directory(error) ||
            !entry.path().filename().string().starts_with("card")) {
            continue;
        }

        std::ifstream busy_file{entry.path() / "device/gpu_busy_percent"};
        double percent{};

        if (busy_file >> percent) {
            return std::clamp(percent, 0.0, 100.0);
        }
    }

    FILE* command = popen(
        "nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null",
        "r");
    if (command) {
        char output[64]{};
        double percent{};
        if (std::fgets(output, sizeof(output), command) != nullptr) {
            pclose(command);
            std::istringstream input{output};
            if (input >> percent) {
                return std::clamp(percent, 0.0, 100.0);
            }
        } else {
            pclose(command);
        }
    }

    return 0.0;
}

} // namespace

monitoring::MetricsSnapshot monitoring::MetricsGenerator::collect() {
    MetricsSnapshot snapshot;
    snapshot.collected_at = std::chrono::system_clock::now();

    std::uint64_t cpu_total{};
    std::uint64_t cpu_idle{};

    if (read_cpu_times(cpu_total, cpu_idle)) {
        if (has_previous_cpu_sample_ && cpu_total > previous_cpu_total_) {
            const auto total_delta = cpu_total - previous_cpu_total_;
            const auto idle_delta = cpu_idle - previous_cpu_idle_;
            const auto busy_delta = total_delta - std::min(total_delta, idle_delta);

            snapshot.cpu_percent = static_cast<double>(busy_delta) * 100.0 / total_delta;
        }

        previous_cpu_total_ = cpu_total;
        previous_cpu_idle_ = cpu_idle;
        has_previous_cpu_sample_ = true;
    }

    snapshot.memory_used_mb = read_memory_used_mb(snapshot.memory_percent);
    snapshot.gpu_percent = read_gpu_percent();
    return snapshot;
}
