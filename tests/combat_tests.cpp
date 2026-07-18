#include "gspl_sprites/combat.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template <class F> bool rejects(F f) { try { f(); } catch (const std::exception &) { return true; } return false; }
CombatProgram program() {
  return {"voltfox.combat", 8, 2,
          {{"arc.bite", CombatTargetRule::enemy, 20, 3, 1500,
            {{CombatEffectKind::damage, {}, 35, 0},
             {CombatEffectKind::status, "charged", 2, 4}}},
           {"recover", CombatTargetRule::self, 0, 0, 0,
            {{CombatEffectKind::healing, {}, 10, 0}}}}};
}
CombatState state() {
  CombatState value;
  value.actors.emplace("fox", CombatActorState{"fox", "heroes", 80, 100, 50, 100, 0, 0});
  value.actors.emplace("drone", CombatActorState{"drone", "enemy", 30, 30, 0, 0, 1000, 0});
  return value;
}
} // namespace
int main() try {
  const auto definition = program();
  auto value = state();
  check(validate_combat_program(definition).ok() && validate_combat_state(definition, value).ok(),
        "valid combat model rejected");
  const auto events = execute_combat_command(definition, value, {0, "fox", "drone", "arc.bite"});
  check(events.size() == 2 && events[0].applied_magnitude == 30 &&
            value.actors.at("drone").health == 0 && value.actors.at("fox").resource == 30 &&
            value.actors.at("drone").statuses.contains("charged"),
        "typed combat effects were not applied deterministically");
  const auto committed = value;
  check(rejects([&] { (void)execute_combat_command(definition, value, {0, "fox", "drone", "arc.bite"}); }) &&
            value.next_sequence == committed.next_sequence,
        "failed combat command partially mutated state");
  advance_combat_to(definition, value, 4);
  check(!value.actors.at("fox").cooldown_expiry.contains("arc.bite") &&
            !value.actors.at("drone").statuses.contains("charged"),
        "combat timers did not expire deterministically");
  auto healing = state();
  const auto healed = execute_combat_command(definition, healing, {0, "fox", "fox", "recover"});
  check(healed.size() == 1 && healing.actors.at("fox").health == 90,
        "self-target healing failed");
  auto distant = state(); distant.actors.at("drone").x_millimeters = 2000;
  check(rejects([&] { (void)execute_combat_command(definition, distant, {0, "fox", "drone", "arc.bite"}); }),
        "out-of-range combat target accepted");
  auto invalid = definition; invalid.abilities[0].effects[0].magnitude = 0;
  check(!validate_combat_program(invalid).ok(), "zero-magnitude combat effect accepted");
  std::cout << "all gspl sprites combat tests passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
