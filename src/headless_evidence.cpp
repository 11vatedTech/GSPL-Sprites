#include "gspl_sprites/headless_evidence.hpp"

#include <sstream>
#include <stdexcept>

namespace gspl::sprites {

namespace {

std::string kind_str(EvidenceEventKind kind) {
  switch (kind) {
  case EvidenceEventKind::spawn: return "spawn";
  case EvidenceEventKind::form_change: return "form_change";
  case EvidenceEventKind::ability_use: return "ability_use";
  case EvidenceEventKind::damage_taken: return "damage_taken";
  case EvidenceEventKind::status_applied: return "status_applied";
  case EvidenceEventKind::despawn: return "despawn";
  }
  throw std::logic_error("unreachable EvidenceEventKind");
}

std::string escape_json(std::string_view value) {
  std::string result;
  result.reserve(value.size());
  for (auto ch : value) {
    if (ch == '"' || ch == '\\')
      result.push_back('\\');
    result.push_back(ch);
  }
  return result;
}

} // namespace

EvidenceTrace run_headless_evidence(const SpriteIr& ir) {
  {
    ResourceLimits rl;
    const auto src = ir.abilities.size() + ir.storm_abilities.size() + ir.form_definitions.size() +
                     ir.transformation_deltas.size() + ir.clips.size() + ir.collision_shapes.size() +
                     ir.collision_windows.size() + ir.projectiles.size() + ir.animation_intents.size();
    if (src > rl.max_sprite_ir_nodes) {
      throw std::runtime_error("RESOURCE_SPRITE_IR_NODES: SpriteIR node count " +
                               std::to_string(src) + " exceeds maximum " +
                               std::to_string(rl.max_sprite_ir_nodes));
    }
  }
  EvidenceTrace trace;
  trace.entity_id = ir.entity_id;
  trace.seed = 42;

  CombatProgram combat;
  combat.id = "headless.evidence.combat";
  combat.maximum_actors = 8;
  combat.maximum_statuses_per_actor = 4;
  combat.abilities = {
    {"directional-lightning", CombatTargetRule::enemy, 8, 120, 5000,
     {{CombatEffectKind::damage, {}, 25, 0}}},
    {"take_damage", CombatTargetRule::enemy, 0, 0, 5000,
     {{CombatEffectKind::damage, {}, 80, 0}}}
  };

  CombatState cstate;
  cstate.tick = 0;
  cstate.actors.emplace("voltfox", CombatActorState{"voltfox", "heroes", 100, 100, 100, 100, 0, 0, {}, {}});
  cstate.actors.emplace("enemy", CombatActorState{"enemy", "enemies", 100, 100, 100, 100, 1000, 0, {}, {}});

  TransformationProgram tprog;
  tprog.id = "headless.evidence.forms";
  tprog.base_form = "Idle";
  tprog.maximum_history = 64;
  tprog.forms = {
    {"Idle", 0, 0, {}},
    {"Running", 0, 0, {}},
    {"Attack", 0, 0, {"directional-lightning"}},
    {"Special", 0, 0, {"directional-lightning"}},
    {"Hurt", 0, 0, {}}
  };
  tprog.transformations = {
    {"idle_to_running", "Idle", "Running", 0, 1, false},
    {"running_to_hurt", "Running", "Hurt", 0, 1, false},
    {"running_to_idle", "Running", "Idle", 0, 1, false},
    {"hurt_to_idle", "Hurt", "Idle", 0, 1, false},
    {"idle_to_attack", "Idle", "Attack", 0, 1, false},
    {"attack_to_special", "Attack", "Special", 0, 1, false},
    {"special_to_idle", "Special", "Idle", 0, 1, false}
  };

  TransformationState tstate;
  tstate.stable_entity_id = ir.entity_id;
  tstate.current_form = "Idle";
  tstate.energy = 100;
  tstate.maximum_energy = 100;

  LivingRuntimeProgram rprog;
  rprog.id = "headless.evidence.runtime";
  rprog.ticks_per_second = 60;
  rprog.maximum_memory_records = 1024;
  rprog.goals = {
    {"survive", -10, {}},
    {"aggress", 100, {{"health_percent", Comparison::greater, 25}}}
  };
  rprog.actions = {
    {"idle_action", "survive", 0, {}, {}, 1, 0, 0, true, {}},
    {"attack_action", "aggress", 50, {}, {}, 1, 0, 0, true, {}}
  };

  LivingRuntimeState rstate;

  auto current_health = [&]() -> std::uint32_t {
    return cstate.actors.at("voltfox").health;
  };

  auto emit_event = [&](std::uint64_t tick, EvidenceEventKind kind,
                        std::string form_before, std::string form_after,
                        std::uint32_t health, std::string status_id,
                        std::string ability_name) {
    EvidenceEvent ev;
    ev.tick = tick;
    ev.kind = kind;
    ev.form_before = std::move(form_before);
    ev.form_after = std::move(form_after);
    ev.health = health;
    ev.status_id = std::move(status_id);
    ev.ability_name = std::move(ability_name);
    trace.events.push_back(std::move(ev));
  };

  auto form_change = [&](std::uint64_t tick, std::string to_form) {
    auto before = tstate.current_form;
    tstate.current_form = to_form;
    emit_event(tick, EvidenceEventKind::form_change,
               std::move(before), std::move(to_form), current_health(), "", "");
  };

  emit_event(0, EvidenceEventKind::spawn, "", "Idle", 100, "", "");

  advance_combat_to(combat, cstate, 10);
  form_change(10, "Running");

  advance_combat_to(combat, cstate, 30);
  {
    CombatCommand cmd{30, "enemy", "voltfox", "take_damage"};
    auto events = execute_combat_command(combat, cstate, cmd);
    emit_event(30, EvidenceEventKind::ability_use, "", "", current_health(), "", "take_damage");
    for (const auto& ev : events) {
      if (ev.effect == CombatEffectKind::damage) {
        emit_event(30, EvidenceEventKind::damage_taken, "", "", current_health(), "", "");
      }
    }
  }

  advance_combat_to(combat, cstate, 35);
  form_change(35, "Hurt");

  advance_combat_to(combat, cstate, 60);
  cstate.actors.at("voltfox").health = 80;
  form_change(60, "Idle");

  advance_combat_to(combat, cstate, 80);
  form_change(80, "Attack");

  advance_combat_to(combat, cstate, 90);
  {
    CombatCommand cmd{90, "voltfox", "enemy", "directional-lightning"};
    auto events = execute_combat_command(combat, cstate, cmd, tstate.current_form, tprog);
    emit_event(90, EvidenceEventKind::ability_use, "", "", current_health(), "", "directional-lightning");
    static_cast<void>(events);
  }

  advance_combat_to(combat, cstate, 100);
  form_change(100, "Special");

  advance_combat_to(combat, cstate, 120);
  form_change(120, "Idle");

  advance_combat_to(combat, cstate, 150);
  emit_event(150, EvidenceEventKind::despawn, "", "", current_health(), "", "");

  trace.total_ticks = 150;
  {
    ResourceLimits rl;
    if (trace.events.size() > rl.max_runtime_event_count) {
      throw std::runtime_error("RESOURCE_RUNTIME_EVENT_COUNT: evidence trace events " +
                               std::to_string(trace.events.size()) + " exceeds maximum " +
                               std::to_string(rl.max_runtime_event_count));
    }
  }
  return trace;
}

std::string write_trace_json(const EvidenceTrace& trace) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"_schema_version\": \"gspl_evidence_v1\",\n";
  out << "  \"entity_id\": \"" << escape_json(trace.entity_id) << "\",\n";
  out << "  \"seed\": " << trace.seed << ",\n";
  out << "  \"total_ticks\": " << trace.total_ticks << ",\n";
  out << "  \"events\": [\n";
  for (std::size_t i = 0; i < trace.events.size(); ++i) {
    const auto& ev = trace.events[i];
    out << "    {\"tick\": " << ev.tick
        << ", \"kind\": \"" << kind_str(ev.kind)
        << "\", \"form_before\": \"" << escape_json(ev.form_before)
        << "\", \"form_after\": \"" << escape_json(ev.form_after)
        << "\", \"health\": " << ev.health
        << ", \"status_id\": \"" << escape_json(ev.status_id)
        << "\", \"ability_name\": \"" << escape_json(ev.ability_name)
        << "\"}";
    if (i + 1 < trace.events.size())
      out << ",";
    out << "\n";
  }
  out << "  ]\n";
  out << "}";
  return out.str();
}

} // namespace gspl::sprites
