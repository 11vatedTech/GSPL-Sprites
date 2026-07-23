#pragma once
#include "gspl/studio/ipc_channel.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::studio {

enum class WorkerState {
    Stopped,
    Starting,
    Running,
    CrashLoop,
    Terminated
};

class WorkerProcess {
public:
    struct Config {
        std::string worker_path;
        std::vector<std::string> args;
        std::string working_directory;
        int restart_limit{3};
        std::chrono::milliseconds health_check_interval{5000};
        std::chrono::milliseconds startup_timeout{30000};
    };

    using StateCallback = std::function<void(WorkerState)>;

    explicit WorkerProcess(Config config);
    ~WorkerProcess();

    WorkerProcess(const WorkerProcess&) = delete;
    WorkerProcess& operator=(const WorkerProcess&) = delete;
    WorkerProcess(WorkerProcess&&) noexcept;
    WorkerProcess& operator=(WorkerProcess&&) noexcept;

    void start();
    void stop();
    void restart();

    [[nodiscard]] auto state() const -> WorkerState;
    [[nodiscard]] auto channel() -> IpcChannel&;
    void set_state_callback(StateCallback cb);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
