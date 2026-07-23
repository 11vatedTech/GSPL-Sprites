#pragma once
#include "gspl/studio/ipc_envelope.hpp"
#include <chrono>
#include <functional>
#include <memory>

namespace gspl::studio {

class IpcChannel {
public:
    using MessageHandler = std::function<void(IpcEnvelope)>;

    explicit IpcChannel(int read_fd, int write_fd);
    ~IpcChannel();

    IpcChannel(const IpcChannel&) = delete;
    IpcChannel& operator=(const IpcChannel&) = delete;
    IpcChannel(IpcChannel&&) noexcept;
    IpcChannel& operator=(IpcChannel&&) noexcept;

    void send(const IpcEnvelope& envelope);
    [[nodiscard]] auto receive(std::chrono::milliseconds timeout) -> IpcEnvelope;

    void start_async(MessageHandler on_message);
    void stop_async();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
