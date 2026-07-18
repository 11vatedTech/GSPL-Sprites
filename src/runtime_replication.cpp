#include "gspl_sprites/runtime_replication.hpp"

#include "gspl_sprites/core.hpp"

#include <charconv>
#include <limits>
#include <stdexcept>

namespace gspl::sprites {
namespace {
bool hash(std::string_view value) {
  return value.size() == 64 && std::ranges::all_of(value, [](unsigned char c) {
           return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
         });
}
std::uint64_t integer(std::string_view value) {
  std::uint64_t result{};
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), result);
  if (error != std::errc{} || end != value.data() + value.size())
    throw std::runtime_error("invalid replication integer");
  return result;
}
std::string line(std::string_view source, std::size_t &position,
                 std::string_view key) {
  const auto end = source.find('\n', position);
  if (end == std::string_view::npos)
    throw std::runtime_error("truncated replication header");
  const auto prefix = std::string(key) + '=';
  const auto value = source.substr(position, end - position);
  if (!value.starts_with(prefix))
    throw std::runtime_error("unexpected replication header field");
  position = end + 1;
  return std::string(value.substr(prefix.size()));
}
std::string state_identity(const LivingRuntimeProgram &program,
                           const LivingRuntimeState &state) {
  return sha256(serialize_living_runtime_state(program, state));
}
} // namespace

RuntimeReplicationUpdate make_runtime_replication_update(
    const LivingRuntimeProgram &program, const LivingRuntimeState &base,
    const LivingRuntimeState &authoritative) {
  if (authoritative.tick < base.tick)
    throw std::invalid_argument("authoritative runtime state rolls back time");
  RuntimeReplicationUpdate result;
  result.program_identity = living_runtime_program_identity(program);
  result.base_state_identity = state_identity(program, base);
  result.target_state = serialize_living_runtime_state(program, authoritative);
  result.target_state_identity = sha256(result.target_state);
  result.target_tick = authoritative.tick;
  return result;
}

std::string serialize_runtime_replication_update(
    const RuntimeReplicationUpdate &update) {
  if (!hash(update.program_identity) || !hash(update.base_state_identity) ||
      !hash(update.target_state_identity) || update.target_state.empty() ||
      sha256(update.target_state) != update.target_state_identity)
    throw std::invalid_argument("runtime replication update is malformed");
  return "schema=gspl.runtime-replication/0.1\nprogram=" +
         update.program_identity + "\nbase=" + update.base_state_identity +
         "\ntarget=" + update.target_state_identity + "\ntick=" +
         std::to_string(update.target_tick) + "\nbytes=" +
         std::to_string(update.target_state.size()) + "\n" +
         update.target_state;
}

RuntimeReplicationUpdate deserialize_runtime_replication_update(
    std::string_view source, std::uint64_t maximum_bytes) {
  if (source.empty() || source.size() > maximum_bytes)
    throw std::runtime_error("replication update exceeds byte limit");
  std::size_t position = 0;
  if (line(source, position, "schema") != "gspl.runtime-replication/0.1")
    throw std::runtime_error("unsupported replication schema");
  RuntimeReplicationUpdate result;
  result.program_identity = line(source, position, "program");
  result.base_state_identity = line(source, position, "base");
  result.target_state_identity = line(source, position, "target");
  result.target_tick = integer(line(source, position, "tick"));
  const auto bytes = integer(line(source, position, "bytes"));
  if (bytes != source.size() - position ||
      bytes > std::numeric_limits<std::size_t>::max())
    throw std::runtime_error("replication payload length mismatch");
  result.target_state = std::string(source.substr(position));
  if (serialize_runtime_replication_update(result) != source)
    throw std::runtime_error("replication update is not canonical");
  return result;
}

void apply_runtime_replication_update(const LivingRuntimeProgram &program,
                                      LivingRuntimeState &local_state,
                                      const RuntimeReplicationUpdate &update) {
  const auto expected_program = living_runtime_program_identity(program);
  if (update.program_identity != expected_program)
    throw std::runtime_error("replication program identity mismatch");
  if (state_identity(program, local_state) != update.base_state_identity)
    throw std::runtime_error("replication base state diverged");
  if (update.target_tick < local_state.tick)
    throw std::runtime_error("replication update attempts rollback");
  if (sha256(update.target_state) != update.target_state_identity)
    throw std::runtime_error("replication target identity mismatch");
  auto candidate = deserialize_living_runtime_state(program, update.target_state);
  if (candidate.tick != update.target_tick)
    throw std::runtime_error("replication target tick mismatch");
  local_state = std::move(candidate);
}

} // namespace gspl::sprites
