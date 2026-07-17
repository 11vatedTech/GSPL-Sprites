#include "gspl_sprites/runtime_persistence.hpp"

#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
std::vector<std::string> split(std::string_view value, char delimiter) {
  std::vector<std::string> result;
  std::size_t start = 0;
  while (start <= value.size()) {
    const auto end = value.find(delimiter, start);
    result.emplace_back(value.substr(start, end == std::string_view::npos
                                                ? value.size() - start
                                                : end - start));
    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }
  return result;
}
template <class T> T integer(std::string_view value, std::string_view field) {
  T output{};
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), output);
  if (error != std::errc{} || end != value.data() + value.size())
    throw std::runtime_error("invalid integer for " + std::string(field));
  return output;
}
std::string condition(const RuntimeCondition &value) {
  return value.variable + '|' +
         std::to_string(static_cast<int>(value.comparison)) + '|' +
         std::to_string(value.threshold);
}
void append_events(std::vector<RuntimeEvent> &target,
                   std::span<const RuntimeEvent> source,
                   std::uint64_t maximum) {
  if (source.size() > maximum - target.size())
    throw std::runtime_error("runtime replay event limit exceeded");
  target.insert(target.end(), source.begin(), source.end());
}
} // namespace

std::string
canonicalize_living_runtime_program(const LivingRuntimeProgram &program) {
  const auto validation = validate_living_runtime_program(program);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
  auto goals = program.goals;
  auto actions = program.actions;
  std::ranges::sort(goals, {}, &RuntimeGoalDefinition::id);
  std::ranges::sort(actions, {}, &RuntimeActionDefinition::id);
  std::ostringstream output;
  output << "schema=gspl.living-runtime-program/0.1\nid=" << program.id
         << "\nticks_per_second=" << program.ticks_per_second
         << "\nmaximum_memory_records=" << program.maximum_memory_records
         << '\n';
  for (auto &goal : goals) {
    std::ranges::sort(goal.activation, {}, [](const auto &value) {
      return std::tuple{value.variable, value.comparison, value.threshold};
    });
    output << "goal=" << goal.id << '|' << goal.priority << '\n';
    for (const auto &value : goal.activation)
      output << "goal_condition=" << goal.id << '|' << condition(value) << '\n';
  }
  for (auto &action : actions) {
    std::ranges::sort(action.utility_terms, {}, [](const auto &value) {
      return std::tuple{value.variable, value.weight_per_million};
    });
    std::ranges::sort(action.preconditions, {}, [](const auto &value) {
      return std::tuple{value.variable, value.comparison, value.threshold};
    });
    std::ranges::sort(action.markers, {}, [](const auto &value) {
      return std::tuple{value.tick_offset, value.id};
    });
    output << "action=" << action.id << '|' << action.goal_id << '|'
           << action.base_utility << '|' << action.duration_ticks << '|'
           << action.cooldown_ticks << '|' << action.energy_cost << '|'
           << (action.interruptible ? "true" : "false") << '\n';
    for (const auto &term : action.utility_terms)
      output << "utility=" << action.id << '|' << term.variable << '|'
             << term.weight_per_million << '\n';
    for (const auto &value : action.preconditions)
      output << "precondition=" << action.id << '|' << condition(value) << '\n';
    for (const auto &marker : action.markers)
      output << "marker=" << action.id << '|' << marker.id << '|'
             << marker.tick_offset << '\n';
  }
  return output.str();
}

std::string
living_runtime_program_identity(const LivingRuntimeProgram &program) {
  return sha256(canonicalize_living_runtime_program(program));
}

