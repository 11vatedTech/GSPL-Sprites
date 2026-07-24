#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace gspl::studio {

struct TelemetryEvent {
    std::string name;
    std::string category;
    std::string data;
    std::chrono::system_clock::time_point timestamp;
    double duration_ms{0.0};
};

class TelemetryBus {
public:
    using EventCallback = std::function<void(const TelemetryEvent&)>;

    TelemetryBus();
    ~TelemetryBus();

    TelemetryBus(const TelemetryBus&) = delete;
    TelemetryBus& operator=(const TelemetryBus&) = delete;

    void emit(std::string_view name, std::string_view category = "",
              std::string_view data = "", double duration_ms = 0.0);
    void subscribe(std::string_view category, EventCallback callback);
    void unsubscribe_all(std::string_view category);

    void start_session();
    void end_session();
    [[nodiscard]] bool is_active() const;

    [[nodiscard]] auto buffered_events() const -> const std::vector<TelemetryEvent>&;
    void clear_buffer();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
