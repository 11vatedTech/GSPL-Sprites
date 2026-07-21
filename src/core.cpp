#include "gspl_sprites/core.hpp"
#include "gspl_sprites/domain.hpp"
#include "gspl_sprites/target_contract.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::vector<std::string> split(std::string_view input, char delimiter) {
  std::vector<std::string> result;
  std::size_t begin = 0;
  while (begin <= input.size()) {
    const auto end = input.find(delimiter, begin);
    result.emplace_back(input.substr(begin, end == std::string_view::npos ? input.size() - begin : end - begin));
    if (end == std::string_view::npos) break;
    begin = end + 1;
  }
  return result;
}

std::uint32_t parse_u32(std::string_view value, std::string_view field) {
  std::uint32_t result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{} || ptr != value.data() + value.size()) throw std::runtime_error("invalid unsigned integer for " + std::string(field));
  return result;
}

std::uint64_t parse_u64(std::string_view value, std::string_view field) {
  std::uint64_t result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{} || ptr != value.data() + value.size()) throw std::runtime_error("invalid unsigned integer for " + std::string(field));
  return result;
}

double parse_double(std::string_view value, std::string_view field) {
  double result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result, std::chars_format::general);
  if (ec != std::errc{} || ptr != value.data() + value.size() || !std::isfinite(result)) throw std::runtime_error("invalid finite number for " + std::string(field));
  return result;
}

bool parse_bool(std::string_view value, std::string_view field) {
  if (value == "true") return true;
  if (value == "false") return false;
  throw std::runtime_error("invalid boolean for " + std::string(field));
}

Comparison parse_comparison(std::string_view value) {
  if (value == "EQUAL")
    return Comparison::equal;
  if (value == "NOT_EQUAL")
    return Comparison::not_equal;
  if (value == "LESS")
    return Comparison::less;
  if (value == "LESS_EQUAL")
    return Comparison::less_equal;
  if (value == "GREATER")
    return Comparison::greater;
  if (value == "GREATER_EQUAL")
    return Comparison::greater_equal;
  throw std::runtime_error("invalid transition comparison");
}


bool repeatable_field(std::string_view key) {
  static constexpr std::array values{"ability", "storm_ability", "bone", "socket", "clip", "track", "clip_event", "state", "transition", "collision", "collision_window"};
  return std::ranges::find(values, key) != values.end();
}

std::vector<double> parse_double_list(std::string_view value, std::size_t expected, std::string_view field) {
  const auto parts = split(value, ',');
  if (parts.size() != expected) throw std::runtime_error("expected " + std::to_string(expected) + " values for " + std::string(field));
  std::vector<double> result;
  result.reserve(expected);
  for (const auto& part : parts) result.push_back(parse_double(trim(part), field));
  return result;
}

RightsClass parse_rights(std::string_view value) {
  static const std::map<std::string_view, RightsClass> values{
    {"ORIGINAL_USER_CREATION", RightsClass::original_user_creation}, {"USER_OWNED_REFERENCE", RightsClass::user_owned},
    {"LICENSED_REFERENCE", RightsClass::licensed}, {"PUBLIC_DOMAIN", RightsClass::public_domain},
    {"PERMISSIVELY_LICENSED", RightsClass::permissive}, {"RESEARCH_ONLY_REFERENCE", RightsClass::research_only},
    {"RESTRICTED_REFERENCE", RightsClass::restricted}, {"UNKNOWN_RIGHTS", RightsClass::unknown}, {"PROHIBITED", RightsClass::prohibited}};
  const auto found = values.find(value);
  if (found == values.end()) throw std::runtime_error("unknown rights classification");
  return found->second;
}

std::string rights_text(RightsClass value) {
  switch (value) {
    case RightsClass::original_user_creation: return "ORIGINAL_USER_CREATION";
    case RightsClass::user_owned: return "USER_OWNED_REFERENCE";
    case RightsClass::licensed: return "LICENSED_REFERENCE";
    case RightsClass::public_domain: return "PUBLIC_DOMAIN";
    case RightsClass::permissive: return "PERMISSIVELY_LICENSED";
    case RightsClass::research_only: return "RESEARCH_ONLY_REFERENCE";
    case RightsClass::restricted: return "RESTRICTED_REFERENCE";
    case RightsClass::unknown: return "UNKNOWN_RIGHTS";
    case RightsClass::prohibited: return "PROHIBITED";
  }
  throw std::logic_error("unreachable rights classification");
}

std::string escape_json(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) { case '\\': out += "\\\\"; break; case '"': out += "\\\""; break; case '\n': out += "\\n"; break; case '\r': out += "\\r"; break; case '\t': out += "\\t"; break; default: if (c < 0x20) throw std::runtime_error("control character is not canonicalizable"); else out += static_cast<char>(c); }
  }
  return out;
}

