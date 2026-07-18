#include "gspl_sprites/combo.hpp"
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template <class F> bool rejects(F f) { try { f(); } catch (const std::exception &) { return true; } return false; }
CombatProgram combat() {
  return {"fighter.combat", 4, 4,
          {{"jab", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 5, 0}}},
           {"cross", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 10, 0}}},
           {"uppercut", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 20, 0}}}}};
}
ComboProgram combo() { return {"fighter.combo", 8, {"jab"}, {{"jab", "cross", 2, 4, true}, {"cross", "uppercut", 1, 3, false}}}; }
CombatState state() {
  CombatState value; value.actors.emplace("a", CombatActorState{"a", "one", 100, 100, 0, 0, 0, 0});
  value.actors.emplace("b", CombatActorState{"b", "two", 100, 100, 0, 0, 1000, 0}); return value;
}
}
int main() try {
  const auto combat_program = combat(); const auto combo_program = combo();
  check(validate_combo_program(combo_program, combat_program).ok(), "valid combo rejected");
  ComboState combo_state; auto combat_state = state();
  auto first = execute_combo_combat_command(combo_program, combat_program, combo_state, combat_state, {0, "a", "b", "jab"});
  check(first.entered_combo && combat_state.actors.at("b").health == 95, "combo entry failed");
  const auto before = combat_state;
  check(rejects([&] { (void)execute_combo_combat_command(combo_program, combat_program, combo_state, combat_state, {2, "a", "b", "cross"}); }) &&
            combat_state.actors.at("b").health == before.actors.at("b").health,
        "unconfirmed transition mutated combat state");
  auto second = execute_combo_combat_command(combo_program, combat_program, combo_state, combat_state, {2, "a", "b", "cross"}, true);
  auto third = execute_combo_combat_command(combo_program, combat_program, combo_state, combat_state, {3, "a", "b", "uppercut"});
  check(second.transitioned && third.transitioned && combat_state.actors.at("b").health == 65 && combo_state.history.size() == 3,
        "branching combo did not execute");
  auto cyclic = combo_program; cyclic.transitions.push_back({"uppercut", "jab", 1, 2, false});
  check(!validate_combo_program(cyclic, combat_program).ok(), "infinite combo chain accepted");
  auto forged = combo_state;
  forged.history[1].started_tick = 99;
  check(!validate_combo_state(combo_program, combat_program, forged).ok(),
        "forged combo history outside its cancel window accepted");
  auto late_combo = ComboState{}; auto late_combat = state();
  (void)execute_combo_combat_command(combo_program, combat_program, late_combo, late_combat, {0, "a", "b", "jab"});
  check(rejects([&] { (void)execute_combo_combat_command(combo_program, combat_program, late_combo, late_combat, {5, "a", "b", "cross"}, true); }),
        "transition outside cancel window accepted");
  end_combo(combo_program, combat_program, combo_state);
  check(combo_state.history.empty() && !combo_state.active_ability, "combo did not end cleanly");
  std::cout << "all gspl sprites combo tests passed\n"; return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
