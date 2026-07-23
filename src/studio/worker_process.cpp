#include "gspl/studio/worker_process.hpp"
#include "gspl/studio/ipc_envelope.hpp"
#include <QProcess>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <thread>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace gspl::studio {

namespace {

constexpr std::uint64_t PING_MESSAGE_ID = 0xFFFFFFFFFFFFFFFEULL;
constexpr std::uint64_t PONG_MESSAGE_ID = 0xFFFFFFFFFFFFFFFDULL;
constexpr std::chrono::milliseconds CRASH_WINDOW{10000};

struct ParentPipes {
    int read_fd{-1};
    int write_fd{-1};

    void close() {
        if (read_fd >= 0) {
#ifdef _WIN32
            _close(read_fd);
#else
            ::close(read_fd);
#endif
            read_fd = -1;
        }
        if (write_fd >= 0) {
#ifdef _WIN32
            _close(write_fd);
#else
            ::close(write_fd);
#endif
            write_fd = -1;
        }
    }
};

struct ChildPipes {
#ifdef _WIN32
    HANDLE stdin_rd{nullptr};
    HANDLE stdout_wr{nullptr};

    ~ChildPipes() {
        close();
    }

    ChildPipes() = default;
    ChildPipes(ChildPipes&& other) noexcept
        : stdin_rd(other.stdin_rd), stdout_wr(other.stdout_wr) {
        other.stdin_rd = nullptr;
        other.stdout_wr = nullptr;
    }
    ChildPipes& operator=(ChildPipes&& other) noexcept {
        if (this != &other) {
            close();
            stdin_rd = other.stdin_rd;
            stdout_wr = other.stdout_wr;
            other.stdin_rd = nullptr;
            other.stdout_wr = nullptr;
        }
        return *this;
    }
    ChildPipes(const ChildPipes&) = delete;
    ChildPipes& operator=(const ChildPipes&) = delete;

    void close() {
        if (stdin_rd) { CloseHandle(stdin_rd); stdin_rd = nullptr; }
        if (stdout_wr) { CloseHandle(stdout_wr); stdout_wr = nullptr; }
    }
#endif
};

bool create_pipes(ParentPipes& parent, ChildPipes& child) {
#ifdef _WIN32
    HANDLE hStdinRd, hStdinWr;
    HANDLE hStdoutRd, hStdoutWr;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&hStdinRd, &hStdinWr, &sa, 0)) return false;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) {
        CloseHandle(hStdinRd); CloseHandle(hStdinWr);
        return false;
    }

    SetHandleInformation(hStdinWr, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    child.stdin_rd = hStdinRd;
    child.stdout_wr = hStdoutWr;

    parent.read_fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdoutRd), _O_RDONLY);
    parent.write_fd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdinWr), _O_WRONLY);

    return parent.read_fd >= 0 && parent.write_fd >= 0;
#else
    int stdin_pipe[2], stdout_pipe[2];
    if (pipe(stdin_pipe) != 0) return false;
    if (pipe(stdout_pipe) != 0) {
        ::close(stdin_pipe[0]); ::close(stdin_pipe[1]);
        return false;
    }
    parent.read_fd = stdout_pipe[0];
    parent.write_fd = stdin_pipe[1];
    return true;
#endif
}

} // anonymous namespace

struct WorkerProcess::Impl {
    Config config;
    WorkerState state_{WorkerState::Stopped};
    StateCallback state_cb;
    QProcess process;
    std::unique_ptr<IpcChannel> channel_;
    ParentPipes parent_pipes;
    ChildPipes child_pipes;
    std::atomic<bool> running_{false};
    std::thread health_thread;
    mutable std::mutex state_mutex;
    int restart_count{0};
    std::chrono::steady_clock::time_point last_restart;

    explicit Impl(Config cfg) : config(std::move(cfg)) {}

    void set_state(WorkerState s) {
        {
            std::lock_guard<std::mutex> lock(state_mutex);
            state_ = s;
        }
        if (state_cb) state_cb(s);
    }
};

