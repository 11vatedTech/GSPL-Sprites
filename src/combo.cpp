#include "gspl_sprites/combo.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
bool id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char c) {
           return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
         });
}
bool has_ability(const CombatProgram &program, std::string_view value) {
  return std::ranges::find(program.abilities, value,
                           &CombatAbilityDefinition::id) != program.abilities.end();
}
const ComboTransition *transition(const ComboProgram &program,
                                  std::string_view from, std::string_view to) {
  const auto found = std::ranges::find_if(program.transitions, [&](const auto &value) {
    return value.from_ability == from && value.to_ability == to;
  });
  return found == program.transitions.end() ? nullptr : &*found;
}
void require_valid(const ComboProgram &program, const CombatProgram &combat,
                   const ComboState &state) {
  const auto validation = validate_combo_state(program, combat, state);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().code + ": " +
                                validation.diagnostics.front().message);
}
} // namespace

ValidationResult validate_combo_program(const ComboProgram &program,
                                        const CombatProgram &combat_program) {
  ValidationResult result;
  const auto combat_validation = validate_combat_program(combat_program);
  result.diagnostics.insert(result.diagnostics.end(), combat_validation.diagnostics.begin(),
                            combat_validation.diagnostics.end());
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!combat_validation.ok()) return result;
  if (!id(program.id) || program.maximum_history == 0 ||
      program.maximum_history > 4096 || program.entry_abilities.empty() ||
      program.entry_abilities.size() > combat_program.abilities.size() ||
      program.transitions.size() > 65'536)
    add("SPRITE_COMBO_PROGRAM_INVALID", "combo identity or bounds are invalid");
  std::set<std::string> entries;
  for (const auto &entry : program.entry_abilities)
    if (!has_ability(combat_program, entry) || !entries.insert(entry).second)
      add("SPRITE_COMBO_ENTRY_INVALID", "combo entry is absent or duplicate");
  std::set<std::pair<std::string, std::string>> edges;
  std::map<std::string, std::vector<std::string>, std::less<>> graph;
  for (const auto &edge : program.transitions) {
    if (!has_ability(combat_program, edge.from_ability) ||
        !has_ability(combat_program, edge.to_ability) ||
        edge.from_ability == edge.to_ability ||
        edge.earliest_tick_offset > edge.latest_tick_offset ||
        edge.latest_tick_offset > 3'600'000 ||
        !edges.emplace(edge.from_ability, edge.to_ability).second)
      add("SPRITE_COMBO_TRANSITION_INVALID", "combo transition is invalid or duplicate");
    else
      graph[edge.from_ability].push_back(edge.to_ability);
  }
  std::set<std::string> reachable(entries.begin(), entries.end());
  std::vector<std::string> frontier(reachable.begin(), reachable.end());
  while (!frontier.empty()) {
    const auto current = frontier.back(); frontier.pop_back();
    for (const auto &next : graph[current])
      if (reachable.insert(next).second) frontier.push_back(next);
  }
  for (const auto &[from, to] : edges)
    if (!reachable.contains(from) || !reachable.contains(to))
      add("SPRITE_COMBO_UNREACHABLE", "combo transition is unreachable from every entry");
  enum class Mark { visiting, complete };
  std::map<std::string, Mark, std::less<>> marks;
  const auto visit = [&](const auto &self, const std::string &node) -> bool {
    const auto found = marks.find(node);
    if (found != marks.end()) return found->second == Mark::visiting;
    marks[node] = Mark::visiting;
    for (const auto &next : graph[node]) if (self(self, next)) return true;
    marks[node] = Mark::complete;
    return false;
  };
  for (const auto &entry : entries)
    if (visit(visit, entry)) { add("SPRITE_COMBO_CYCLE", "combo graph contains an infinite chain"); break; }
  return result;
}

