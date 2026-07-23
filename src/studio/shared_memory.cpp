#include "gspl/studio/shared_memory.hpp"
#include <QSharedMemory>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>

namespace gspl::studio {

struct SharedMemory::Impl {
    std::string base_key;
    std::uint64_t mem_size;
    QSharedMemory buffers[2];
    std::atomic<int> active_buffer{0};
    mutable std::mutex mutex;

    Impl(std::string key, std::uint64_t size)
        : base_key(std::move(key)), mem_size(size),
          buffers{QSharedMemory(QString::fromStdString(base_key + "_a")),
                  QSharedMemory(QString::fromStdString(base_key + "_b"))} {}

    int flip() {
        return active_buffer.exchange((active_buffer + 1) % 2);
    }
};

SharedMemory::SharedMemory(std::string key, std::uint64_t size)
    : impl_(std::make_unique<Impl>(std::move(key), size)) {}

SharedMemory::~SharedMemory() {
    for (auto& buf : impl_->buffers) {
        if (buf.isAttached()) {
            buf.detach();
        }
    }
}

SharedMemory::SharedMemory(SharedMemory&&) noexcept = default;
SharedMemory& SharedMemory::operator=(SharedMemory&&) noexcept = default;

bool SharedMemory::write(std::string_view data) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto write_buf = impl_->flip();
    auto& buf = impl_->buffers[write_buf];

    if (buf.isAttached()) {
        buf.detach();
    }

    auto size = std::min(data.size(), static_cast<std::size_t>(impl_->mem_size));
    if (!buf.create(static_cast<int>(impl_->mem_size))) {
        if (buf.error() == QSharedMemory::AlreadyExists) {
            buf.attach();
        } else {
            return false;
        }
    }

    if (!buf.attach(QSharedMemory::ReadWrite)) {
        return false;
    }

    buf.lock();
    std::memcpy(buf.data(), data.data(), size);
    buf.unlock();
    buf.detach();

    return true;
}

auto SharedMemory::read() const -> std::string {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto read_buf = impl_->active_buffer.load();
    auto& buf = impl_->buffers[read_buf];

    if (!buf.attach(QSharedMemory::ReadOnly)) {
        return {};
    }

    buf.lock();
    auto size = static_cast<std::size_t>(buf.size());
    std::string result(static_cast<const char*>(buf.constData()), size);
    buf.unlock();
    buf.detach();

    return result;
}

auto SharedMemory::key() const -> const std::string& {
    return impl_->base_key;
}

auto SharedMemory::size() const -> std::uint64_t {
    return impl_->mem_size;
}

} // namespace gspl::studio
