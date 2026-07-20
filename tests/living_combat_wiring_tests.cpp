#include "gspl_sprites/combat.hpp"
#include "gspl_sprites/runtime_persistence.hpp"
#include "gspl_sprites/transformation.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template <class F> bool rejects(F f) { try { f(); } catch (const std::exception &) { return true; } return false; }
CombatProgram combat() { return {"wiring.combat", 4, 4,
  {{"bite", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 5, 0}}},
   {"heal", CombatTargetRule::self, 0, 0, 0, {{CombatEffectKind::healing, {}, 10, 0}}},
   {"burn", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::status, "burning", 3, 5}}},
   {"directional-lightning", CombatTargetRule::enemy, 0, 0, 4000, {{CombatEffectKind::damage, {}, 20, 0}}}}}; }
TransformationProgram forms() { return {"wiring.forms", "base", 8,
  {{"base", 0, 0, {"bite", "heal"}},
   {"attack", 0, 0, {"bite", "directional-lightning"}},
   {"special", 0, 0, {"bite", "directional-lightning"}}},
  {{"to_attack", "base", "attack", 10, 3, true},
   {"to_special", "base", "special", 10, 3, true},
   {"to_base", "attack", "base", 0, 2, true}}}; }
} // namespace

int main() try {
  const auto cp = combat();
  const auto tp = forms();

  {
    CombatEvent damage_dealt{0, 1, "fox", "drone", "bite", CombatEffectKind::damage, {}, 5};
    const auto obs = translate_combat_event_to_perception(damage_dealt, "fox", 0);
    check(obs.key == "damage_dealt" && obs.value == 5 && obs.confidence_per_million == 1'000'000 &&
          obs.lifetime_ticks == 120, "damage_dealt perception mapping failed");
  }
  {
    CombatEvent damage_taken{0, 1, "drone", "fox", "bite", CombatEffectKind::damage, {}, 5};
    const auto obs = translate_combat_event_to_perception(damage_taken, "fox", 0);
    check(obs.key == "damage_taken" && obs.value == 5, "damage_taken perception mapping failed");
  }
  {
    CombatEvent healing{0, 2, "fox", "fox", "heal", CombatEffectKind::healing, {}, 10};
    const auto received = translate_combat_event_to_perception(healing, "fox", 0);
    check(received.key == "healing_received", "healing_received perception mapping failed");
    const auto given = translate_combat_event_to_perception(healing, "source", 0);
    check(given.key == "healing_given", "healing_given perception mapping failed");
  }
  {
    CombatEvent status{0, 3, "fox", "drone", "burn", CombatEffectKind::status, "burning", 3};
    const auto obs = translate_combat_event_to_perception(status, "drone", 0);
    check(obs.key == "status_applied_burning" && obs.value == 3, "status perception mapping failed");
  }
  {
    check(can_use_ability_in_form("bite", "base", tp), "base form should have bite");
    check(!can_use_ability_in_form("directional-lightning", "base", tp), "base form should not have directional-lightning");
    check(can_use_ability_in_form("directional-lightning", "attack", tp), "attack form should have directional-lightning");
    check(can_use_ability_in_form("directional-lightning", "special", tp), "special form should have directional-lightning");
    check(!can_use_ability_in_form("nonexistent", "base", tp), "missing ability should return false");
    check(!can_use_ability_in_form("bite", "nonexistent", tp), "missing form should return false");
  }
  {
    CombatState cs;
    cs.actors.emplace("fox", CombatActorState{"fox", "hero", 100, 100, 0, 0, 0, 0, {}, {}});
    cs.actors.emplace("drone", CombatActorState{"drone", "enemy", 30, 30, 0, 0, 1000, 0, {}, {}});
    CombatCommand cmd{0, "fox", "drone", "directional-lightning"};
    check(rejects([&] { (void)execute_combat_command(cp, cs, cmd, "base", tp); }),
          "directional-lightning should be rejected in base form");
    const auto events = execute_combat_command(cp, cs, cmd, "attack", tp);
    check(events.size() == 1 && events[0].applied_magnitude == 20,
          "directional-lightning should execute in attack form");
  }
  {
    const auto vfx = trigger_transformation_vfx(tp, "to_attack", 42);
    check(vfx.transformation_id == "to_attack" && vfx.from_form == "base" && vfx.to_form == "attack" &&
          vfx.tick == 42 && vfx.duration_ticks == 15, "transformation VFX fields are incorrect");
  }
  {
    check(rejects([&] { (void)trigger_transformation_vfx(tp, "nonexistent", 0); }),
          "missing transformation should throw");
  }
  {
    CombatState cs;
    cs.actors.emplace("fox", CombatActorState{"fox", "hero", 75, 100, 0, 0, 0, 0,
      {{"bite", 10}}, {{"burning", CombatStatusState{3, 100}}}});
    TransformationState ts{"fox", "attack", 50, 50, std::nullopt, {}};
    LivingRuntimeState rs;
    const auto header = capture_persistence_header(cs, ts, rs);
    check(header.entity_id == "fox" && header.form_id == "attack" && header.health == 75 &&
          header.maximum_health == 100 && header.active_statuses.size() == 1 &&
          header.active_statuses[0] == "burning" && header.cooldown_ids.size() == 1 &&
          header.cooldown_ids[0] == "bite", "capture_persistence_header failed");
    CombatState restored_cs;
    TransformationState restored_ts;
    LivingRuntimeState restored_rs;
    restore_from_persistence_header(header, restored_cs, restored_ts, restored_rs);
    check(restored_ts.stable_entity_id == "fox" && restored_ts.current_form == "attack" &&
          restored_cs.actors["fox"].health == 75 && restored_cs.actors["fox"].maximum_health == 100 &&
          restored_cs.actors["fox"].statuses.contains("burning") &&
          restored_cs.actors["fox"].cooldown_expiry.contains("bite"),
          "restore_from_persistence_header failed");
  }
  std::cout << "all gspl sprites living combat wiring tests passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
