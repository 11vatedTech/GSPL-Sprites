#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>

namespace gspl::studio {

struct IpcEnvelope {
    static constexpr std::uint32_t MAGIC = 0x4753504C;
    static constexpr std::uint32_t PROTOCOL_VERSION = 1;

    std::uint32_t magic{MAGIC};
    std::uint32_t version{PROTOCOL_VERSION};
    std::uint64_t message_id{0};
    std::string   method;
    std::string   payload;
    bool          has_shared_memory{false};
    std::string   shared_memory_key;
    std::uint64_t shared_memory_size{0};

    [[nodiscard]] std::string serialize() const;
    static auto deserialize(std::string_view data) -> IpcEnvelope;
};

} // namespace gspl::studio
