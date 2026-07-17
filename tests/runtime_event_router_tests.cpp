#include "gspl_sprites/runtime_event_router.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace

int main() try {
  LivingRuntimeProgram runtime{
      "combat.runtime",
      60,
      8,
      {{"combat", 1, {}}},
      {{"slash", "combat", 1, {}, {}, 3, 2, 5, true, {{"impact", 1}}}}};
  RuntimeEventRoutingProgram routing{
      "combat.preview",
      {{RuntimeEventKind::action_started, "slash", "",
        RuntimeConsumerKind::animation, "animation.slash", 0},
       {RuntimeEventKind::action_marker, "slash", "impact",
        RuntimeConsumerKind::combat, "ability.slash.hit", 25},
       {RuntimeEventKind::action_marker, "slash", "impact",
        RuntimeConsumerKind::effect, "effect.slash.arc", 0},
       {RuntimeEventKind::action_marker, "slash", "impact",
        RuntimeConsumerKind::audio, "audio.slash.impact", 0}}};
  check(validate_runtime_event_routing_program(routing, runtime).ok(),
        "valid routing rejected");
  const std::array events{
      RuntimeEvent{4, 0, RuntimeEventKind::action_started, "slash", ""},
      RuntimeEvent{5, 1, RuntimeEventKind::action_marker, "slash", "impact"}};
  RuntimeEventRouterState state;
  const auto commands = route_runtime_events(routing, runtime, state, events);
  check(commands.size() == 4 &&
            commands[0].consumer == RuntimeConsumerKind::animation &&
            commands[1].consumer == RuntimeConsumerKind::combat &&
            commands[1].value == 25 && state.next_event_sequence == 2,
        "routing output is incorrect");
  auto reordered = routing;
  std::reverse(reordered.bindings.begin(), reordered.bindings.end());
  check(canonicalize_runtime_event_routing_program(routing, runtime) ==
            canonicalize_runtime_event_routing_program(reordered, runtime),
        "canonical routing depends on input order");
  bool gap = false;
  try {
    RuntimeEventRouterState cursor;
    const std::array bad{events[1]};
    (void)route_runtime_events(routing, runtime, cursor, bad);
  } catch (const std::invalid_argument &) {
    gap = true;
  }
  check(gap, "event gap accepted");
  RuntimeEventRouterState transactional;
  bool limited = false;
  try {
    (void)route_runtime_events(routing, runtime, transactional, events, 1);
  } catch (const std::runtime_error &) {
    limited = true;
  }
  check(limited && transactional.next_event_sequence == 0 &&
            transactional.next_command_sequence == 0,
        "failed routing batch advanced its cursor");
  auto unknown = routing;
  unknown.bindings[1].marker_id = "missing";
  check(!validate_runtime_event_routing_program(unknown, runtime).ok(),
        "unknown marker accepted");
  auto crossed = routing;
  crossed.bindings[0].operation_id = "audio.slash";
  check(!validate_runtime_event_routing_program(crossed, runtime).ok(),
        "cross-consumer operation accepted");
  std::cout << "all gspl sprites runtime event router tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
