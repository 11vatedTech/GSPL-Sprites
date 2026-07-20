#include "gspl_sprites/transformation.hpp"
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template <class F> bool rejects(F f) { try { f(); } catch (const std::exception &) { return true; } return false; }
CombatProgram combat() { return {"forms.combat", 4, 4,
  {{"bite", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 5, 0}}},
   {"lightning", CombatTargetRule::enemy, 0, 0, 4000, {{CombatEffectKind::damage, {}, 20, 0}}}}}; }
TransformationProgram transformations() { return {"fox.forms", "base", 8,
  {{"base", 0, 0, {"bite"}}, {"charged", 50, 30, {"bite", "lightning"}}},
  {{"charge", "base", "charged", 20, 3, true}, {"discharge", "charged", "base", 0, 2, true}}}; }
CombatState combat_state() { CombatState value;
  value.actors.emplace("fox", CombatActorState{"fox", "hero", 100, 100, 20, 20, 0, 0, {}, {}});
  value.actors.emplace("enemy", CombatActorState{"enemy", "enemy", 100, 100, 0, 0, 1000, 0, {}, {}}); return value; }
}
int main() try {
  const auto cp = combat(); const auto tp = transformations();
  TransformationState state{"fox", "base", 50, 50, std::nullopt, {}}; auto combat_value = combat_state();
  check(validate_transformation_program(tp, cp).ok() && validate_transformation_state(tp, cp, state).ok(), "valid transformation rejected");
  check(rejects([&] { (void)execute_transformed_combat_command(tp, cp, state, combat_value, {0, "fox", "enemy", "lightning"}); }), "form-locked ability accepted");
  begin_transformation(tp, cp, state, "charge", 1);
  check(state.energy == 30 && state.active.has_value(), "transformation did not begin");
  advance_transformation_to(tp, cp, state, combat_value, 3);
  check(state.current_form == "base", "transformation completed early");
  advance_transformation_to(tp, cp, state, combat_value, 4);
  check(state.current_form == "charged" && combat_value.actors.at("fox").maximum_health == 150 &&
        combat_value.actors.at("fox").maximum_resource == 50, "form deltas were not applied");
  const auto events = execute_transformed_combat_command(tp, cp, state, combat_value, {4, "fox", "enemy", "lightning"});
  check(events.size() == 1 && combat_value.actors.at("enemy").health == 80, "form ability did not execute");
  begin_transformation(tp, cp, state, "discharge", 5); advance_transformation_to(tp, cp, state, combat_value, 7);
  check(state.current_form == "base" && combat_value.actors.at("fox").maximum_health == 100 && state.history.size() == 2,
        "reversible transformation failed");
  auto missing_reverse = tp; missing_reverse.transformations.pop_back();
  check(!validate_transformation_program(missing_reverse, cp).ok(), "missing explicit reverse accepted");
  auto forged = state; forged.current_form = "charged";
  check(!validate_transformation_state(tp, cp, forged).ok(), "identity/form history mismatch accepted");
  std::cout << "all gspl sprites transformation tests passed\n"; return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
