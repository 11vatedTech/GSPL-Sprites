#include "gspl_sprites/runtime_consumers.hpp"

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
  const std::array commands{
      RuntimeConsumerCommand{0, 10, 5, RuntimeConsumerKind::animation,
                             "animation.slash", 0},
      RuntimeConsumerCommand{1, 11, 5, RuntimeConsumerKind::combat,
                             "ability.slash.hit", 25},
      RuntimeConsumerCommand{2, 11, 5, RuntimeConsumerKind::effect,
                             "effect.slash.arc", 0},
      RuntimeConsumerCommand{3, 11, 5, RuntimeConsumerKind::audio,
                             "audio.slash.impact", 0}};
  RuntimePreviewState state;
  apply_runtime_consumer_commands(state, commands);
  check(state.animation_operation == "animation.slash" &&
            state.combat.size() == 1 && state.effects.size() == 1 &&
            state.audio.size() == 1 && state.next_command_sequence == 4,
        "consumer state is incorrect");
  const auto prior = state;
  RuntimePreviewLimits limits;
  limits.maximum_pending_audio = 1;
  const std::array overflow{RuntimeConsumerCommand{
      4, 12, 6, RuntimeConsumerKind::audio, "audio.second", 0}};
  bool bounded = false;
  try {
    apply_runtime_consumer_commands(state, overflow, limits);
  } catch (const std::runtime_error &) {
    bounded = true;
  }
  check(bounded && state.next_command_sequence == prior.next_command_sequence &&
            state.audio.size() == prior.audio.size(),
        "failed consumer batch mutated state");
  bool duplicate = false;
  try {
    const std::array repeated{commands[3]};
    apply_runtime_consumer_commands(state, repeated);
  } catch (const std::invalid_argument &) {
    duplicate = true;
  }
  check(duplicate, "duplicate command accepted");
  std::cout << "all gspl sprites runtime consumer tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
