#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace gspl::studio {

class SharedMemory {
public:
    explicit SharedMemory(std::string key, std::uint64_t size);
    ~SharedMemory();

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
    SharedMemory(SharedMemory&&) noexcept;
    SharedMemory& operator=(SharedMemory&&) noexcept;

    bool write(std::string_view data);
    [[nodiscard]] std::string read() const;

    [[nodiscard]] auto key() const -> const std::string&;
    [[nodiscard]] auto size() const -> std::uint64_t;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