std::string serialize_living_runtime_state(const LivingRuntimeProgram &program,
                                           const LivingRuntimeState &state) {
  const auto validation = validate_living_runtime_state(program, state);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
  auto memory = state.memory;
  std::ranges::sort(memory, [](const auto &left, const auto &right) {
    return std::tuple{left.observation.observed_tick, left.sequence,
                      left.observation.key, left.observation.source_entity} <
           std::tuple{right.observation.observed_tick, right.sequence,
                      right.observation.key, right.observation.source_entity};
  });
  std::ostringstream output;
  output << "schema=gspl.living-runtime-state/0.1\nprogram="
         << living_runtime_program_identity(program) << "\ntick=" << state.tick
         << "\nnext_sequence=" << state.next_sequence
         << "\nenergy=" << state.energy << '\n';
  for (const auto &[key, value] : state.variables)
    output << "variable=" << key << '|' << value << '\n';
  for (const auto &record : memory) {
    const auto &o = record.observation;
    output << "memory=" << o.key << '|' << o.source_entity << '|' << o.value
           << '|' << o.confidence_per_million << '|' << o.observed_tick << '|'
           << o.lifetime_ticks << '|' << record.sequence << '\n';
  }
  for (const auto &[id, remaining] : state.cooldowns)
    output << "cooldown=" << id << '|' << remaining << '\n';
  if (state.active_action) {
    const auto &a = *state.active_action;
    output << "active=" << a.action_id << '|' << a.started_tick << '|'
           << a.elapsed_ticks << '|' << a.remaining_ticks << '\n';
  } else
    output << "active=none\n";
  return output.str();
}

LivingRuntimeState
deserialize_living_runtime_state(const LivingRuntimeProgram &program,
                                 std::string_view source,
                                 std::uint64_t maximum_bytes) {
  if (source.empty() || source.size() > maximum_bytes)
    throw std::runtime_error(
        "runtime state input is empty or exceeds byte limit");
  const auto program_validation = validate_living_runtime_program(program);
  if (!program_validation.ok())
    throw std::invalid_argument(program_validation.diagnostics.front().code +
                                ": " +
                                program_validation.diagnostics.front().message);
  LivingRuntimeState state;
  std::map<std::string, bool> scalars;
  std::set<std::string> variables, cooldowns;
  std::set<std::uint64_t> sequences;
  std::istringstream stream{std::string(source)};
  std::string line;
  std::size_t lines = 0;
  while (std::getline(stream, line)) {
    if (++lines > 1'000'000)
      throw std::runtime_error("runtime state exceeds line limit");
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (line.empty())
      throw std::runtime_error("runtime state contains an empty line");
    const auto equals = line.find('=');
    if (equals == std::string::npos)
      throw std::runtime_error("runtime state line requires key=value");
    const auto key = line.substr(0, equals);
    const auto value = line.substr(equals + 1);
    if (key == "schema" || key == "program" || key == "tick" ||
        key == "next_sequence" || key == "energy" || key == "active") {
      if (!scalars.emplace(key, true).second)
        throw std::runtime_error("duplicate runtime state scalar: " + key);
    }
    if (key == "schema") {
      if (value != "gspl.living-runtime-state/0.1")
        throw std::runtime_error("unsupported runtime state schema");
    } else if (key == "program") {
      if (value != living_runtime_program_identity(program))
        throw std::runtime_error("runtime state program identity mismatch");
    } else if (key == "tick")
      state.tick = integer<std::uint64_t>(value, key);
    else if (key == "next_sequence")
      state.next_sequence = integer<std::uint64_t>(value, key);
    else if (key == "energy")
      state.energy = integer<std::uint32_t>(value, key);
    else if (key == "variable") {
      const auto parts = split(value, '|');
      if (parts.size() != 2 || !variables.insert(parts[0]).second)
        throw std::runtime_error(
            "runtime state variable is malformed or duplicate");
      state.variables.emplace(parts[0], integer<std::int32_t>(parts[1], key));
    } else if (key == "memory") {
      const auto parts = split(value, '|');
      if (parts.size() != 7)
        throw std::runtime_error("runtime state memory is malformed");
      const auto sequence = integer<std::uint64_t>(parts[6], key);
      if (!sequences.insert(sequence).second)
        throw std::runtime_error("runtime memory sequence is duplicate");
      state.memory.push_back(
          {{parts[0], parts[1], integer<std::int32_t>(parts[2], key),
            integer<std::uint32_t>(parts[3], key),
            integer<std::uint64_t>(parts[4], key),
            integer<std::uint32_t>(parts[5], key)},
           sequence});
    } else if (key == "cooldown") {
      const auto parts = split(value, '|');
      if (parts.size() != 2 || !cooldowns.insert(parts[0]).second)
        throw std::runtime_error("runtime cooldown is malformed or duplicate");
      state.cooldowns.emplace(parts[0], integer<std::uint32_t>(parts[1], key));
    } else if (key == "active") {
      if (value != "none") {
        const auto parts = split(value, '|');
        if (parts.size() != 4)
          throw std::runtime_error("runtime active action is malformed");
        state.active_action =
            ActiveRuntimeAction{parts[0], integer<std::uint64_t>(parts[1], key),
                                integer<std::uint32_t>(parts[2], key),
                                integer<std::uint32_t>(parts[3], key)};
      }
    } else
      throw std::runtime_error("unknown runtime state field: " + key);
  }
  static constexpr std::array required{"schema",        "program", "tick",
                                       "next_sequence", "energy",  "active"};
  for (const auto key : required)
    if (!scalars.contains(key))
      throw std::runtime_error("runtime state required field is missing: " +
                               std::string(key));
  const auto validation = validate_living_runtime_state(program, state);
  if (!validation.ok())
    throw std::runtime_error(validation.diagnostics.front().code + ": " +
                             validation.diagnostics.front().message);
  if (serialize_living_runtime_state(program, state) != source)
    throw std::runtime_error("runtime state is not canonical");
  return state;
}

