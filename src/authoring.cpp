#include "gspl_sprites/authoring.hpp"

#include "gspl_sprites/domain.hpp"

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
           return (character >= 'a' && character <= 'z') ||
                  (character >= 'A' && character <= 'Z') ||
                  (character >= '0' && character <= '9') || character == '.' ||
                  character == '_' || character == '-';
         });
}

bool sha256_text(std::string_view value) {
  return value.size() == 64 &&
         std::ranges::all_of(value, [](unsigned char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

bool valid_utf8(std::string_view value) {
  for (std::size_t index = 0; index < value.size();) {
    const auto first = static_cast<unsigned char>(value[index]);
    std::size_t length = 1;
    std::uint32_t codepoint = first;
    if ((first & 0xe0U) == 0xc0U) {
      length = 2;
      codepoint = first & 0x1fU;
    } else if ((first & 0xf0U) == 0xe0U) {
      length = 3;
      codepoint = first & 0x0fU;
    } else if ((first & 0xf8U) == 0xf0U) {
      length = 4;
      codepoint = first & 0x07U;
    } else if (first >= 0x80U) {
      return false;
    }
    if (index + length > value.size())
      return false;
    for (std::size_t offset = 1; offset < length; ++offset) {
      const auto continuation =
          static_cast<unsigned char>(value[index + offset]);
      if ((continuation & 0xc0U) != 0x80U)
        return false;
      codepoint = (codepoint << 6U) | (continuation & 0x3fU);
    }
    if ((length == 2 && codepoint < 0x80U) ||
        (length == 3 && codepoint < 0x800U) ||
        (length == 4 && codepoint < 0x10000U) ||
        (codepoint >= 0xd800U && codepoint <= 0xdfffU) || codepoint > 0x10ffffU)
      return false;
    index += length;
  }
  return true;
}

std::string escape_json(std::string_view value) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string output;
  for (const unsigned char character : value) {
    if (character == '"' || character == '\\') {
      output += '\\';
      output += static_cast<char>(character);
    } else if (character == '\n') {
      output += "\\n";
    } else if (character == '\r') {
      output += "\\r";
    } else if (character == '\t') {
      output += "\\t";
    } else if (character < 0x20) {
      output += "\\u00";
      output += hex[character >> 4U];
      output += hex[character & 0x0fU];
    } else {
      output += static_cast<char>(character);
    }
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

std::string rights_name(RightsClass value) {
  switch (value) {
  case RightsClass::original_user_creation:
    return "ORIGINAL_USER_CREATION";
  case RightsClass::user_owned:
    return "USER_OWNED_REFERENCE";
  case RightsClass::licensed:
    return "LICENSED_REFERENCE";
  case RightsClass::public_domain:
    return "PUBLIC_DOMAIN";
  case RightsClass::permissive:
    return "PERMISSIVELY_LICENSED";
  case RightsClass::research_only:
    return "RESEARCH_ONLY_REFERENCE";
  case RightsClass::restricted:
    return "RESTRICTED_REFERENCE";
  case RightsClass::unknown:
    return "UNKNOWN_RIGHTS";
  case RightsClass::prohibited:
    return "PROHIBITED";
  }
  throw std::logic_error("invalid rights class");
}

std::string_view reference_use_name(AuthoringReferenceUse use) {
  switch (use) {
  case AuthoringReferenceUse::semantic_structure:
    return "semantic-structure";
  case AuthoringReferenceUse::visual_asset:
    return "visual-asset";
  case AuthoringReferenceUse::motion_asset:
    return "motion-asset";
  case AuthoringReferenceUse::audio_asset:
    return "audio-asset";
  }
  throw std::logic_error("invalid authoring reference use");
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
      !valid_utf8(project.intent) || project.fields.size() > 4'096 ||
      project.abilities.size() > 256 || project.variants.size() > 1'024 ||
      project.references.size() > 4'096 || project.targets.size() > 64 ||
      (project.revision == 0) !=
          !project.parent_revision_identity.has_value() ||
      (project.parent_revision_identity &&
       !sha256_text(*project.parent_revision_identity)))
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
      if (value.empty() || value.size() > 8'192 || !valid_utf8(value) ||
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
    if (!token(ability.value.id) || !valid_utf8(ability.value.effect) ||
        !abilities.insert(ability.value.id).second)
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
  std::set<std::string> references;
  for (const auto &reference : project.references) {
    if (!token(reference.id) || !references.insert(reference.id).second ||
        reference.uri.empty() || reference.uri.size() > 4'096 ||
        !valid_utf8(reference.uri) || !sha256_text(reference.content_sha256) ||
        static_cast<unsigned>(reference.rights) >
            static_cast<unsigned>(RightsClass::prohibited) ||
        static_cast<unsigned>(reference.use) >
            static_cast<unsigned>(AuthoringReferenceUse::audio_asset))
      add(result, "SPRITE_AUTHORING_REFERENCE_INVALID",
          "authoring reference identity, URI, or content hash is invalid");
  }
  std::set<std::string> targets;
  for (const auto &target : project.targets) {
    if (!token(target.adapter_id) ||
        !targets.insert(target.adapter_id).second || target.features.empty() ||
        target.features.size() > 256) {
      add(result, "SPRITE_AUTHORING_TARGET_INVALID",
          "authoring target identity, feature set, or uniqueness is invalid");
      continue;
    }
    try {
      const auto report = evaluate_target_compatibility(
          builtin_target_adapter(target.adapter_id), target.features);
      for (const auto &diagnostic : report.validation.diagnostics)
        add(result, diagnostic.code, diagnostic.message);
    } catch (const std::exception &error) {
      add(result, "SPRITE_AUTHORING_TARGET_UNKNOWN", error.what());
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
  auto references = project.references;
  auto targets = project.targets;
  std::ranges::sort(fields, {}, &AuthoringField::path);
  std::ranges::sort(abilities, [](const auto &left, const auto &right) {
    return left.value.id < right.value.id;
  });
  std::ranges::sort(variants, {}, &AuthoringVariant::id);
  std::ranges::sort(references, {}, &AuthoringReference::id);
  std::ranges::sort(targets, {}, &AuthoringTargetRequest::adapter_id);
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
  out << ",\"references\":[";
  for (std::size_t index = 0; index < references.size(); ++index) {
    if (index)
      out << ',';
    const auto &reference = references[index];
    out << "{\"contentSha256\":\"" << reference.content_sha256 << "\",\"id\":\""
        << reference.id
        << "\",\"required\":" << (reference.required ? "true" : "false")
        << ",\"rights\":\"" << rights_name(reference.rights) << "\",\"uri\":\""
        << escape_json(reference.uri) << "\",\"use\":\""
        << reference_use_name(reference.use) << "\"}";
  }
  out << "],\"revision\":" << project.revision << ",\"schema\":\""
      << project.schema << "\",\"targets\":[";
  for (std::size_t index = 0; index < targets.size(); ++index) {
    if (index)
      out << ',';
    auto features = targets[index].features;
    std::ranges::sort(features, {}, &TargetRequirement::feature);
    out << "{\"adapterId\":\"" << targets[index].adapter_id
        << "\",\"features\":[";
    for (std::size_t feature = 0; feature < features.size(); ++feature) {
      if (feature)
        out << ',';
      out << "{\"feature\":\"" << target_feature_name(features[feature].feature)
          << "\",\"required\":"
          << (features[feature].required ? "true" : "false") << '}';
    }
    out << "]}";
  }
  out << "],\"variants\":[";
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

std::string canonicalize_authoring_provenance(const AuthoringProject &project) {
  const auto validation = validate_authoring_project(project);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto references = project.references;
  std::ranges::sort(references, {}, &AuthoringReference::id);
  std::ostringstream out;
  out << "{\"project\":{\"id\":\"" << project.id
      << "\",\"revision\":" << project.revision << ",\"revisionIdentity\":\""
      << authoring_revision_identity(project) << "\"},\"references\":[";
  for (std::size_t index = 0; index < references.size(); ++index) {
    if (index)
      out << ',';
    const auto &reference = references[index];
    out << "{\"contentSha256\":\"" << reference.content_sha256 << "\",\"id\":\""
        << reference.id
        << "\",\"required\":" << (reference.required ? "true" : "false")
        << ",\"rights\":\"" << rights_name(reference.rights) << "\",\"uri\":\""
        << escape_json(reference.uri) << "\",\"use\":\""
        << reference_use_name(reference.use) << "\"}";
  }
  out << "]}";
  return out.str();
}

std::string
canonicalize_authoring_target_reports(const AuthoringProject &project) {
  const auto validation = validate_authoring_project(project);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto targets = project.targets;
  std::ranges::sort(targets, {}, &AuthoringTargetRequest::adapter_id);
  std::ostringstream out;
  out << "{\"reports\":[";
  for (std::size_t index = 0; index < targets.size(); ++index) {
    if (index)
      out << ',';
    out << canonicalize_target_compatibility(evaluate_target_compatibility(
        builtin_target_adapter(targets[index].adapter_id),
        targets[index].features));
  }
  out << "]}";
  return out.str();
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
  for (const auto &reference : project.references) {
    if (!reference.required)
      continue;
    const auto usage =
        reference.use == AuthoringReferenceUse::semantic_structure
            ? AssetUsage::research
            : AssetUsage::commercial_export;
    const auto decision = evaluate_rights(reference.rights, usage);
    if (!decision.allowed)
      add(result.validation, decision.code,
          "required reference " + reference.id + ": " + decision.explanation);
  }
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
                  values.at("appearance.accent_color"),
                  {},
                  std::nullopt,
                  {},
                  std::nullopt,
                  {},
                  {}};
  for (const auto &ability : project.abilities)
    if (ability.enabled && !disabled.contains(ability.value.id))
      seed.abilities.push_back(ability.value);
  result.validation = validate(seed);
  if (result.validation.ok())
    result.seed = std::move(seed);
  return result;
}

AuthoringProject authoring_project_from_seed(const SpriteSeed &seed,
                                             std::string project_id,
                                             std::string intent) {
  const auto seed_validation = validate(seed);
  if (!seed_validation.ok())
    throw std::invalid_argument(seed_validation.diagnostics.front().message);
  AuthoringProject project;
  project.id = std::move(project_id);
  project.intent = std::move(intent);
  project.fields = {
      {"identity.stable_id", {seed.stable_id}, 0, true},
      {"identity.name", {seed.name}, 0, false},
      {"identity.classification", {seed.classification}, 0, true},
      {"rights.class", {rights_name(seed.rights)}, 0, true},
      {"entropy.root", {std::to_string(seed.entropy_root)}, 0, false},
      {"appearance.primary_color", {seed.primary_color}, 0, false},
      {"appearance.accent_color", {seed.accent_color}, 0, false}};
  for (const auto &ability : seed.abilities)
    project.abilities.push_back({ability, true, false});
  const auto validation = validate_authoring_project(project);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  return project;
}

} // namespace gspl::sprites