constexpr std::array<std::uint32_t, 64> k{
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
constexpr std::uint32_t rotr(std::uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
}

SpriteSeed parse_seed(std::string_view source) {
  const auto source_check = enforce_resource_limits_source(source);
  if (!source_check.ok()) throw std::runtime_error(source_check.diagnostics.front().code + ": " + source_check.diagnostics.front().message);
  SpriteSeed seed;
  std::istringstream stream{std::string(source)};
  std::string line;
  std::size_t line_number = 0;
  std::map<std::string, bool> seen;
  std::string current_section_type;
  std::string current_section_name;
  std::string current_section_form;
  auto reset_section = [&] { current_section_type.clear(); current_section_name.clear(); current_section_form.clear(); };
  auto in_section = [&] { return !current_section_type.empty(); };
  while (std::getline(stream, line)) {
    ++line_number;
    if (line_number > 4096) throw std::runtime_error("source exceeds 4096-line limit");
    if (line.size() > 8192) throw std::runtime_error("line exceeds 8192-byte limit");
    line = trim(line);
    if (line.empty() || line.starts_with('#')) continue;
    if (line.starts_with('[') && line.ends_with(']')) {
      reset_section();
      const auto inner = trim(line.substr(1, line.size() - 2));
      if (inner.empty()) throw std::runtime_error("line " + std::to_string(line_number) + ": empty section name");
      const auto dot = inner.find('.');
      if (dot == std::string::npos) {
        current_section_type = inner;
      } else {
        current_section_type = inner.substr(0, dot);
        auto sub = inner.substr(dot + 1);
        const auto dot2 = sub.find('.');
        if (current_section_type == "morphology" && dot2 != std::string::npos) {
          current_section_name = sub.substr(0, dot2);
          current_section_form = sub.substr(dot2 + 1);
        } else {
          current_section_name = sub;
        }
        if (current_section_name.empty()) throw std::runtime_error("line " + std::to_string(line_number) + ": empty section sub-name");
      }
      continue;
    }
    const auto equals = line.find('=');
    if (equals == std::string::npos) throw std::runtime_error("line " + std::to_string(line_number) + ": expected key=value");
    const auto key = trim(line.substr(0, equals));
    const auto value = trim(line.substr(equals + 1));
    if (!in_section() && !repeatable_field(key) && !seen.emplace(key, true).second) throw std::runtime_error("line " + std::to_string(line_number) + ": duplicate field " + key);
    if (in_section()) {
      if (current_section_type == "form") {
        if (key == "transformations") {
          FormSeed fs;
          fs.id = current_section_name;
          const auto trimmed = trim(value);
          if (!trimmed.empty()) {
            for (const auto& t : split(trimmed, ',')) {
              const auto tid = trim(t);
              if (!tid.empty()) fs.transformation_ids.push_back(tid);
            }
          }
          seed.forms.push_back(std::move(fs));
        } else if (key == "resource_capacity") {
          if (!seed.form_attributes.count(current_section_name)) seed.form_attributes[current_section_name] = FormAttributes{};
          seed.form_attributes[current_section_name].resource_capacity = parse_u32(value, "form.resource_capacity");
        } else if (key == "collision_scale") {
          if (!seed.form_attributes.count(current_section_name)) seed.form_attributes[current_section_name] = FormAttributes{};
          seed.form_attributes[current_section_name].collision_scale = parse_double(value, "form.collision_scale");
        } else if (key == "ability_envelope") {
          if (!seed.form_attributes.count(current_section_name)) seed.form_attributes[current_section_name] = FormAttributes{};
          seed.form_attributes[current_section_name].ability_envelope = parse_double(value, "form.ability_envelope");
        } else if (key == "max_health") {
          if (!seed.form_attributes.count(current_section_name)) seed.form_attributes[current_section_name] = FormAttributes{};
          seed.form_attributes[current_section_name].max_health = parse_u32(value, "form.max_health");
        } else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown form field " + key);
      } else if (current_section_type == "transformation") {
        if (key == "from_form") { seed.transformations.push_back({current_section_name, value, {}, {}}); }
        else if (key == "to_form") { if (!seed.transformations.empty() && seed.transformations.back().id == current_section_name && seed.transformations.back().to_form.empty()) seed.transformations.back().to_form = value; else throw std::runtime_error("line " + std::to_string(line_number) + ": to_form requires preceding from_form"); }
        else if (key == "trigger") { if (!seed.transformations.empty() && seed.transformations.back().id == current_section_name) seed.transformations.back().trigger_condition = value; else throw std::runtime_error("line " + std::to_string(line_number) + ": trigger requires preceding from_form"); }
        else if (key == "duration_ticks") { if (!seed.transformations.empty() && seed.transformations.back().id == current_section_name) seed.transformations.back().duration_ticks = parse_u32(value, "transformation.duration_ticks"); else throw std::runtime_error("line " + std::to_string(line_number) + ": duration_ticks requires preceding from_form"); }
        else if (key == "resource_cost") { if (!seed.transformations.empty() && seed.transformations.back().id == current_section_name) seed.transformations.back().resource_cost = parse_u32(value, "transformation.resource_cost"); else throw std::runtime_error("line " + std::to_string(line_number) + ": resource_cost requires preceding from_form"); }
        else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown transformation field " + key);
      } else if (current_section_type == "morphology") {
        MorphologyPart part;
        if (key == "position") { const auto v = parse_double_list(value, 3, "morphology.position"); part.x = v[0]; part.y = v[1]; part.z = v[2]; }
        else if (key == "size") { const auto v = parse_double_list(value, 3, "morphology.size"); part.size_x = v[0]; part.size_y = v[1]; part.size_z = v[2]; }
        else if (key == "color") { part.color = value; }
        else if (key == "rotation") { part.rotation_degrees = parse_double(value, "morphology.rotation"); }
        else if (key == "parent") { part.parent = value; }
        else if (key == "emissive") { part.emissive = parse_bool(value, "morphology.emissive"); }
        else if (key == "electrical_marking") { part.electrical_marking = parse_bool(value, "morphology.electrical_marking"); }
        else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown morphology field " + key);
        auto upsert = [&](auto& map) {
          if (!map.emplace(current_section_name, part).second) {
            auto& existing = map[current_section_name];
            if (key == "position") { existing.x = part.x; existing.y = part.y; existing.z = part.z; }
            else if (key == "size") { existing.size_x = part.size_x; existing.size_y = part.size_y; existing.size_z = part.size_z; }
            else if (key == "color") { existing.color = part.color; }
            else if (key == "rotation") { existing.rotation_degrees = part.rotation_degrees; }
            else if (key == "parent") { existing.parent = part.parent; }
            else if (key == "emissive") { existing.emissive = part.emissive; }
            else if (key == "electrical_marking") { existing.electrical_marking = part.electrical_marking; }
          }
        };
        if (current_section_form.empty()) {
          upsert(seed.morphology);
        } else {
          upsert(seed.form_morphology_overrides[current_section_form]);
        }
      } else if (current_section_type == "runtime") {
        if (!seed.runtime) seed.runtime = RuntimeAttributes{};
        if (key == "aggression") seed.runtime->aggression = parse_u32(value, "runtime.aggression");
        else if (key == "curiosity") seed.runtime->curiosity = parse_u32(value, "runtime.curiosity");
        else if (key == "energy") seed.runtime->energy = parse_u32(value, "runtime.energy");
        else if (key == "loyalty") seed.runtime->loyalty = parse_u32(value, "runtime.loyalty");
        else if (key == "animation_intents") {
          for (const auto& pair : split(value, ',')) {
            const auto colon = pair.find(':');
            if (colon == std::string::npos) throw std::runtime_error("line " + std::to_string(line_number) + ": animation_intent requires state:clip format");
            seed.runtime->animation_intents.emplace_back(trim(pair.substr(0, colon)), trim(pair.substr(colon + 1)));
          }
        } else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown runtime field " + key);
      } else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown section type " + current_section_type);
      continue;
    }
    if (key == "schema") seed.schema = value;
    else if (key == "id") seed.stable_id = value;
    else if (key == "name") seed.name = value;
    else if (key == "classification") seed.classification = value;
    else if (key == "rights") seed.rights = parse_rights(value);
    else if (key == "entropy_root") seed.entropy_root = parse_u64(value, key);
    else if (key == "primary_color") seed.primary_color = value;
    else if (key == "accent_color") seed.accent_color = value;
    else if (key == "storm_primary_color") seed.storm_primary_color = value;
    else if (key == "storm_accent_color") seed.storm_accent_color = value;
    else if (key == "emissive_color") seed.emissive_color = value;
    else if (key == "aura_color") seed.aura_color = value;
    else if (key == "storm_ability") {
      if (seed.storm_abilities.size() >= 64) throw std::runtime_error("storm_ability count exceeds 64");
      const auto parts = split(value, '|');
      if (parts.size() != 5) throw std::runtime_error("line " + std::to_string(line_number) + ": storm_ability requires id|effect|cost|cooldown|active");
      seed.storm_abilities.push_back({parts[0], parts[1], parse_u32(parts[2], "storm_ability.cost"), parse_u32(parts[3], "storm_ability.cooldown"), parse_u32(parts[4], "storm_ability.active")});
    } else if (key == "ability") {
      if (seed.abilities.size() >= 64) throw std::runtime_error("ability count exceeds 64");
      const auto parts = split(value, '|');
      if (parts.size() != 5) throw std::runtime_error("line " + std::to_string(line_number) + ": ability requires id|effect|cost|cooldown|active");
      seed.abilities.push_back({parts[0], parts[1], parse_u32(parts[2], "ability.cost"), parse_u32(parts[3], "ability.cooldown"), parse_u32(parts[4], "ability.active")});
    } else if (key == "rig") {
      seed.rig = RigDefinition{value, {}, {}};
    } else if (key == "bone") {
      if (!seed.rig) throw std::runtime_error("line " + std::to_string(line_number) + ": rig must precede bones");
      if (seed.rig->bones.size() >= 256) throw std::runtime_error("bone count exceeds 256");
      const auto p=split(value,'|'); if(p.size()!=10) throw std::runtime_error("line " + std::to_string(line_number) + ": bone requires id|parent|-transform-|length|limits");
      seed.rig->bones.push_back({p[0],p[1]=="-"?std::nullopt:std::optional<std::string>{p[1]},{parse_double(p[2],"bone.x"),parse_double(p[3],"bone.y"),parse_double(p[4],"bone.rotation"),parse_double(p[5],"bone.scale_x"),parse_double(p[6],"bone.scale_y")},parse_double(p[7],"bone.length"),{parse_double(p[8],"bone.minimum"),parse_double(p[9],"bone.maximum")}});
    } else if (key == "socket") {
      if (!seed.rig) throw std::runtime_error("line " + std::to_string(line_number) + ": rig must precede sockets");
      if (seed.rig->sockets.size() >= 256) throw std::runtime_error("socket count exceeds 256");
      const auto p=split(value,'|'); if(p.size()!=7) throw std::runtime_error("line " + std::to_string(line_number) + ": socket requires id|bone|transform");
      seed.rig->sockets.push_back({p[0],p[1],{parse_double(p[2],"socket.x"),parse_double(p[3],"socket.y"),parse_double(p[4],"socket.rotation"),parse_double(p[5],"socket.scale_x"),parse_double(p[6],"socket.scale_y")}});
    } else if (key == "clip") {
      if (seed.clips.size() >= 256) throw std::runtime_error("clip count exceeds 256");
      const auto p=split(value,'|'); if(p.size()!=3) throw std::runtime_error("line " + std::to_string(line_number) + ": clip requires id|duration|looping");
      seed.clips.push_back({p[0],parse_u32(p[1],"clip.duration"),parse_bool(p[2],"clip.looping"),{}, {}});
    } else if (key == "track") {
      const auto p=split(value,'|'); if(p.size()!=3) throw std::runtime_error("line " + std::to_string(line_number) + ": track requires clip|bone|key-list");
      const auto clip=std::ranges::find(seed.clips,p[0],&SkeletalClip::id); if(clip==seed.clips.end()) throw std::runtime_error("line " + std::to_string(line_number) + ": clip must precede track");
      BoneTrack track{p[1],{}}; const auto keys=split(p[2],';'); if(keys.size()>65536) throw std::runtime_error("track key count exceeds 65536");
      for(const auto& encoded_key:keys){const auto v=split(encoded_key,',');if(v.size()!=6)throw std::runtime_error("line " + std::to_string(line_number) + ": key requires tick,x,y,rotation,scale_x,scale_y");track.keys.push_back({parse_u32(v[0],"key.tick"),{parse_double(v[1],"key.x"),parse_double(v[2],"key.y"),parse_double(v[3],"key.rotation"),parse_double(v[4],"key.scale_x"),parse_double(v[5],"key.scale_y")}});} clip->tracks.push_back(std::move(track));
    } else if (key == "clip_event") {
      const auto p=split(value,'|');if(p.size()!=3)throw std::runtime_error("line " + std::to_string(line_number) + ": clip_event requires clip|id|tick");const auto clip=std::ranges::find(seed.clips,p[0],&SkeletalClip::id);if(clip==seed.clips.end())throw std::runtime_error("line " + std::to_string(line_number) + ": clip must precede event");clip->events.emplace_back(p[1],parse_u32(p[2],"event.tick"));
    } else if (key == "initial_state") {
      if (!seed.animation_graph) seed.animation_graph=AnimationStateGraph{};
      seed.animation_graph->initial_state=value;
    } else if (key == "state") {
      if (!seed.animation_graph) seed.animation_graph=AnimationStateGraph{};
      const auto p=split(value,'|');if(p.size()!=2)throw std::runtime_error("line " + std::to_string(line_number) + ": state requires id|clip");seed.animation_graph->states.push_back({p[0],p[1],{}});
    } else if (key == "transition") {
      if (!seed.animation_graph)
        throw std::runtime_error("line " + std::to_string(line_number) + ": states must precede transitions");
      const auto p = split(value,'|');
      if(p.size()!=8)
        throw std::runtime_error("line " + std::to_string(line_number) + ": transition requires source|target|parameter|comparison|threshold|min|blend|priority");
      const auto state=std::ranges::find(seed.animation_graph->states,p[0],&AnimationState::id);
      if(state==seed.animation_graph->states.end())
        throw std::runtime_error("line " + std::to_string(line_number) + ": source state must precede transition");
      state->transitions.push_back({p[1],p[2],parse_comparison(p[3]),parse_double(p[4],"transition.threshold"),parse_u32(p[5],"transition.minimum"),parse_u32(p[6],"transition.blend"),parse_u32(p[7],"transition.priority")});
    } else if (key == "collision") {
      if(seed.collision_shapes.size()>=512)
        throw std::runtime_error("collision shape count exceeds 512");
      const auto p=split(value,'|');
      if(p.size()!=7)
        throw std::runtime_error("line " + std::to_string(line_number) + ": collision requires id|kind|attachment|offset|extents");
      const auto kind=p[1]=="CIRCLE"?CollisionKind::circle:p[1]=="AXIS_ALIGNED_BOX"?CollisionKind::axis_aligned_box:throw std::runtime_error("invalid collision kind");
      seed.collision_shapes.push_back({p[0],kind,p[2],parse_double(p[3],"collision.offset_x"),parse_double(p[4],"collision.offset_y"),parse_double(p[5],"collision.extent_x"),parse_double(p[6],"collision.extent_y")});
    } else if (key == "collision_window") {
      if(seed.collision_windows.size()>=2048)
        throw std::runtime_error("collision window count exceeds 2048");
      const auto p=split(value,'|');
      if(p.size()!=5)
        throw std::runtime_error("line " + std::to_string(line_number) + ": collision_window requires ability|shape|start|end|deals_damage");
      seed.collision_windows.push_back({p[1],parse_u32(p[2],"window.start"),parse_u32(p[3],"window.end"),parse_bool(p[4],"window.deals_damage"),p[0]});
    } else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown field " + key);

  }
  return seed;
}

ValidationResult validate(const SpriteSeed& seed) {
  ValidationResult result;
  auto require = [&](bool condition, std::string code, std::string message) { if (!condition) result.diagnostics.push_back({std::move(code), std::move(message)}); };
  require(seed.schema == "gspl.sprite-seed/0.1" || seed.schema == "gspl.sprite-seed/0.2", "SPRITE_SCHEMA_UNSUPPORTED", "schema must be gspl.sprite-seed/0.1 or 0.2");
  require(!seed.stable_id.empty() && seed.stable_id.size() <= 128, "SPRITE_ID_INVALID", "stable id must contain 1..128 bytes");
  require(!seed.name.empty() && seed.name.size() <= 128, "SPRITE_NAME_INVALID", "name must contain 1..128 bytes");
  require(!seed.classification.empty(), "SPRITE_CLASSIFICATION_REQUIRED", "classification is required");
  require(seed.rights != RightsClass::unknown && seed.rights != RightsClass::prohibited && seed.rights != RightsClass::research_only && seed.rights != RightsClass::restricted, "SPRITE_RIGHTS_EXPORT_DENIED", "rights classification does not permit production export");
  const auto color_ok = [](const std::string& c) { return c.size() == 7 && c[0] == '#' && std::all_of(c.begin() + 1, c.end(), [](unsigned char x){ return std::isxdigit(x) != 0; }); };
  require(color_ok(seed.primary_color) && color_ok(seed.accent_color), "SPRITE_COLOR_INVALID", "colors must use #RRGGBB");
  require(!seed.abilities.empty() && seed.abilities.size() <= 64, "SPRITE_ABILITIES_INVALID", "entity requires 1..64 abilities");
  std::set<std::string> ability_ids;
  for (const auto& ability : seed.abilities) {
    require(!ability.id.empty() && !ability.effect.empty(), "SPRITE_ABILITY_INVALID", "ability id and semantic effect are required");
    require(ability.cost <= 100 && ability.active_ticks > 0 && ability.active_ticks <= 600 && ability.cooldown_ticks <= 36000, "SPRITE_ABILITY_BOUNDS", "ability cost or timing exceeds bounds");
    require(ability_ids.insert(ability.id).second, "SPRITE_ABILITY_DUPLICATE", "ability ids must be unique");
  }
  const bool has_animation_domain = seed.rig || !seed.clips.empty() || seed.animation_graph || !seed.collision_shapes.empty() || !seed.collision_windows.empty();
  require(!has_animation_domain || seed.rig.has_value(), "SPRITE_RIG_REQUIRED", "rig is required when animation or collision semantics are present");
  if (seed.rig) {
    const auto rig_validation=validate_rig(*seed.rig); result.diagnostics.insert(result.diagnostics.end(),rig_validation.diagnostics.begin(),rig_validation.diagnostics.end());
    for(const auto& clip:seed.clips){const auto clip_validation=validate_skeletal_clip(clip,*seed.rig);result.diagnostics.insert(result.diagnostics.end(),clip_validation.diagnostics.begin(),clip_validation.diagnostics.end());}
    if(seed.animation_graph){const auto graph_validation=validate_state_graph(*seed.animation_graph,seed.clips);result.diagnostics.insert(result.diagnostics.end(),graph_validation.diagnostics.begin(),graph_validation.diagnostics.end());}
    const auto shape_validation=validate_collision_contract(seed.collision_shapes,{},*seed.rig,0);result.diagnostics.insert(result.diagnostics.end(),shape_validation.diagnostics.begin(),shape_validation.diagnostics.end());
    for(const auto& window:seed.collision_windows){const auto ability=std::ranges::find(seed.abilities,window.ability_id,&AbilitySeed::id);if(ability==seed.abilities.end()){result.diagnostics.push_back({"SPRITE_COLLISION_ABILITY_MISSING","collision window references absent ability: "+window.ability_id});continue;}const std::array one{window};const auto collision_validation=validate_collision_contract(seed.collision_shapes,one,*seed.rig,ability->active_ticks);result.diagnostics.insert(result.diagnostics.end(),collision_validation.diagnostics.begin(),collision_validation.diagnostics.end());}
  }
  if (!seed.forms.empty() || !seed.transformations.empty() || !seed.morphology.empty() || seed.runtime) {
    require(!seed.forms.empty(), "SPRITE_FORMS_REQUIRED", "forms are required when transformation or morphology semantics are present");
    require(!seed.transformations.empty(), "SPRITE_TRANSFORMATIONS_REQUIRED", "transformations are required when form semantics are present");
    require(seed.morphology.size() >= 11, "SPRITE_MORPHOLOGY_INCOMPLETE", "morphology requires at least 11 body parts (torso, head, eyes, ears, muzzle, tail, limbs, aura)");
    std::set<std::string> form_names;
    for (const auto& form : seed.forms) {
      require(!form.id.empty(), "SPRITE_FORM_INVALID", "form id must not be empty");
      require(form_names.insert(form.id).second, "SPRITE_FORM_DUPLICATE", "form ids must be unique");
    }
    std::set<std::string> transformation_ids;
    for (const auto& t : seed.transformations) {
      require(!t.id.empty(), "SPRITE_TRANSFORMATION_INVALID", "transformation id must not be empty");
      require(!t.from_form.empty() && !t.to_form.empty(), "SPRITE_TRANSFORMATION_ENDPOINTS", "transformation requires from_form and to_form");
      require(form_names.count(t.from_form) && form_names.count(t.to_form), "SPRITE_TRANSFORMATION_FORM_MISSING", "transformation references non-existent form");
      require(transformation_ids.insert(t.id).second, "SPRITE_TRANSFORMATION_DUPLICATE", "transformation ids must be unique");
    }
    for (const auto& form : seed.forms) {
      for (const auto& t_name : form.transformation_ids) {
        require(transformation_ids.count(t_name), "SPRITE_FORM_TRANSFORMATION_MISSING", "form references non-existent transformation");
      }
    }
    if (seed.runtime) {
      require(seed.runtime->aggression <= 100 && seed.runtime->curiosity <= 100 && seed.runtime->energy <= 100 && seed.runtime->loyalty <= 100, "SPRITE_RUNTIME_BOUNDS", "runtime attributes must be 0..100");
    }
  }
  if (result.ok()) {
    const auto rl = enforce_resource_limits(seed);
    result.diagnostics.insert(result.diagnostics.end(), rl.diagnostics.begin(), rl.diagnostics.end());
  }
  return result;
}

ValidationResult enforce_resource_limits_source(std::string_view source, const ResourceLimits& limits) {
  ValidationResult result;
  auto add = [&](bool ok, std::string code, std::string msg) { if (!ok) result.diagnostics.push_back({std::move(code), std::move(msg)}); };
  add(source.size() <= limits.max_source_bytes, "RESOURCE_SOURCE_SIZE",
      "source size " + std::to_string(source.size()) + " bytes exceeds maximum " + std::to_string(limits.max_source_bytes) + " bytes");
  std::size_t token_count = 0;
  std::size_t max_token_len = 0;
  std::size_t current_len = 0;
  for (std::size_t i = 0; i <= source.size(); ++i) {
    if (i == source.size() || source[i] <= ' ') {
      if (current_len > 0) {
        ++token_count;
        if (current_len > max_token_len) max_token_len = current_len;
        current_len = 0;
      }
    } else {
      ++current_len;
    }
  }
  add(token_count <= limits.max_token_count, "RESOURCE_TOKEN_COUNT",
      "token count " + std::to_string(token_count) + " exceeds maximum " + std::to_string(limits.max_token_count));
  add(max_token_len <= limits.max_token_length, "RESOURCE_TOKEN_LENGTH",
      "max token length " + std::to_string(max_token_len) + " exceeds maximum " + std::to_string(limits.max_token_length));
  return result;
}

ValidationResult enforce_resource_limits(const SpriteSeed& seed, const ResourceLimits& limits) {
  ValidationResult result;
  auto add = [&](bool ok, std::string code, std::string msg) { if (!ok) result.diagnostics.push_back({std::move(code), std::move(msg)}); };
  const auto canonical = canonicalize(seed);
  add(canonical.size() <= limits.max_seed_bytes, "RESOURCE_SEED_SIZE",
      "seed serialized size " + std::to_string(canonical.size()) + " bytes exceeds maximum " + std::to_string(limits.max_seed_bytes) + " bytes");
  add(seed.forms.size() <= limits.max_forms, "RESOURCE_FORMS",
      "forms count " + std::to_string(seed.forms.size()) + " exceeds maximum " + std::to_string(limits.max_forms));
  add(seed.transformations.size() <= limits.max_transformations, "RESOURCE_TRANSFORMATIONS",
      "transformations count " + std::to_string(seed.transformations.size()) + " exceeds maximum " + std::to_string(limits.max_transformations));
  if (seed.rig) {
    add(seed.rig->bones.size() <= limits.max_bones, "RESOURCE_BONES",
        "bones count " + std::to_string(seed.rig->bones.size()) + " exceeds maximum " + std::to_string(limits.max_bones));
    add(seed.rig->sockets.size() <= limits.max_sockets, "RESOURCE_SOCKETS",
        "sockets count " + std::to_string(seed.rig->sockets.size()) + " exceeds maximum " + std::to_string(limits.max_sockets));
  }
  add(seed.clips.size() <= limits.max_animation_clips, "RESOURCE_CLIPS",
      "animation clips count " + std::to_string(seed.clips.size()) + " exceeds maximum " + std::to_string(limits.max_animation_clips));
  auto check_string = [&](std::string_view name, std::string_view val) {
    add(val.size() <= limits.max_string_length, "RESOURCE_STRING_LENGTH",
        std::string(name) + " length " + std::to_string(val.size()) + " exceeds maximum " + std::to_string(limits.max_string_length));
  };
  check_string("stable_id", seed.stable_id);
  check_string("name", seed.name);
  check_string("classification", seed.classification);
  check_string("primary_color", seed.primary_color);
  check_string("accent_color", seed.accent_color);
  check_string("schema", seed.schema);
  std::size_t gene_count = seed.abilities.size() + seed.storm_abilities.size() +
                           seed.transformations.size() + seed.forms.size();
  add(gene_count <= limits.max_gene_count, "RESOURCE_GENE_COUNT",
      "gene count " + std::to_string(gene_count) + " exceeds maximum " + std::to_string(limits.max_gene_count));
  std::size_t constraint_count = seed.collision_windows.size();
  for (const auto& c : seed.clips) constraint_count += c.events.size();
  add(constraint_count <= limits.max_constraint_count, "RESOURCE_CONSTRAINT_COUNT",
      "constraint count " + std::to_string(constraint_count) + " exceeds maximum " + std::to_string(limits.max_constraint_count));
  return result;
}

std::string canonicalize(const SpriteSeed& seed) {
  std::vector<AbilitySeed> abilities = seed.abilities;
  std::ranges::sort(abilities, {}, &AbilitySeed::id);
  std::ostringstream out;
  out << "{\"abilities\":[";
  for (std::size_t i = 0; i < abilities.size(); ++i) {
    if (i) out << ',';
    const auto& a = abilities[i];
    out << "{\"activeTicks\":" << a.active_ticks << ",\"cooldownTicks\":" << a.cooldown_ticks << ",\"cost\":" << a.cost << ",\"effect\":\"" << escape_json(a.effect) << "\",\"id\":\"" << escape_json(a.id) << "\"}";
  }
  out << "],\"animationClips\":" << canonicalize_clips(seed.clips) << ",\"animationStateGraph\":";
  if(seed.animation_graph) out << canonicalize_state_graph(*seed.animation_graph); else out << "null";
  out << ",\"classification\":\"" << escape_json(seed.classification) << "\",\"collisions\":" << canonicalize_collisions(seed.collision_shapes,seed.collision_windows) << ",\"colors\":{\"accent\":\"" << seed.accent_color << "\",\"primary\":\"" << seed.primary_color << "\"";
  if(!seed.storm_primary_color.empty()) out << ",\"stormPrimary\":\"" << seed.storm_primary_color << "\",\"stormAccent\":\"" << seed.storm_accent_color << "\"";
  if(!seed.emissive_color.empty()) out << ",\"emissive\":\"" << seed.emissive_color << "\"";
  if(!seed.aura_color.empty()) out << ",\"aura\":\"" << seed.aura_color << "\"";
  out << "},\"entropyRoot\":" << seed.entropy_root << ",\"id\":\"" << escape_json(seed.stable_id) << "\",\"name\":\"" << escape_json(seed.name) << "\",\"rig\":";
  if(seed.rig) out << canonicalize_rig(*seed.rig); else out << "null";
  out << ",\"rights\":\"" << rights_text(seed.rights) << "\",\"schema\":\"" << seed.schema << "\"}";
  return out.str();
}

std::string sha256(std::string_view input) {
  std::vector<std::uint8_t> data(input.begin(), input.end());
  const auto bit_length = static_cast<std::uint64_t>(data.size()) * 8;
  data.push_back(0x80);
  while ((data.size() % 64) != 56) data.push_back(0);
  for (int i = 7; i >= 0; --i) data.push_back(static_cast<std::uint8_t>(bit_length >> (i * 8)));
  std::array<std::uint32_t, 8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  for (std::size_t offset = 0; offset < data.size(); offset += 64) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) w[i] = (std::uint32_t(data[offset+i*4])<<24)|(std::uint32_t(data[offset+i*4+1])<<16)|(std::uint32_t(data[offset+i*4+2])<<8)|data[offset+i*4+3];
    for (std::size_t i = 16; i < 64; ++i) { const auto s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3); const auto s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10); w[i]=w[i-16]+s0+w[i-7]+s1; }
    auto [a,b,c,d,e,f,g,hh]=h;
    for (std::size_t i=0;i<64;++i){const auto s1=rotr(e,6)^rotr(e,11)^rotr(e,25);const auto ch=(e&f)^((~e)&g);const auto t1=hh+s1+ch+k[i]+w[i];const auto s0=rotr(a,2)^rotr(a,13)^rotr(a,22);const auto maj=(a&b)^(a&c)^(b&c);const auto t2=s0+maj;hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
  }
  std::ostringstream out; out << std::hex << std::setfill('0'); for (const auto value : h) out << std::setw(8) << value; return out.str();
}

