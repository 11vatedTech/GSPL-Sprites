#include "gspl/studio/metrics.hpp"
#include <algorithm>
#include <mutex>
#include <vector>
#include <unordered_map>

namespace gspl::studio {

struct MetricsCollector::Impl {
    std::unordered_map<std::string, double> counters_;
    std::unordered_map<std::string, double> gauges_;
    struct TimingSample {
        std::string name;
        std::chrono::microseconds duration;
    };
    std::vector<TimingSample> timings_;
    std::mutex mutex_;
};

MetricsCollector::MetricsCollector() : impl_(std::make_unique<Impl>()) {}
MetricsCollector::~MetricsCollector() = default;

void MetricsCollector::increment(std::string_view name, double value) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->counters_[std::string(name)] += value;
}

void MetricsCollector::gauge(std::string_view name, double value) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->gauges_[std::string(name)] = value;
}

void MetricsCollector::timing(std::string_view name, std::chrono::microseconds duration) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->timings_.push_back({std::string(name), duration});
}

auto MetricsCollector::get_counter(std::string_view name) const -> double {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    auto it = impl_->counters_.find(std::string(name));
    return it != impl_->counters_.end() ? it->second : 0.0;
}

auto MetricsCollector::get_gauge(std::string_view name) const -> double {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    auto it = impl_->gauges_.find(std::string(name));
    return it != impl_->gauges_.end() ? it->second : 0.0;
}

auto MetricsCollector::get_timing_avg(std::string_view name) const -> double {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    double sum = 0;
    int count = 0;
    for (const auto& t : impl_->timings_) {
        if (t.name == name) {
            sum += static_cast<double>(t.duration.count());
            ++count;
        }
    }
    return count > 0 ? sum / count : 0.0;
}

auto MetricsCollector::snapshot() const -> std::unordered_map<std::string, double> {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    std::unordered_map<std::string, double> result;
    for (const auto& [k, v] : impl_->counters_) result["counter_" + k] = v;
    for (const auto& [k, v] : impl_->gauges_) result["gauge_" + k] = v;
    return result;
}

void MetricsCollector::reset() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->counters_.clear();
    impl_->gauges_.clear();
    impl_->timings_.clear();
}

MetricsCollector::Timer::Timer(MetricsCollector& collector, std::string_view name)
    : collector_(&collector), name_(name), start_(std::chrono::steady_clock::now()) {}

MetricsCollector::Timer::~Timer() { if (!stopped_) stop(); }

void MetricsCollector::Timer::stop() {
    if (stopped_) return;
    stopped_ = true;
    auto end = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    collector_->timing(name_, dur);
}

} // namespace gspl::studio
