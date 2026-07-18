#include "gspl_sprites/authoring.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
constexpr std::array required_fields{
    "identity.stable_id",     "identity.name", "identity.classification",
    "rights.class",           "entropy.root",  "appearance.primary_color",
    "appearance.accent_color"};

bool token(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char character) {
           return std::isalnum(character) || character == '.' ||
                  character == '_' || character == '-';
         });
}

std::string escape_json(std::string_view value) {
  std::string output;
  for (const unsigned char character : value) {
    if (character == '"' || character == '\\')
      output += '\\';
    if (character < 0x20)
      throw std::invalid_argument(
          "authoring value contains a control character");
    output += static_cast<char>(character);
  }
  return output;
}

std::optional<RightsClass> parse_rights(std::string_view value) {
  constexpr std::array values{
      std::pair{"ORIGINAL_USER_CREATION", RightsClass::original_user_creation},
      std::pair{"USER_OWNED_REFERENCE", RightsClass::user_owned},
      std::pair{"LICENSED_REFERENCE", RightsClass::licensed},
      std::pair{"PUBLIC_DOMAIN", RightsClass::public_domain},
      std::pair{"PERMISSIVELY_LICENSED", RightsClass::permissive},
      std::pair{"RESEARCH_ONLY_REFERENCE", RightsClass::research_only},
      std::pair{"RESTRICTED_REFERENCE", RightsClass::restricted},
      std::pair{"UNKNOWN_RIGHTS", RightsClass::unknown},
      std::pair{"PROHIBITED", RightsClass::prohibited}};
  const auto found = std::ranges::find_if(
      values, [&](const auto &item) { return item.first == value; });
  return found == values.end() ? std::optional<RightsClass>{} : found->second;
}

std::string selected_value(const AuthoringField &field) {
  return field.selected ? field.alternatives.at(*field.selected)
                        : std::string{};
}

void add(ValidationResult &result, std::string code, std::string message) {
  result.diagnostics.push_back({std::move(code), std::move(message)});
}
} // namespace

ValidationResult validate_authoring_project(const AuthoringProject &project) {
  ValidationResult result;
  if (project.schema != "gspl.sprite-authoring/0.1" || !token(project.id) ||
      project.intent.empty() || project.intent.size() > 65'536 ||
      project.fields.size() > 4'096 || project.abilities.size() > 256 ||
      project.variants.size() > 1'024 ||
      (project.revision == 0) !=
          !project.parent_revision_identity.has_value() ||
      (project.parent_revision_identity &&
       project.parent_revision_identity->size() != 64))
    add(result, "SPRITE_AUTHORING_HEADER_INVALID",
        "authoring project header, limits, or revision ancestry is invalid");
  std::set<std::string> paths;
  for (const auto &field : project.fields) {
    std::set<std::string> alternatives;
    if (!token(field.path) || !paths.insert(field.path).second ||
        field.alternatives.empty() || field.alternatives.size() > 256)
      add(result, "SPRITE_AUTHORING_FIELD_INVALID",
          "authoring field is malformed, duplicate, or empty: " + field.path);
    for (const auto &value : field.alternatives)
      if (value.empty() || value.size() > 8'192 ||
          !alternatives.insert(value).second)
        add(result, "SPRITE_AUTHORING_ALTERNATIVE_INVALID",
            "authoring alternative is empty, duplicate, or too large: " +
                field.path);
    if ((field.selected && *field.selected >= field.alternatives.size()) ||
        (field.locked && !field.selected))
      add(result, "SPRITE_AUTHORING_SELECTION_INVALID",
          "authoring selection or lock is invalid: " + field.path);
  }
  for (const auto path : required_fields)
    if (!paths.contains(path))
      add(result, "SPRITE_AUTHORING_REQUIRED_FIELD_MISSING",
          "required authoring field is absent: " + std::string(path));
  std::set<std::string> abilities;
  for (const auto &ability : project.abilities)
    if (!token(ability.value.id) || !abilities.insert(ability.value.id).second)
      add(result, "SPRITE_AUTHORING_ABILITY_INVALID",
          "authored ability identity is invalid or duplicate");
  std::set<std::string> variants;
  for (const auto &variant : project.variants) {
    if (!token(variant.id) || !variants.insert(variant.id).second ||
        variant.field_overrides.size() > 4'096 ||
        variant.disabled_abilities.size() > 256)
      add(result, "SPRITE_AUTHORING_VARIANT_INVALID",
          "authoring variant is malformed or duplicate");
    std::set<std::string> overrides;
    for (const auto &[path, value] : variant.field_overrides) {
      const auto field =
          std::ranges::find(project.fields, path, &AuthoringField::path);
      if (field == project.fields.end() || field->locked || value.empty() ||
          std::ranges::find(field->alternatives, value) ==
              field->alternatives.end() ||
          !overrides.insert(path).second)
        add(result, "SPRITE_AUTHORING_VARIANT_OVERRIDE_INVALID",
            "variant override is absent, locked, duplicate, or not an "
            "alternative");
    }
    std::set<std::string> disabled;
    for (const auto &id : variant.disabled_abilities) {
      const auto ability =
          std::ranges::find_if(project.abilities, [&](const auto &item) {
            return item.value.id == id;
          });
      if (ability == project.abilities.end() || ability->locked ||
          !disabled.insert(id).second)
        add(result, "SPRITE_AUTHORING_VARIANT_ABILITY_INVALID",
            "variant-disabled ability is absent, locked, or duplicate");
    }
  }
  return result;
}