SpriteIr compile(const SpriteSeed& seed) {
  const auto validation = validate(seed);
  if (!validation.ok()) throw std::runtime_error(validation.diagnostics.front().code + ": " + validation.diagnostics.front().message);
  const GeneRegistry registry;
  const auto genes = genes_from_seed(seed);
  const auto gene_validation = registry.validate(genes);
  if (!gene_validation.ok()) throw std::runtime_error(gene_validation.diagnostics.front().code + ": " + gene_validation.diagnostics.front().message);
  std::vector<FormDefinition> form_defs;
  for (const auto& f : seed.forms) form_defs.push_back({f.id, f.transformation_ids});
  std::vector<TransformationDelta> trans_deltas;
  for (const auto& t : seed.transformations) trans_deltas.push_back({t.id, t.from_form, t.to_form, t.trigger_condition});
  std::vector<AnimationIntent> anim_intents;
  RuntimeAttributes rt;
  if (seed.runtime) {
    rt = *seed.runtime;
    for (const auto& intent : seed.runtime->animation_intents) anim_intents.push_back({intent.first, intent.second});
  }
  SpriteIr ir;
  ir.seed_identity = sha256(canonicalize(seed));
  ir.entity_id = seed.stable_id;
  ir.name = seed.name;
  ir.classification = seed.classification;
  ir.rights = seed.rights;
  ir.provenance_hash = ir.seed_identity;
  ir.schema_version = seed.schema;
  ir.abilities = seed.abilities;
  ir.primary_color = seed.primary_color;
  ir.accent_color = seed.accent_color;
  ir.storm_primary_color = seed.storm_primary_color;
  ir.storm_accent_color = seed.storm_accent_color;
  ir.emissive_color = seed.emissive_color;
  ir.aura_color = seed.aura_color;
  ir.rig = seed.rig;
  ir.clips = seed.clips;
  ir.animation_graph = seed.animation_graph;
  ir.collision_shapes = seed.collision_shapes;
  ir.collision_windows = seed.collision_windows;
  ir.form_definitions = std::move(form_defs);
  ir.form_attributes = seed.form_attributes;
  ir.transformation_deltas = std::move(trans_deltas);
  ir.morphology = seed.morphology;
  ir.form_morphology_overrides = seed.form_morphology_overrides;
  ir.animation_intents = std::move(anim_intents);
  ir.storm_abilities = seed.storm_abilities;
  ir.runtime = rt;
  return ir;
}

