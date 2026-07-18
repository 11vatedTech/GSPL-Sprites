#include "gspl_sprites/transformation_persistence.hpp"

#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
std::vector<std::string> split(std::string_view value) {
  std::vector<std::string> result;
  std::size_t start = 0;
  while (start <= value.size()) {
    const auto end = value.find('|', start);
    result.emplace_back(value.substr(start, end == std::string_view::npos
      ? value.size() - start : end - start));
    if (end == std::string_view::npos) break;
    start = end + 1;
  }
  return result;
}
template <class T> T integer(std::string_view value) {
  T result{};
  const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (error != std::errc{} || end != value.data() + value.size())
    throw std::runtime_error("invalid transformation state integer");
  return result;
}
} // namespace

std::string canonicalize_transformation_program(
    const TransformationProgram &program, const CombatProgram &combat_program) {
  const auto validation = validate_transformation_program(program, combat_program);
  if (!validation.ok()) throw std::invalid_argument(validation.diagnostics.front().message);
  auto forms = program.forms;
  auto transitions = program.transformations;
  std::ranges::sort(forms, {}, &TransformationFormDefinition::id);
  std::ranges::sort(transitions, {}, &TransformationDefinition::id);
  std::ostringstream output;
  output << "schema=gspl.transformation-program/0.1\nid=" << program.id
         << "\ncombat=" << combat_program_identity(combat_program)
         << "\nbase_form=" << program.base_form
         << "\nmaximum_history=" << program.maximum_history << '\n';
  for (auto &form : forms) {
    std::ranges::sort(form.enabled_abilities);
    output << "form=" << form.id << '|' << form.maximum_health_delta << '|'
           << form.maximum_resource_delta << '\n';
    for (const auto &ability : form.enabled_abilities)
      output << "form_ability=" << form.id << '|' << ability << '\n';
  }
  for (const auto &value : transitions)
    output << "transformation=" << value.id << '|' << value.from_form << '|'
           << value.to_form << '|' << value.energy_cost << '|'
           << value.duration_ticks << '|' << (value.reversible ? "true" : "false") << '\n';
  return output.str();
}

std::string transformation_program_identity(
    const TransformationProgram &program, const CombatProgram &combat_program) {
  return sha256(canonicalize_transformation_program(program, combat_program));
}

std::string serialize_transformation_state(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &state) {
  const auto validation = validate_transformation_state(program, combat_program, state);
  if (!validation.ok()) throw std::invalid_argument(validation.diagnostics.front().message);
  std::ostringstream output;
  output << "schema=gspl.transformation-state/0.1\nprogram="
         << transformation_program_identity(program, combat_program)
         << "\ncombat=" << combat_program_identity(combat_program)
         << "\nentity=" << state.stable_entity_id << "\nform=" << state.current_form
         << "\nenergy=" << state.energy << "\nmaximum_energy=" << state.maximum_energy << '\n';
  for (const auto &entry : state.history)
    output << "history=" << entry.transformation_id << '|' << entry.from_form << '|'
           << entry.to_form << '|' << entry.completed_tick << '\n';
  if (state.active)
    output << "active=" << state.active->transformation_id << '|'
           << state.active->from_form << '|' << state.active->to_form << '|'
           << state.active->started_tick << '|' << state.active->completes_tick << '\n';
  else output << "active=none\n";
  return output.str();
}

TransformationState deserialize_transformation_state(
    const TransformationProgram &program, const CombatProgram &combat_program,
    std::string_view source, std::uint64_t maximum_bytes) {
  if (source.empty() || source.size() > maximum_bytes)
    throw std::runtime_error("transformation state exceeds byte limit");
  TransformationState state;
  std::map<std::string, bool, std::less<>> scalars;
  std::istringstream stream{std::string(source)};
  std::string line;
  std::size_t lines{};
  while (std::getline(stream, line)) {
    if (++lines > 100'000 || line.empty() || line.back() == '\r')
      throw std::runtime_error("transformation state line is invalid");
    const auto equals = line.find('=');
    if (equals == std::string::npos) throw std::runtime_error("transformation state requires key=value");
    const auto key = line.substr(0, equals); const auto value = line.substr(equals + 1);
    if (key != "history" && !scalars.emplace(key, true).second)
      throw std::runtime_error("duplicate transformation state scalar");
    if (key == "schema") { if (value != "gspl.transformation-state/0.1") throw std::runtime_error("unsupported transformation state schema"); }
    else if (key == "program") { if (value != transformation_program_identity(program, combat_program)) throw std::runtime_error("transformation program identity mismatch"); }
    else if (key == "combat") { if (value != combat_program_identity(combat_program)) throw std::runtime_error("transformation combat identity mismatch"); }
    else if (key == "entity") state.stable_entity_id = value;
    else if (key == "form") state.current_form = value;
    else if (key == "energy") state.energy = integer<std::uint32_t>(value);
    else if (key == "maximum_energy") state.maximum_energy = integer<std::uint32_t>(value);
    else if (key == "history") {
      const auto parts = split(value); if (parts.size() != 4) throw std::runtime_error("malformed transformation history");
      state.history.push_back({parts[0], parts[1], parts[2], integer<std::uint64_t>(parts[3])});
    } else if (key == "active") {
      if (value != "none") { const auto parts = split(value); if (parts.size() != 5) throw std::runtime_error("malformed active transformation");
        state.active = ActiveTransformation{parts[0], parts[1], parts[2], integer<std::uint64_t>(parts[3]), integer<std::uint64_t>(parts[4])}; }
    } else throw std::runtime_error("unknown transformation state field");
  }
  static constexpr std::array required{"schema", "program", "combat", "entity", "form", "energy", "maximum_energy", "active"};
  for (const auto key : required) if (!scalars.contains(key)) throw std::runtime_error("missing transformation state field");
  const auto validation = validate_transformation_state(program, combat_program, state);
  if (!validation.ok()) throw std::runtime_error(validation.diagnostics.front().message);
  if (serialize_transformation_state(program, combat_program, state) != source)
    throw std::runtime_error("transformation state is not canonical");
  return state;
}

std::string transformation_state_identity(
    const TransformationProgram &program, const CombatProgram &combat_program,
    const TransformationState &state) {
  return sha256(serialize_transformation_state(program, combat_program, state));
}

} // namespace gspl::sprites
