#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <chrono>

namespace gspl::studio {

class MetricsCollector {
public:
    MetricsCollector();
    ~MetricsCollector();

    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    void increment(std::string_view name, double value = 1.0);
    void gauge(std::string_view name, double value);
    void timing(std::string_view name, std::chrono::microseconds duration);

    [[nodiscard]] auto get_counter(std::string_view name) const -> double;
    [[nodiscard]] auto get_gauge(std::string_view name) const -> double;
    [[nodiscard]] auto get_timing_avg(std::string_view name) const -> double;

    [[nodiscard]] auto snapshot() const -> std::unordered_map<std::string, double>;
    void reset();

    struct Timer {
        explicit Timer(MetricsCollector& collector, std::string_view name);
        ~Timer();
        void stop();
    private:
        MetricsCollector* collector_;
        std::string name_;
        std::chrono::steady_clock::time_point start_;
        bool stopped_{false};
    };

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