bool activate(RuntimeEntity& entity, const AbilitySeed& ability) {
  if (entity.state != RuntimeState::idle || entity.cooldown_ticks != 0 || entity.energy < ability.cost) return false;
  entity.energy -= ability.cost; entity.state = RuntimeState::attacking; entity.remaining_ticks = ability.active_ticks; entity.cooldown_ticks = ability.cooldown_ticks; return true;
}

void tick(RuntimeEntity& entity) noexcept {
  if (entity.cooldown_ticks > 0) --entity.cooldown_ticks;
  if (entity.remaining_ticks > 0 && --entity.remaining_ticks == 0) entity.state = RuntimeState::recovering;
  else if (entity.state == RuntimeState::recovering) entity.state = RuntimeState::idle;
}

std::string render_svg(const SpriteIr& ir) {
  std::ostringstream out;
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"256\" height=\"256\" viewBox=\"0 0 256 256\" role=\"img\" aria-label=\"" << ir.entity_id << "\">"
      << "<g fill=\"" << ir.primary_color << "\" stroke=\"" << ir.accent_color << "\" stroke-width=\"8\" stroke-linejoin=\"round\">"
      << "<path d=\"M52 122 28 62l62 34c25-18 55-18 78 0l60-34-22 64c12 18 10 49-7 67-21 23-112 23-137 0-17-18-20-48-10-71Z\"/>"
      << "<path fill=\"none\" d=\"m106 137 18 17 27-33-10 31 20 5-35 39 11-31-25-5Z\"/>"
      << "<circle cx=\"91\" cy=\"128\" r=\"7\"/><circle cx=\"169\" cy=\"128\" r=\"7\"/></g></svg>";
  return out.str();
}

