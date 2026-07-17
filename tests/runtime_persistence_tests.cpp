#include "gspl_sprites/runtime_persistence.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
LivingRuntimeProgram program() {
  return {"replay.runtime",
          60,
          8,
          {{"react", 10, {{"perception.threat", Comparison::greater_equal, 1}}},
           {"idle", 0, {}}},
          {{"attack",
            "react",
            10,
            {{"perception.threat", 100000}},
            {{"confidence.threat", Comparison::greater_equal, 500000}},
            2,
            3,
            10,
            true,
            {{"release", 0}}},
           {"wait", "idle", 1, {}, {}, 1, 0, 0, true, {}}}};
}
} // namespace

int main() {
  try {
    const auto definition = program();
    const auto identity = living_runtime_program_identity(definition);
    check(identity.size() == 64 &&
              canonicalize_living_runtime_program(definition) ==
                  canonicalize_living_runtime_program(definition),
          "runtime program identity is invalid");
    LivingRuntimeState state;
    set_runtime_variable(state, "mood.courage", 700000);
    observe(state, definition, {"threat", "enemy.1", 2, 900000, 0, 4});
    (void)step_living_runtime(definition, state);
    const auto encoded = serialize_living_runtime_state(definition, state);
    const auto decoded = deserialize_living_runtime_state(definition, encoded);
    check(serialize_living_runtime_state(definition, decoded) == encoded,
          "runtime state roundtrip is not canonical");
    bool wrong_program = false;
    try {
      auto changed = definition;
      changed.actions[0].energy_cost = 11;
      (void)deserialize_living_runtime_state(changed, encoded);
    } catch (const std::runtime_error &) {
      wrong_program = true;
    }
    check(wrong_program, "runtime state loaded against another program");
    bool noncanonical = false;
    try {
      (void)deserialize_living_runtime_state(definition, encoded + "\n");
    } catch (const std::runtime_error &) {
      noncanonical = true;
    }
    check(noncanonical, "noncanonical runtime state accepted");
    bool duplicate_variable = false;
    try {
      const auto position = encoded.find("variable=");
      check(position != std::string::npos, "test state has no variable");
      const auto end = encoded.find('\n', position);
      const auto line = encoded.substr(position, end - position + 1);
      (void)deserialize_living_runtime_state(definition,
                                             encoded.substr(0, end + 1) + line +
                                                 encoded.substr(end + 1));
    } catch (const std::runtime_error &) {
      duplicate_variable = true;
    }
    check(duplicate_variable, "duplicate runtime variable accepted");
    LivingRuntimeState initial;
    const std::array frames{
        RuntimeReplayFrame{0,
                           {{"mood.courage", 800000}},
                           {{"threat", "enemy.1", 2, 900000, 0, 3}},
                           false},
        RuntimeReplayFrame{2, {}, {}, true}};
    const auto first = replay_living_runtime(definition, initial, frames, 5);
    const auto second = replay_living_runtime(definition, initial, frames, 5);
    check(first.final_state_identity == second.final_state_identity &&
              serialize_living_runtime_state(definition, first.final_state) ==
                  serialize_living_runtime_state(definition,
                                                 second.final_state) &&
              first.events.size() == second.events.size(),
          "runtime replay is nondeterministic");
    for (std::size_t i = 0; i < first.events.size(); ++i)
      check(first.events[i].tick == second.events[i].tick &&
                first.events[i].sequence == second.events[i].sequence &&
                first.events[i].kind == second.events[i].kind &&
                first.events[i].action_id == second.events[i].action_id &&
                first.events[i].marker_id == second.events[i].marker_id,
            "runtime replay event stream diverged");
    bool unsorted = false;
    try {
      const std::array bad{RuntimeReplayFrame{1, {}, {}, false},
                           RuntimeReplayFrame{1, {}, {}, false}};
      (void)replay_living_runtime(definition, initial, bad, 3);
    } catch (const std::invalid_argument &) {
      unsorted = true;
    }
    check(unsorted, "unsorted replay frames accepted");
    bool bounded = false;
    try {
      RuntimeReplayLimits limits;
      limits.maximum_ticks = 4;
      (void)replay_living_runtime(definition, initial, frames, 5, limits);
    } catch (const std::invalid_argument &) {
      bounded = true;
    }
    check(bounded, "replay tick limit was not enforced");
    std::cout << "all gspl sprites runtime persistence tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
