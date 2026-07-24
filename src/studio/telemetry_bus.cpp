#include "gspl/studio/telemetry_bus.hpp"
#include <algorithm>
#include <mutex>
#include <unordered_map>

namespace gspl::studio {

struct TelemetryBus::Impl {
    std::vector<TelemetryEvent> buffer_;
    std::unordered_map<std::string, std::vector<EventCallback>> subscribers_;
    bool active_{false};
    std::mutex mutex_;
};

TelemetryBus::TelemetryBus() : impl_(std::make_unique<Impl>()) {}
TelemetryBus::~TelemetryBus() = default;

void TelemetryBus::emit(std::string_view name, std::string_view category,
                        std::string_view data, double duration_ms) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    TelemetryEvent event{
        std::string(name),
        std::string(category),
        std::string(data),
        std::chrono::system_clock::now(),
        duration_ms
    };
    impl_->buffer_.push_back(event);

    auto it = impl_->subscribers_.find(std::string(category));
    if (it != impl_->subscribers_.end()) {
        for (auto& cb : it->second) {
            cb(event);
        }
    }
    auto all_it = impl_->subscribers_.find("");
    if (all_it != impl_->subscribers_.end()) {
        for (auto& cb : all_it->second) {
            cb(event);
        }
    }
}

void TelemetryBus::subscribe(std::string_view category, EventCallback callback) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->subscribers_[std::string(category)].push_back(std::move(callback));
}

void TelemetryBus::unsubscribe_all(std::string_view category) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->subscribers_.erase(std::string(category));
}

void TelemetryBus::start_session() { impl_->active_ = true; }
void TelemetryBus::end_session() { impl_->active_ = false; }
bool TelemetryBus::is_active() const { return impl_->active_; }

auto TelemetryBus::buffered_events() const -> const std::vector<TelemetryEvent>& {
    return impl_->buffer_;
}

void TelemetryBus::clear_buffer() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->buffer_.clear();
}

} // namespace gspl::studio