static void build_package_internal(const SpriteSeed& seed, std::span<const FrameSource> frames, const SpriteSheetOptions& options, std::string_view visual_metadata, std::span<const ChannelMap> channel_maps, std::string_view channel_metadata, const PackageGovernanceEvidence& governance, const std::filesystem::path& output);

void build_package(const SpriteSeed& seed, const std::filesystem::path& output) {
  build_package_internal(seed, {}, {}, {}, {}, {}, {}, output);
}

void build_package(const SpriteSeed& seed, const PackageGovernanceEvidence& governance, const std::filesystem::path& output) {
  build_package_internal(seed, {}, {}, {}, {}, {}, governance, output);
}

void build_package(const SpriteSeed& seed, std::span<const FrameSource> frames, const SpriteSheetOptions& options, const std::filesystem::path& output) {
  build_package_internal(seed, frames, options, {}, {}, {}, {}, output);
}

void build_package(const SpriteSeed& seed, const AuthoredVisualSet& visual_set, const std::filesystem::path& output) {
  if(visual_set.canonical_metadata.empty()||visual_set.canonical_channel_metadata.empty())throw std::invalid_argument("authored visual set lacks canonical projection metadata");
  build_package_internal(seed,visual_set.frames,visual_set.sheet,visual_set.canonical_metadata,visual_set.channel_maps,visual_set.canonical_channel_metadata,{},output);
}