WorkerProcess::WorkerProcess(Config config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

WorkerProcess::~WorkerProcess() {
    stop();
}

WorkerProcess::WorkerProcess(WorkerProcess&&) noexcept = default;
WorkerProcess& WorkerProcess::operator=(WorkerProcess&&) noexcept = default;

void WorkerProcess::start() {
    if (impl_->state_ != WorkerState::Stopped &&
        impl_->state_ != WorkerState::Terminated) return;
    impl_->set_state(WorkerState::Starting);

    if (!create_pipes(impl_->parent_pipes, impl_->child_pipes)) {
        impl_->set_state(WorkerState::Terminated);
        return;
    }

    auto* proc = &impl_->process;

#ifdef _WIN32
    proc->setChildProcessModifier([this](QProcess::CreateProcessArguments* args) {
        args->dwFlags |= STARTF_USESTDHANDLES;
        args->hStdInput = impl_->child_pipes.stdin_rd;
        args->hStdOutput = impl_->child_pipes.stdout_wr;
        args->hStdError = GetStdHandle(STD_ERROR_HANDLE);
    });
#else
    proc->setChildProcessModifier([this](void* /* attr */) {
        // On POSIX this would set up posix_spawn_file_actions_t
        // to redirect stdin/stdout to our pipes. The default
        // QProcess pipe forwarding is used for now.
    });
#endif

    QObject::connect(proc, &QProcess::finished,
        [this](int /* exitCode */, QProcess::ExitStatus status) {
            impl_->running_ = false;
            if (impl_->health_thread.joinable()) {
                impl_->health_thread.join();
            }
            impl_->channel_.reset();
            impl_->parent_pipes.close();

            auto now = std::chrono::steady_clock::now();
            if (status == QProcess::CrashExit &&
                (now - impl_->last_restart) < CRASH_WINDOW) {
                ++impl_->restart_count;
                if (impl_->restart_count > impl_->config.restart_limit) {
                    impl_->set_state(WorkerState::CrashLoop);
                    return;
                }
            } else {
                impl_->restart_count = 0;
            }
            impl_->last_restart = now;
            impl_->set_state(WorkerState::Stopped);

            if (impl_->restart_count > 0 &&
                impl_->restart_count <= impl_->config.restart_limit) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100) *
                    (1 << (impl_->restart_count - 1)));
                start();
            }
        });

    QObject::connect(proc, &QProcess::errorOccurred,
        [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                impl_->set_state(WorkerState::Terminated);
            }
        });

    if (!impl_->config.working_directory.empty()) {
        proc->setWorkingDirectory(
            QString::fromStdString(impl_->config.working_directory));
    }

    QStringList args;
    for (auto const& a : impl_->config.args) {
        args << QString::fromStdString(a);
    }

    proc->start(QString::fromStdString(impl_->config.worker_path), args);

    if (!proc->waitForStarted(
            static_cast<int>(impl_->config.startup_timeout.count()))) {
        impl_->set_state(WorkerState::Terminated);
        return;
    }

    impl_->child_pipes.close();

    impl_->channel_ = std::make_unique<IpcChannel>(
        impl_->parent_pipes.read_fd, impl_->parent_pipes.write_fd);
    impl_->parent_pipes.read_fd = -1;
    impl_->parent_pipes.write_fd = -1;

    impl_->running_ = true;
    impl_->health_thread = std::thread([this]() {
        while (impl_->running_) {
            std::this_thread::sleep_for(impl_->config.health_check_interval);
            if (!impl_->running_) break;
            IpcEnvelope ping;
            ping.message_id = PING_MESSAGE_ID;
            ping.method = "ping";
            impl_->channel_->send(ping);
            auto pong = impl_->channel_->receive(
                impl_->config.health_check_interval);
            if (pong.message_id != PONG_MESSAGE_ID) {
                impl_->running_ = false;
                impl_->process.kill();
                break;
            }
        }
    });

    impl_->set_state(WorkerState::Running);
}

void WorkerProcess::stop() {
    impl_->running_ = false;
    if (impl_->health_thread.joinable()) {
        impl_->health_thread.join();
    }
    impl_->channel_.reset();
    impl_->parent_pipes.close();
    impl_->child_pipes.close();
    impl_->process.close();
    impl_->process.waitForFinished(3000);
    if (impl_->process.state() != QProcess::NotRunning) {
        impl_->process.kill();
        impl_->process.waitForFinished(1000);
    }
    impl_->set_state(WorkerState::Stopped);
}

void WorkerProcess::restart() {
    stop();
    impl_->restart_count = 0;
    start();
}

auto WorkerProcess::state() const -> WorkerState {
    std::lock_guard<std::mutex> lock(impl_->state_mutex);
    return impl_->state_;
}

auto WorkerProcess::channel() -> IpcChannel& {
    return *impl_->channel_;
}

void WorkerProcess::set_state_callback(StateCallback cb) {
    impl_->state_cb = std::move(cb);
}

} // namespace gspl::studio
