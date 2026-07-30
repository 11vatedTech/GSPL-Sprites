#include "gspl_sprites/living_runtime.hpp"
#include "gspl_sprites/runtime_persistence.hpp"
#include "gspl_sprites/core.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
LivingRuntimeProgram program() {
  return {
      "voltfox.runtime",
      60,
      2,
      {{"defend", 100, {{"perception.threat", Comparison::greater_equal, 1}}},
       {"idle", 0, {}}},
      {{"attack",
        "defend",
        10,
        {{"perception.threat", 500000}},
        {{"confidence.threat", Comparison::greater_equal, 500000}},
        2,
        3,
        20,
        true,
        {{"release", 0}}},
       {"observe", "idle", 1, {}, {}, 1, 0, 0, true, {}}}};
}

void test_persistence_and_replay() {
  // Test save/restore round-trip and replay against persisted state
  auto def = program();
  LivingRuntimeState initial, state;
  (void)step_living_runtime(def, state); // tick 1, starts observe
  (void)step_living_runtime(def, state); // tick 2, completes observe, re-selects

  // Serialize the state
  std::string serialized = serialize_living_runtime_state(def, state);
  check(!serialized.empty(), "serialized state is empty");
  check(serialized.find("tick=") != std::string::npos,
        "serialized state missing tick");
  check(serialized.find("active=") != std::string::npos,
        "serialized state missing active");

  // Deserialize into a new state
  auto restored = deserialize_living_runtime_state(def, serialized, 65536);
  check(state.tick == restored.tick,
        "tick mismatch after save/restore round-trip");
  check(state.energy == restored.energy,
        "energy mismatch after save/restore round-trip");
  check(state.next_sequence == restored.next_sequence,
        "next_sequence mismatch after save/restore round-trip");

  // Re-serialize and check canonical form
  std::string reserialized = serialize_living_runtime_state(def, restored);
  check(serialized == reserialized,
        "save/restore round-trip is not canonical");

  // Build replay frames from the initial state up to tick 5
  LivingRuntimeState replay_state = initial;
  std::vector<RuntimeReplayFrame> frames;
  for (std::uint64_t t = 0; t < 5; ++t) {
    RuntimeReplayFrame frame;
    frame.tick = t;
    frame.variables = {};
    frame.observations = {};
    frame.interrupt_active_action = false;
    frames.push_back(std::move(frame));
  }

  RuntimeReplayLimits limits;
  limits.maximum_ticks = 100;
  limits.maximum_events = 1000;
  limits.maximum_frames = 100;
  auto replay_result = replay_living_runtime(def, initial, frames, 5, limits);
  check(replay_result.final_state.tick == 5,
        "replay did not reach tick 5");
  check(!replay_result.events.empty(),
        "replay produced no events");

  // Run a fresh live execution to tick 5 and compare final state hashes
  LivingRuntimeState live_state = initial;
  for (std::uint64_t t = 0; t < 5; ++t)
    (void)step_living_runtime(def, live_state);
  std::string live_identity = sha256(serialize_living_runtime_state(def, live_state));
  check(live_identity == replay_result.final_state_identity,
        "replay final state does not match live execution");
  check(std::all_of(replay_result.events.begin(), replay_result.events.end(),
                    [&](const RuntimeEvent& e) { return e.tick < 5; }),
        "replay event has tick >= end_tick");
}

} // namespace

int main() {
  try {
    const auto definition = program();
    check(validate_living_runtime_program(definition).ok(),
          "valid living runtime rejected");
    LivingRuntimeState state;
    auto first = step_living_runtime(definition, state);
    check(first.selected_action == "observe" && state.tick == 1 &&
              state.active_action->action_id == "observe",
          "idle behavior selection failed");
    auto second = step_living_runtime(definition, state);
    check(second.events.size() == 2 &&
              second.events[0].kind == RuntimeEventKind::action_completed &&
              second.selected_action == "observe",
          "action completion or deterministic reselection failed");
    std::vector<RuntimeEvent> interrupted;
    check(interrupt_living_action(definition, state, interrupted) &&
              interrupted.size() == 1 &&
              interrupted[0].kind == RuntimeEventKind::action_interrupted,
          "interruptible action did not stop");
    observe(state, definition, {"threat", "enemy.1", 2, 900000, state.tick, 2});
    auto attack = step_living_runtime(definition, state);
    check(attack.selected_action == "attack" && state.energy == 80,
          "perception-driven utility action not selected");
    auto executing = step_living_runtime(definition, state);
    check(executing.events.size() == 1 &&
              executing.events[0].kind == RuntimeEventKind::action_marker &&
              executing.events[0].marker_id == "release",
          "action marker did not fire deterministically");
    std::vector<RuntimeEvent> attack_interruption;
    check(interrupt_living_action(definition, state, attack_interruption),
          "attack interruption failed");
    auto cooldown = step_living_runtime(definition, state);
    check(cooldown.selected_action == "observe", "cooldown admission failed");
    observe(state, definition, {"threat", "enemy.2", 3, 800000, state.tick, 1});
    observe(state, definition, {"sound", "enemy.3", 1, 700000, state.tick, 1});
    observe(state, definition, {"light", "world", 1, 600000, state.tick, 1});
    check(state.memory.size() == 2, "bounded memory eviction failed");
    (void)step_living_runtime(definition, state);
    (void)step_living_runtime(definition, state);
    check(state.memory.empty() &&
              !state.variables.contains("perception.light") &&
              !state.variables.contains("confidence.light"),
          "expired perceptions remained in memory or blackboard");
    auto invalid = definition;
    invalid.actions[0].goal_id = "absent";
    check(!validate_living_runtime_program(invalid).ok(),
          "action with absent goal accepted");
    auto duplicate = definition;
    duplicate.actions.push_back(duplicate.actions[0]);
    check(!validate_living_runtime_program(duplicate).ok(),
          "duplicate action accepted");
    auto invalid_state = state;
    invalid_state.energy = 1'000'001;
    bool state_rejected = false;
    try {
      (void)step_living_runtime(definition, invalid_state);
    } catch (const std::invalid_argument &) {
      state_rejected = true;
    }
    check(state_rejected, "invalid runtime snapshot accepted");

    // Run persistence and replay test
    test_persistence_and_replay();

    std::cout << "all gspl sprites living runtime tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