ValidationResult validate_combo_state(const ComboProgram &program,
                                      const CombatProgram &combat_program,
                                      const ComboState &state) {
  auto result = validate_combo_program(program, combat_program);
  if (!result.ok()) return result;
  if (state.history.size() > program.maximum_history ||
      state.active_ability.has_value() != !state.history.empty()) {
    result.diagnostics.push_back({"SPRITE_COMBO_STATE_INVALID", "combo active/history state is inconsistent"});
    return result;
  }
  std::uint64_t previous{};
  for (std::size_t i = 0; i < state.history.size(); ++i) {
    const auto &entry = state.history[i];
    const ComboTransition *edge = nullptr;
    if (i > 0)
      edge = transition(program, state.history[i - 1].ability_id,
                        entry.ability_id);
    if (!has_ability(combat_program, entry.ability_id) ||
        (i > 0 && entry.started_tick < previous) ||
        (i == 0 && !std::ranges::contains(program.entry_abilities, entry.ability_id)) ||
        (i > 0 && edge == nullptr))
      result.diagnostics.push_back({"SPRITE_COMBO_HISTORY_INVALID", "combo history violates its graph"});
    if (edge != nullptr) {
      const auto offset = entry.started_tick - state.history[i - 1].started_tick;
      if (offset < edge->earliest_tick_offset ||
          offset > edge->latest_tick_offset ||
          (edge->requires_hit_confirmation &&
           !state.history[i - 1].hit_confirmed))
        result.diagnostics.push_back(
            {"SPRITE_COMBO_HISTORY_WINDOW_INVALID",
             "combo history violates its cancel or hit-confirm window"});
    }
    previous = entry.started_tick;
  }
  if (state.active_ability &&
      (*state.active_ability != state.history.back().ability_id ||
       state.active_started_tick != state.history.back().started_tick ||
       state.active_hit_confirmed != state.history.back().hit_confirmed))
    result.diagnostics.push_back({"SPRITE_COMBO_ACTIVE_INVALID", "combo active state differs from history"});
  return result;
}

ComboCombatResult execute_combo_combat_command(
    const ComboProgram &program, const CombatProgram &combat_program,
    ComboState &combo_state, CombatState &combat_state,
    const CombatCommand &command, bool confirms_previous_hit) {
  require_valid(program, combat_program, combo_state);
  auto combo_candidate = combo_state;
  auto combat_candidate = combat_state;
  ComboCombatResult result;
  if (!combo_candidate.active_ability) {
    if (!std::ranges::contains(program.entry_abilities, command.ability_id))
      throw std::runtime_error("ability is not a combo entry");
    result.entered_combo = true;
  } else {
    if (confirms_previous_hit) {
      combo_candidate.active_hit_confirmed = true;
      combo_candidate.history.back().hit_confirmed = true;
    }
    const auto *edge = transition(program, *combo_candidate.active_ability, command.ability_id);
    if (edge == nullptr || command.tick < combo_candidate.active_started_tick)
      throw std::runtime_error("combo transition is absent or stale");
    const auto offset = command.tick - combo_candidate.active_started_tick;
    if (offset < edge->earliest_tick_offset || offset > edge->latest_tick_offset ||
        (edge->requires_hit_confirmation &&
         !combo_candidate.active_hit_confirmed))
      throw std::runtime_error("combo cancel window or hit requirement failed");
    result.transitioned = true;
  }
  if (combo_candidate.history.size() >= program.maximum_history)
    throw std::runtime_error("combo history bound exceeded");
  result.combat_events = execute_combat_command(combat_program, combat_candidate, command);
  combo_candidate.active_ability = command.ability_id;
  combo_candidate.active_started_tick = command.tick;
  combo_candidate.active_hit_confirmed = false;
  combo_candidate.history.push_back({command.ability_id, command.tick, false});
  require_valid(program, combat_program, combo_candidate);
  combo_state = std::move(combo_candidate);
  combat_state = std::move(combat_candidate);
  return result;
}

void end_combo(const ComboProgram &program, const CombatProgram &combat_program,
               ComboState &state) {
  require_valid(program, combat_program, state);
  state = {};
}

} // namespace gspl::sprites