RuntimeReplayResult replay_living_runtime(
    const LivingRuntimeProgram &program, LivingRuntimeState state,
    std::span<const RuntimeReplayFrame> frames, std::uint64_t end_tick,
    const RuntimeReplayLimits &limits) {
  if (limits.maximum_ticks == 0 || limits.maximum_events == 0 ||
      limits.maximum_frames == 0 || frames.size() > limits.maximum_frames ||
      end_tick < state.tick || end_tick - state.tick > limits.maximum_ticks)
    throw std::invalid_argument("runtime replay bounds are invalid");
  const auto state_validation = validate_living_runtime_state(program, state);
  if (!state_validation.ok())
    throw std::invalid_argument(state_validation.diagnostics.front().code +
                                ": " +
                                state_validation.diagnostics.front().message);
  std::uint64_t previous = 0;
  bool first = true;
  for (const auto &frame : frames) {
    if (frame.tick < state.tick || frame.tick >= end_tick ||
        (!first && frame.tick <= previous) || frame.variables.size() > 4096 ||
        frame.observations.size() > program.maximum_memory_records)
      throw std::invalid_argument(
          "runtime replay frame is unsorted, out of range, or unbounded");
    previous = frame.tick;
    first = false;
  }
  RuntimeReplayResult result;
  result.final_state = std::move(state);
  std::size_t frame_index = 0;
  while (result.final_state.tick < end_tick) {
    if (frame_index < frames.size() &&
        frames[frame_index].tick == result.final_state.tick) {
      const auto &frame = frames[frame_index++];
      std::set<std::string> updates;
      for (const auto &update : frame.variables) {
        if (!updates.insert(update.key).second)
          throw std::invalid_argument(
              "runtime replay frame contains duplicate variable updates");
        set_runtime_variable(result.final_state, update.key, update.value);
      }
      for (auto observation : frame.observations) {
        if (observation.observed_tick != frame.tick)
          throw std::invalid_argument(
              "runtime replay observation tick mismatch");
        observe(result.final_state, program, std::move(observation));
      }
      if (frame.interrupt_active_action) {
        std::vector<RuntimeEvent> interrupted;
        (void)interrupt_living_action(program, result.final_state, interrupted);
        append_events(result.events, interrupted, limits.maximum_events);
      }
    }
    const auto step = step_living_runtime(program, result.final_state);
    append_events(result.events, step.events, limits.maximum_events);
  }
  result.final_state_identity =
      sha256(serialize_living_runtime_state(program, result.final_state));
  return result;
}
} // namespace gspl::sprites