std::string canonicalize_authoring_project(const AuthoringProject &project) {
  const auto validation = validate_authoring_project(project);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto fields = project.fields;
  auto abilities = project.abilities;
  auto variants = project.variants;
  std::ranges::sort(fields, {}, &AuthoringField::path);
  std::ranges::sort(abilities, [](const auto &left, const auto &right) {
    return left.value.id < right.value.id;
  });
  std::ranges::sort(variants, {}, &AuthoringVariant::id);
  std::ostringstream out;
  out << "{\"abilities\":[";
  for (std::size_t index = 0; index < abilities.size(); ++index) {
    if (index)
      out << ',';
    const auto &item = abilities[index];
    out << "{\"activeTicks\":" << item.value.active_ticks
        << ",\"cooldownTicks\":" << item.value.cooldown_ticks
        << ",\"cost\":" << item.value.cost << ",\"effect\":\""
        << escape_json(item.value.effect)
        << "\",\"enabled\":" << (item.enabled ? "true" : "false")
        << ",\"id\":\"" << escape_json(item.value.id)
        << "\",\"locked\":" << (item.locked ? "true" : "false") << '}';
  }
  out << "],\"fields\":[";
  for (std::size_t index = 0; index < fields.size(); ++index) {
    if (index)
      out << ',';
    const auto &field = fields[index];
    out << "{\"alternatives\":[";
    for (std::size_t alternative = 0; alternative < field.alternatives.size();
         ++alternative) {
      if (alternative)
        out << ',';
      out << '"' << escape_json(field.alternatives[alternative]) << '"';
    }
    out << "],\"locked\":" << (field.locked ? "true" : "false")
        << ",\"path\":\"" << field.path << "\",\"selected\":";
    if (field.selected)
      out << *field.selected;
    else
      out << "null";
    out << '}';
  }
  out << "],\"id\":\"" << project.id << "\",\"intent\":\""
      << escape_json(project.intent) << "\",\"parentRevisionIdentity\":";
  if (project.parent_revision_identity)
    out << '"' << *project.parent_revision_identity << '"';
  else
    out << "null";
  out << ",\"revision\":" << project.revision << ",\"schema\":\""
      << project.schema << "\",\"variants\":[";
  for (std::size_t index = 0; index < variants.size(); ++index) {
    if (index)
      out << ',';
    auto overrides = variants[index].field_overrides;
    auto disabled = variants[index].disabled_abilities;
    std::ranges::sort(overrides);
    std::ranges::sort(disabled);
    out << "{\"disabledAbilities\":[";
    for (std::size_t ability = 0; ability < disabled.size(); ++ability) {
      if (ability)
        out << ',';
      out << '"' << disabled[ability] << '"';
    }
    out << "],\"fieldOverrides\":[";
    for (std::size_t override_index = 0; override_index < overrides.size();
         ++override_index) {
      if (override_index)
        out << ',';
      out << "{\"path\":\"" << overrides[override_index].first
          << "\",\"value\":\"" << escape_json(overrides[override_index].second)
          << "\"}";
    }
    out << "],\"id\":\"" << variants[index].id << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string authoring_revision_identity(const AuthoringProject &project) {
  return sha256(canonicalize_authoring_project(project));
}

AuthoringProject
revise_authoring_project(const AuthoringProject &project,
                         std::string_view expected_revision_identity,
                         std::span<const AuthoringEdit> edits) {
  const auto current_identity = authoring_revision_identity(project);
  if (expected_revision_identity != current_identity)
    throw std::runtime_error("authoring revision conflict");
  if (edits.empty() || edits.size() > 4'096)
    throw std::invalid_argument("authoring edit batch is empty or too large");
  AuthoringProject result = project;
  std::set<std::string> edited;
  bool changed = false;
  for (const auto &edit : edits) {
    if (!edit.selected_value && !edit.locked)
      throw std::invalid_argument("authoring edit has no operation");
    if (!edited.insert(edit.field_path).second)
      throw std::invalid_argument("authoring field edited more than once");
    const auto field = std::ranges::find(result.fields, edit.field_path,
                                         &AuthoringField::path);
    if (field == result.fields.end())
      throw std::invalid_argument("authoring edit references an absent field");
    if (field->locked && edit.selected_value)
      throw std::invalid_argument("locked authoring field cannot be changed");
    if (edit.selected_value) {
      const auto selected =
          std::ranges::find(field->alternatives, *edit.selected_value);
      if (selected == field->alternatives.end())
        throw std::invalid_argument(
            "authoring edit value is not an alternative");
      const auto selection = static_cast<std::uint32_t>(
          std::distance(field->alternatives.begin(), selected));
      changed = changed || field->selected != selection;
      field->selected = selection;
    }
    if (edit.locked) {
      if (*edit.locked && !field->selected)
        throw std::invalid_argument(
            "unresolved authoring field cannot be locked");
      changed = changed || field->locked != *edit.locked;
      field->locked = *edit.locked;
    }
  }
  if (!changed)
    throw std::invalid_argument("authoring edit batch makes no change");
  result.parent_revision_identity = current_identity;
  ++result.revision;
  const auto validation = validate_authoring_project(result);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  return result;
}

AuthoringLoweringResult
lower_authoring_project(const AuthoringProject &project,
                        std::optional<std::string_view> variant_id) {
  AuthoringLoweringResult result;
  result.validation = validate_authoring_project(project);
  if (!result.validation.ok())
    return result;
  std::map<std::string, std::string, std::less<>> values;
  for (const auto &field : project.fields) {
    if (field.selected)
      values.emplace(field.path, selected_value(field));
  }
  std::set<std::string> disabled;
  if (variant_id) {
    const auto variant =
        std::ranges::find(project.variants, *variant_id, &AuthoringVariant::id);
    if (variant == project.variants.end()) {
      add(result.validation, "SPRITE_AUTHORING_VARIANT_UNKNOWN",
          "requested authoring variant is absent");
      return result;
    }
    for (const auto &[path, value] : variant->field_overrides)
      values[path] = value;
    disabled.insert(variant->disabled_abilities.begin(),
                    variant->disabled_abilities.end());
  }
  for (const auto path : required_fields)
    if (!values.contains(path))
      add(result.validation, "SPRITE_AUTHORING_UNRESOLVED",
          "required authoring decision is unresolved: " + std::string(path));
  if (!result.validation.ok())
    return result;
  const auto rights = parse_rights(values.at("rights.class"));
  std::uint64_t entropy{};
  const auto entropy_text = values.at("entropy.root");
  const auto [end, error] = std::from_chars(
      entropy_text.data(), entropy_text.data() + entropy_text.size(), entropy);
  if (!rights || error != std::errc{} ||
      end != entropy_text.data() + entropy_text.size()) {
    add(result.validation, "SPRITE_AUTHORING_TYPED_VALUE_INVALID",
        "rights or entropy authoring value has an invalid type");
    return result;
  }
  SpriteSeed seed{"gspl.sprite-seed/0.1",
                  values.at("identity.stable_id"),
                  values.at("identity.name"),
                  values.at("identity.classification"),
                  *rights,
                  entropy,
                  values.at("appearance.primary_color"),
                  values.at("appearance.accent_color")};
  for (const auto &ability : project.abilities)
    if (ability.enabled && !disabled.contains(ability.value.id))
      seed.abilities.push_back(ability.value);
  result.validation = validate(seed);
  if (result.validation.ok())
    result.seed = std::move(seed);
  return result;
}

} // namespace gspl::sprites
