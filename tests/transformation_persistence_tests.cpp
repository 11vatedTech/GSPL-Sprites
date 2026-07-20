#include "gspl_sprites/transformation_persistence.hpp"
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template <class F> bool rejects(F f) { try { f(); } catch (const std::exception &) { return true; } return false; }
CombatProgram combat() { return {"persist.combat", 2, 2, {{"pulse", CombatTargetRule::enemy, 0, 0, 10, {{CombatEffectKind::damage, {}, 1, 0}}}}}; }
TransformationProgram program() { return {"persist.forms", "base", 4, {{"base", 0, 0, {"pulse"}}, {"power", 10, 0, {"pulse"}}}, {{"up", "base", "power", 1, 2, true}, {"down", "power", "base", 0, 1, true}}}; }
}
int main() try {
  const auto cp = combat(); const auto tp = program(); TransformationState state{"entity", "base", 5, 5, std::nullopt, {}};
  begin_transformation(tp, cp, state, "up", 7);
  const auto encoded = serialize_transformation_state(tp, cp, state);
  const auto decoded = deserialize_transformation_state(tp, cp, encoded);
  check(serialize_transformation_state(tp, cp, decoded) == encoded && transformation_state_identity(tp, cp, decoded).size() == 64,
        "transformation state did not round-trip canonically");
  auto changed_combat = cp; changed_combat.abilities[0].effects[0].magnitude = 2;
  check(rejects([&] { (void)deserialize_transformation_state(tp, changed_combat, encoded); }), "changed combat program accepted persisted form state");
  auto changed_program = tp; changed_program.transformations[0].duration_ticks = 3;
  check(rejects([&] { (void)deserialize_transformation_state(changed_program, cp, encoded); }), "changed transformation program accepted persisted state");
  auto corrupt = encoded; const auto position = corrupt.find("energy=4"); check(position != std::string::npos, "fixture energy absent"); corrupt.replace(position, 8, "energy=3");
  check(rejects([&] { (void)deserialize_transformation_state(tp, cp, corrupt); }) == false,
        "valid canonical energy change should parse");
  check(rejects([&] { (void)deserialize_transformation_state(tp, cp, encoded + "\n"); }), "noncanonical trailing line accepted");
  check(rejects([&] { (void)deserialize_transformation_state(tp, cp, encoded, 8); }), "byte limit ignored");
  std::cout << "all gspl sprites transformation persistence tests passed\n"; return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