static void build_package_internal(const SpriteSeed& seed, std::span<const FrameSource> frames, const SpriteSheetOptions& options, std::string_view visual_metadata, std::span<const ChannelMap> channel_maps, std::string_view channel_metadata, const PackageGovernanceEvidence& governance, const std::filesystem::path& output) {
  const auto limits_validation = enforce_resource_limits(seed);
  if (!limits_validation.ok()) throw std::invalid_argument(limits_validation.diagnostics.front().code + ": " + limits_validation.diagnostics.front().message);
  const auto ir = compile(seed); const auto canonical = canonicalize(seed); const auto svg = render_svg(ir);
  const auto authoring_evidence_valid =
      governance.authoring_provenance_json == "{\"project\":null,\"references\":[]}" ||
      (governance.authoring_provenance_json.starts_with("{\"project\":{") &&
       governance.authoring_provenance_json.find("},\"references\":[") != std::string::npos &&
       governance.authoring_provenance_json.ends_with("]}"));
  const auto target_evidence_valid =
      governance.target_compatibility_json.starts_with("{\"reports\":[") &&
      governance.target_compatibility_json.ends_with("]}");
  if (!authoring_evidence_valid || !target_evidence_valid ||
      governance.authoring_provenance_json.size() > 4ULL * 1024ULL * 1024ULL ||
      governance.target_compatibility_json.size() > 4ULL * 1024ULL * 1024ULL)
    throw std::invalid_argument("package governance evidence is malformed or oversized");
  const auto rights = evaluate_rights(seed.rights, AssetUsage::commercial_export);
  if (!rights.allowed) throw std::runtime_error(rights.code + ": " + rights.explanation);
  if (output.empty()) throw std::invalid_argument("output path must not be empty");
  if (std::filesystem::exists(output)) throw std::runtime_error("output already exists; refusing to overwrite: " + output.string());
  auto staging = output; staging += ".staging";
  if (std::filesystem::exists(staging)) throw std::runtime_error("staging path already exists: " + staging.string());
  std::filesystem::create_directories(staging / "assets");
  auto write = [](const auto& path, std::string_view bytes) { std::ofstream file(path, std::ios::binary); if (!file) throw std::runtime_error("cannot write " + path.string()); file.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); if (!file) throw std::runtime_error("failed writing " + path.string()); };
  try {
    const ProvenanceRecord seed_provenance{"prov-seed-" + ir.seed_identity, ProvenanceActor::user, "user", "author-seed/1", {}, ir.seed_identity};
    AssetGraph graph;
    const auto seed_artifact = graph.add("application/vnd.gspl.sprite-seed+json", canonical, {}, "canonicalize-seed/1", seed_provenance.id, "portable", ArtifactValidation::valid);
    const ProvenanceRecord svg_provenance{"prov-svg-" + sha256(svg), ProvenanceActor::compiler, "gspl-sprites/0.1.0", "render-svg/1", {seed_artifact}, sha256(svg)};
    (void)graph.add("image/svg+xml", svg, {seed_artifact}, "render-svg/1", svg_provenance.id, "svg", ArtifactValidation::valid);
    write(staging / "seed.canonical.json", canonical); write(staging / "assets" / "entity.svg", svg);
    std::vector<ProvenanceRecord> provenance_records{seed_provenance,svg_provenance};
    std::vector<std::pair<std::string,std::string>> manifest_artifacts{{"assets/entity.svg",sha256(svg)},{"seed.canonical.json",sha256(canonical)}};
    const auto add_semantic_artifact=[&](std::string path,std::string type,std::string pass,std::string_view bytes,std::string target,std::vector<std::string> dependencies){const auto hash=sha256(bytes);const ProvenanceRecord provenance{"prov-"+pass+"-"+hash,ProvenanceActor::compiler,"gspl-sprites/0.1.0",pass,dependencies,hash};(void)graph.add(std::move(type),bytes,dependencies,pass,provenance.id,std::move(target),ArtifactValidation::valid);write(staging/path,bytes);provenance_records.push_back(provenance);manifest_artifacts.emplace_back(std::move(path),hash);};
    add_semantic_artifact("authoring-provenance.json","application/vnd.gspl.sprite-authoring-provenance+json","bind-authoring-provenance/1",governance.authoring_provenance_json,"portable",{seed_artifact});
    add_semantic_artifact("target-compatibility.json","application/vnd.gspl.sprite-target-compatibility+json","evaluate-target-contracts/1",governance.target_compatibility_json,"portable",{seed_artifact});
    if(ir.rig){
      const auto rig_json=canonicalize_rig(*ir.rig);const auto clips_json=canonicalize_clips(ir.clips);const auto graph_json=ir.animation_graph?canonicalize_state_graph(*ir.animation_graph):"null";const auto collisions_json=canonicalize_collisions(ir.collision_shapes,ir.collision_windows);
      add_semantic_artifact("rig.json","application/vnd.gspl.sprite-rig+json","lower-rig/1",rig_json,"portable",{seed_artifact});
      add_semantic_artifact("animations.json","application/vnd.gspl.sprite-animation+json","lower-animation/1",clips_json,"portable",{seed_artifact});
      if(ir.animation_graph)add_semantic_artifact("animation-state-graph.json","application/vnd.gspl.sprite-animation-graph+json","lower-animation-graph/1",graph_json,"portable",{seed_artifact});
      add_semantic_artifact("collisions.json","application/vnd.gspl.sprite-collision+json","lower-collision/1",collisions_json,"portable",{seed_artifact});
    }
    std::map<std::string,std::string,std::less<>> frame_artifact_by_id;
    if(!frames.empty()){
      std::vector<std::string> frame_artifacts;frame_artifacts.reserve(frames.size());
      for(const auto&frame:frames){if(frame.id.empty()||!frame.image.invariant())throw std::invalid_argument("visual source frame is invalid");std::ostringstream header;header<<"{\"alphaMode\":"<<static_cast<int>(frame.image.alpha_mode)<<",\"colorSpace\":"<<static_cast<int>(frame.image.color_space)<<",\"durationTicks\":"<<frame.duration_ticks<<",\"height\":"<<frame.image.height<<",\"id\":\""<<escape_json(frame.id)<<"\",\"pivotX\":"<<frame.pivot_x<<",\"pivotY\":"<<frame.pivot_y<<",\"width\":"<<frame.image.width<<"}\n";std::string bytes=header.str();bytes.append(reinterpret_cast<const char*>(frame.image.pixels.data()),frame.image.pixels.size());const auto hash=sha256(bytes);const ProvenanceRecord provenance{"prov-frame-source-"+hash,ProvenanceActor::user,"user","ingest-frame/1",{},hash};const auto artifact=graph.add("application/vnd.gspl.sprite-frame+rgba8",bytes,{},"ingest-frame/1",provenance.id,"canonical-2d",ArtifactValidation::valid);provenance_records.push_back(provenance);frame_artifacts.push_back(artifact);frame_artifact_by_id.emplace(frame.id,artifact);}
      auto dependencies=frame_artifacts;dependencies.push_back(seed_artifact);std::ranges::sort(dependencies);const auto sheet=compile_sprite_sheet(frames,options);const auto atlas_png=encode_png(sheet.atlas.image);const auto alpha_png=encode_png(sheet.alpha);const auto outline_png=encode_png(sheet.outline);const auto view=[](const std::vector<std::byte>& bytes){return std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size());};
      add_semantic_artifact("assets/sprite-atlas.png","image/png","compile-atlas/1",view(atlas_png),"png",dependencies);
      add_semantic_artifact("assets/sprite-alpha-mask.png","image/png","compile-alpha-mask/1",view(alpha_png),"png",dependencies);
      add_semantic_artifact("assets/sprite-outline-mask.png","image/png","compile-outline-mask/1",view(outline_png),"png",dependencies);
      add_semantic_artifact("atlas.json","application/vnd.gspl.sprite-atlas+json","emit-atlas-metadata/1",sheet.metadata,"portable",dependencies);
      if(!visual_metadata.empty())add_semantic_artifact("visual-projection.json","application/vnd.gspl.sprite-visual-projection+json","emit-visual-projection/1",visual_metadata,"portable",dependencies);
    }
    if(!channel_maps.empty()){
      std::filesystem::create_directories(staging/"assets"/"channels");std::vector<std::string> map_artifacts;map_artifacts.reserve(channel_maps.size());
      for(const auto&map:channel_maps){const auto target=std::ranges::find(frames,map.target_frame_id,&FrameSource::id);if(target==frames.end())throw std::invalid_argument("channel map target is absent");const auto validation=validate_channel_map(map,*target);if(!validation.ok())throw std::invalid_argument(validation.diagnostics.front().code+": "+validation.diagnostics.front().message);std::ostringstream header;header<<"{\"colorSpace\":"<<static_cast<int>(map.image.color_space)<<",\"height\":"<<map.image.height<<",\"id\":\""<<escape_json(map.id)<<"\",\"kind\":"<<static_cast<int>(map.kind)<<",\"target\":\""<<escape_json(map.target_frame_id)<<"\",\"width\":"<<map.image.width<<"}\n";std::string source_bytes=header.str();source_bytes.append(reinterpret_cast<const char*>(map.image.pixels.data()),map.image.pixels.size());const auto source_hash=sha256(source_bytes);const ProvenanceRecord source_provenance{"prov-channel-source-"+source_hash,ProvenanceActor::user,"user","ingest-channel-map/1",{},source_hash};const auto source_artifact=graph.add("application/vnd.gspl.sprite-channel-map+rgba8",source_bytes,{},"ingest-channel-map/1",source_provenance.id,"canonical-2d",ArtifactValidation::valid);provenance_records.push_back(source_provenance);map_artifacts.push_back(source_artifact);const auto png=encode_png(map.image);const auto bytes=std::string_view(reinterpret_cast<const char*>(png.data()),png.size());std::vector dependencies{seed_artifact,source_artifact,frame_artifact_by_id.at(map.target_frame_id)};std::ranges::sort(dependencies);add_semantic_artifact("assets/channels/"+map.id+".png","image/png","encode-channel-map/1",bytes,"png",dependencies);}
      auto dependencies=map_artifacts;dependencies.push_back(seed_artifact);std::ranges::sort(dependencies);add_semantic_artifact("channel-maps.json","application/vnd.gspl.sprite-channel-maps+json","emit-channel-maps/1",channel_metadata,"portable",dependencies);
    }
    std::vector<TargetRequirement> package_requirements{
        {TargetFeature::canonical_seed, true},
        {TargetFeature::rights_and_provenance, true}};
    if(!frames.empty())package_requirements.push_back({TargetFeature::raster_2d,true});
    if(ir.rig)package_requirements.push_back({TargetFeature::skeletal_2d,true});

    // Enforce package size limit before rename
    {
      std::uint64_t total_bytes = 0;
      for (const auto& entry : std::filesystem::recursive_directory_iterator(staging)) {
        if (entry.is_regular_file())
          total_bytes += static_cast<std::uint64_t>(entry.file_size());
      }
      ResourceLimits pkg_rl;
      if (total_bytes > pkg_rl.max_package_bytes)
        throw std::runtime_error("RESOURCE_PACKAGE_SIZE: package size " +
            std::to_string(total_bytes) + " bytes exceeds maximum " +
            std::to_string(pkg_rl.max_package_bytes) + " bytes");
    }
    if(ir.animation_graph)package_requirements.push_back({TargetFeature::animation_graph,true});
    if(!ir.collision_shapes.empty()||!ir.collision_windows.empty())package_requirements.push_back({TargetFeature::collision_2d,true});
    if(!channel_maps.empty())package_requirements.push_back({TargetFeature::channel_maps,true});
    const auto package_requirements_json=canonicalize_target_requirements(package_requirements);
    const auto package_report_json=canonicalize_target_compatibility(evaluate_target_compatibility(builtin_target_adapter("portable-package"),package_requirements));
    add_semantic_artifact("package-target-requirements.json","application/vnd.gspl.sprite-target-requirements+json","declare-package-requirements/1",package_requirements_json,"portable",{seed_artifact});
    add_semantic_artifact("package-target-report.json","application/vnd.gspl.sprite-target-compatibility+json","evaluate-package-target/1",package_report_json,"portable",{seed_artifact});
    const auto asset_graph_json=graph.canonical_manifest();write(staging / "asset-graph.json",asset_graph_json);manifest_artifacts.emplace_back("asset-graph.json",sha256(asset_graph_json));
    std::ranges::sort(provenance_records,{},&ProvenanceRecord::id);std::ostringstream provenance_json;provenance_json<<"{\"records\":[";for(std::size_t i=0;i<provenance_records.size();++i){if(i)provenance_json<<',';provenance_json<<canonical_provenance(provenance_records[i]);}provenance_json<<"]}";write(staging / "provenance.json",provenance_json.str());manifest_artifacts.emplace_back("provenance.json",sha256(provenance_json.str()));
    const auto rights_json="{\"classification\":\"" + rights_text(seed.rights) + "\",\"commercialExport\":true,\"decisionCode\":\"" + rights.code + "\"}";write(staging / "rights.json",rights_json);manifest_artifacts.emplace_back("rights.json",sha256(rights_json));
    std::ranges::sort(manifest_artifacts);std::ostringstream manifest; manifest << "{\"artifacts\":[";for(std::size_t i=0;i<manifest_artifacts.size();++i){if(i)manifest<<',';manifest<<"{\"path\":\""<<manifest_artifacts[i].first<<"\",\"sha256\":\""<<manifest_artifacts[i].second<<"\"}";}manifest << "],\"assetGraph\":\"asset-graph.json\",\"entityId\":\"" << ir.entity_id << "\",\"format\":\"gspl.sprite-package/0.1\",\"provenance\":\"provenance.json\",\"rights\":\"rights.json\",\"seedIdentity\":\"" << ir.seed_identity << "\"}";
    write(staging / "manifest.json", manifest.str());
    std::filesystem::rename(staging, output);
  } catch (...) {
    std::error_code ignored; std::filesystem::remove_all(staging, ignored); throw;
  }
}
} // namespace gspl::sprites
