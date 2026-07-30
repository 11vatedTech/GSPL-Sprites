#include "gspl_sprites/package.hpp"

#include "gspl/json.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/image.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/target_contract.hpp"
#include "package_semantics.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <map>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
struct ArtifactEntry { std::string path; std::string hash; };
struct ParsedManifest { std::vector<ArtifactEntry> artifacts; std::string entity_id; std::string seed_identity; };

class ManifestParser final {
public:
  explicit ManifestParser(std::string_view source, std::uint32_t max_artifacts) : source_(source), max_artifacts_(max_artifacts) {}
  ParsedManifest parse() {
    ParsedManifest result;
    expect("{\"artifacts\":[");
    if (!consume(']')) {
      for (;;) {
        if (result.artifacts.size() >= max_artifacts_) fail("artifact count exceeds limit");
        expect("{\"path\":"); const auto path=string(); expect(",\"sha256\":"); const auto hash=string(); expect("}"); result.artifacts.push_back({path,hash});
        if (consume(']')) break;
        expect(",");
      }
    }
    expect(",\"assetGraph\":\"asset-graph.json\",\"entityId\":"); result.entity_id=string();
    expect(",\"format\":\"gspl.sprite-package/0.1\",\"provenance\":\"provenance.json\",\"rights\":\"rights.json\",\"seedIdentity\":"); result.seed_identity=string(); expect("}");
    if (cursor_ != source_.size()) fail("trailing manifest data");
    return result;
  }
private:
  bool consume(char value) { if(cursor_<source_.size()&&source_[cursor_]==value){++cursor_;return true;}return false; }
  void expect(std::string_view value) { if(source_.substr(cursor_,value.size())!=value)fail("canonical manifest token mismatch");cursor_+=value.size(); }
  std::string string() {
    if(!consume('"'))fail("expected JSON string");
    std::string result;
    while(cursor_<source_.size()){
      const unsigned char c=static_cast<unsigned char>(source_[cursor_++]);
      if(c=='"')return result;
      if(c<0x20)fail("control character in JSON string");
      if(c!='\\'){result+=static_cast<char>(c);continue;}
      if(cursor_>=source_.size())fail("truncated JSON escape");
      const char escaped=source_[cursor_++];
      switch(escaped){case '"':result+='"';break;case '\\':result+='\\';break;case 'n':result+='\n';break;case 'r':result+='\r';break;case 't':result+='\t';break;default:fail("non-canonical JSON escape");}
    }
    fail("unterminated JSON string");
  }
  [[noreturn]] void fail(std::string_view message) const { throw std::runtime_error(std::string(message)+" at byte "+std::to_string(cursor_)); }
  std::string_view source_; std::size_t cursor_{}; std::uint32_t max_artifacts_{};
};

std::string read_bounded(const std::filesystem::path& path, std::uint64_t maximum) {
  const auto size=std::filesystem::file_size(path);if(size>maximum||size>std::numeric_limits<std::size_t>::max())throw std::runtime_error("file exceeds byte limit: "+path.string());
  std::string bytes(static_cast<std::size_t>(size),'\0');std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("cannot open file: "+path.string());input.read(bytes.data(),static_cast<std::streamsize>(bytes.size()));if(!input&& !bytes.empty())throw std::runtime_error("cannot read complete file: "+path.string());return bytes;
}

bool lowercase_sha256(std::string_view value) { return value.size()==64&&std::ranges::all_of(value,[](unsigned char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}); }

bool safe_relative_path(std::string_view value, const PackageLimits& limits) {
  if(value.empty()||value.size()>limits.max_path_bytes||value.front()=='/'||value.back()=='/'||value.find('\\')!=std::string_view::npos||value.find(':')!=std::string_view::npos)return false;
  if(!std::ranges::all_of(value,[](unsigned char c){return c>=0x20&&c<0x7f;}))return false;
  std::size_t start=0;while(start<value.size()){const auto end=value.find('/',start);const auto part=value.substr(start,end==std::string_view::npos?value.size()-start:end-start);if(part.empty()||part=="."||part=="..")return false;if(end==std::string_view::npos)break;start=end+1;}return true;
}

bool allowed_rights_document(std::string_view value) {
  static constexpr std::array classifications{"ORIGINAL_USER_CREATION","USER_OWNED_REFERENCE","LICENSED_REFERENCE","PUBLIC_DOMAIN","PERMISSIVELY_LICENSED"};
  return std::ranges::any_of(classifications,[&](std::string_view classification){return value=="{\"classification\":\""+std::string(classification)+"\",\"commercialExport\":true,\"decisionCode\":\"SPRITE_RIGHTS_ALLOWED\"}";});
}

bool plausible_authoring_provenance(std::string_view value) {
  return value == "{\"project\":null,\"references\":[]}" ||
         (value.starts_with("{\"project\":{") &&
          value.find("},\"references\":[") != std::string_view::npos &&
          value.ends_with("]}"));
}

bool plausible_target_compatibility(std::string_view value) {
  return value.starts_with("{\"reports\":[") && value.ends_with("]}");
}

bool package_feature_present(TargetFeature feature,
                             const std::set<std::string> &declared) {
  switch(feature){
  case TargetFeature::canonical_seed:return declared.contains("seed.canonical.json");
  case TargetFeature::rights_and_provenance:return declared.contains("rights.json")&&declared.contains("provenance.json")&&declared.contains("asset-graph.json");
  case TargetFeature::raster_2d:return declared.contains("assets/sprite-atlas.png")&&declared.contains("assets/sprite-alpha-mask.png")&&declared.contains("assets/sprite-outline-mask.png")&&declared.contains("atlas.json");
  case TargetFeature::skeletal_2d:return declared.contains("rig.json")&&declared.contains("animations.json");
  case TargetFeature::animation_graph:return declared.contains("animation-state-graph.json");
  case TargetFeature::collision_2d:return declared.contains("collisions.json");
  case TargetFeature::channel_maps:return declared.contains("channel-maps.json")&&std::ranges::any_of(declared,[](const auto& path){return path.starts_with("assets/channels/");});
  default:return false;
  }
}
}

PackageVerification verify_package(const std::filesystem::path& root, const PackageLimits& limits) {
  PackageVerification result;auto add=[&](std::string code,std::string message){result.validation.diagnostics.push_back({std::move(code),std::move(message)});};
  try {
    if(root.empty()||!std::filesystem::exists(root)||!std::filesystem::is_directory(root)||std::filesystem::is_symlink(std::filesystem::symlink_status(root))){add("SPRITE_PACKAGE_ROOT_INVALID","package root must be a present non-symlink directory");return result;}
    const auto manifest_path=root/"manifest.json";if(!std::filesystem::is_regular_file(manifest_path)||std::filesystem::is_symlink(std::filesystem::symlink_status(manifest_path))){add("SPRITE_PACKAGE_MANIFEST_MISSING","canonical manifest is absent or unsafe");return result;}
    const auto manifest_bytes=read_bounded(manifest_path,limits.max_manifest_bytes);result.package_identity=sha256(manifest_bytes);const auto manifest=ManifestParser(manifest_bytes,limits.max_artifacts).parse();result.entity_id=manifest.entity_id;result.seed_identity=manifest.seed_identity;result.artifact_count=static_cast<std::uint32_t>(manifest.artifacts.size());
    if(result.entity_id.empty()||!lowercase_sha256(result.seed_identity))add("SPRITE_PACKAGE_IDENTITY_INVALID","entity or seed identity is invalid");
    std::set<std::string> declared;std::string previous;
    for(const auto& artifact:manifest.artifacts){
      if(!safe_relative_path(artifact.path,limits)){add("SPRITE_PACKAGE_PATH_UNSAFE","unsafe artifact path: "+artifact.path);continue;}
      if(!declared.insert(artifact.path).second){add("SPRITE_PACKAGE_PATH_DUPLICATE","duplicate artifact path: "+artifact.path);continue;}
      if(!previous.empty()&&artifact.path<=previous)add("SPRITE_PACKAGE_MANIFEST_NONCANONICAL","artifact paths are not strictly sorted");
      previous=artifact.path;
      if(!lowercase_sha256(artifact.hash)){add("SPRITE_PACKAGE_HASH_INVALID","invalid SHA-256 for: "+artifact.path);continue;}
      auto current=root;bool unsafe_component=false;std::size_t start=0;while(start<artifact.path.size()){const auto end=artifact.path.find('/',start);const auto part=artifact.path.substr(start,end==std::string_view::npos?artifact.path.size()-start:end-start);current/=part;if(std::filesystem::exists(current)&&std::filesystem::is_symlink(std::filesystem::symlink_status(current))){unsafe_component=true;break;}if(end==std::string_view::npos)break;start=end+1;}
      if(unsafe_component||!std::filesystem::is_regular_file(current)){add("SPRITE_PACKAGE_ARTIFACT_UNSAFE","artifact is absent, non-regular, or traverses a symlink: "+artifact.path);continue;}
      const auto bytes=read_bounded(current,limits.max_artifact_bytes);if(bytes.size()>limits.max_total_bytes||result.total_artifact_bytes>limits.max_total_bytes-bytes.size()){add("SPRITE_PACKAGE_TOTAL_LIMIT","package exceeds total byte limit");continue;}result.total_artifact_bytes+=bytes.size();if(sha256(bytes)!=artifact.hash)add("SPRITE_PACKAGE_HASH_MISMATCH","artifact hash mismatch: "+artifact.path);
    }
    static constexpr std::array required{"asset-graph.json","assets/entity.svg","authoring-provenance.json","package-target-report.json","package-target-requirements.json","provenance.json","rights.json","seed.canonical.json","target-compatibility.json"};for(const auto path:required)if(!declared.contains(path))add("SPRITE_PACKAGE_REQUIRED_ARTIFACT_MISSING","required artifact is undeclared: "+std::string(path));
    if(declared.contains("seed.canonical.json")){const auto seed=read_bounded(root/"seed.canonical.json",limits.max_artifact_bytes);if(sha256(seed)!=result.seed_identity)add("SPRITE_PACKAGE_SEED_IDENTITY_MISMATCH","canonical seed does not match seed identity");}
    if(declared.contains("rights.json")){const auto rights=read_bounded(root/"rights.json",limits.max_artifact_bytes);if(!allowed_rights_document(rights))add("SPRITE_PACKAGE_RIGHTS_DENIED","rights document does not authorize commercial export");}
    if(declared.contains("authoring-provenance.json")){const auto evidence=read_bounded(root/"authoring-provenance.json",limits.max_artifact_bytes);if(!plausible_authoring_provenance(evidence))add("SPRITE_PACKAGE_AUTHORING_PROVENANCE_INVALID","authoring provenance document is not canonical evidence");}
    if(declared.contains("target-compatibility.json")){const auto evidence=read_bounded(root/"target-compatibility.json",limits.max_artifact_bytes);if(!plausible_target_compatibility(evidence))add("SPRITE_PACKAGE_TARGET_COMPATIBILITY_INVALID","target compatibility document is not canonical evidence");}
    if(declared.contains("package-target-requirements.json")&&declared.contains("package-target-report.json")){const auto requirements=parse_target_requirements(read_bounded(root/"package-target-requirements.json",limits.max_artifact_bytes));const auto expected=canonicalize_target_compatibility(evaluate_target_compatibility(builtin_target_adapter("portable-package"),requirements));if(read_bounded(root/"package-target-report.json",limits.max_artifact_bytes)!=expected)add("SPRITE_PACKAGE_TARGET_REPORT_MISMATCH","portable package target report does not match requirements");for(const auto& requirement:requirements)if(requirement.required&&!package_feature_present(requirement.feature,declared))add("SPRITE_PACKAGE_TARGET_STRUCTURE_MISSING","portable package lacks required feature artifacts: "+std::string(target_feature_name(requirement.feature)));}
    if(declared.contains("asset-graph.json")&&declared.contains("provenance.json")){const auto graph=read_bounded(root/"asset-graph.json",limits.max_artifact_bytes);const auto provenance=read_bounded(root/"provenance.json",limits.max_artifact_bytes);const auto closure=validate_package_semantic_closure(graph,provenance,limits.max_artifacts);for(const auto& diagnostic:closure.diagnostics)add(diagnostic.code,diagnostic.message);}
    std::set<std::string> actual;std::uint32_t directory_entries=0;for(const auto& entry:std::filesystem::recursive_directory_iterator(root)){if(++directory_entries>limits.max_directory_entries){add("SPRITE_PACKAGE_ENTRY_LIMIT","package directory entry count exceeds limit");break;}if(std::filesystem::is_symlink(entry.symlink_status())){add("SPRITE_PACKAGE_SYMLINK","package contains a symlink");continue;}if(entry.is_regular_file()){const auto relative=entry.path().lexically_relative(root).generic_string();if(relative!="manifest.json")actual.insert(relative);}}
    if(actual!=declared)add("SPRITE_PACKAGE_FILE_SET_MISMATCH","package contains undeclared files or declared files are absent");
  } catch(const std::exception& error){add("SPRITE_PACKAGE_MALFORMED",error.what());}
  return result;
}

/* ---------- Living Visual Package: Builder / Reader / Verifier ---------- */

namespace {

/* -- Escape helpers -- */
std::string lv_escape(std::string_view v) {
  std::string o; for (unsigned char c : v) { switch (c) { case '\\': o+="\\\\"; break; case '"': o+="\\\""; break; case '\n': o+="\\n"; break; case '\r': o+="\\r"; break; case '\t': o+="\\t"; break; default: if (c<0x20) throw std::runtime_error("control char"); o+=static_cast<char>(c); }} return o;
}

/* -- File I/O -- */
void lv_write(const std::filesystem::path& p, std::string_view bytes) {
  std::ofstream f(p, std::ios::binary); if (!f) throw std::runtime_error("write fail: "+p.string());
  f.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); if (!f) throw std::runtime_error("write incomplete: "+p.string());
}
std::string lv_read(const std::filesystem::path& p, std::uint64_t max) {
  auto sz = std::filesystem::file_size(p); if (sz > max) throw std::runtime_error("oversize: "+p.string());
  std::string d(static_cast<std::size_t>(sz), '\0');
  std::ifstream in(p, std::ios::binary); if (!in) throw std::runtime_error("open fail: "+p.string());
  in.read(d.data(), static_cast<std::streamsize>(sz)); return d;
}

/* -- INJECTIVE hex path codec: "id-" + lowercase hex of every byte -- */
std::string encode_pc(std::string_view id) {
  if (id.size() > 200) throw std::runtime_error("input too long for path encoding");
  std::string out = "id-";
  out.reserve(3 + id.size() * 2);
  for (unsigned char c : id) {
    char hex[3]{}; auto [ptr,ec] = std::to_chars(hex, hex+2, c, 16);
    if (ec != std::errc{}) throw std::runtime_error("path encoding failed");
    if (hex[1] == '\0') { out += '0'; out += hex[0]; }
    else { out += hex[0]; out += hex[1]; }
  }
  if (out == "id-" || out.size() > 512) throw std::runtime_error("invalid encoded component");
  return out;
}

std::string lv_morph_json(const EffectiveMorphology& m) {
  std::ostringstream o; o << "{"; bool fst=true;
  for (auto const& [n, p] : m) {
    if (!fst) o << ","; fst=false;
    o << "\"" << lv_escape(n) << "\":{\"bone_id\":\"" << lv_escape(p.bone_id)
      << "\",\"color\":\"" << lv_escape(p.color) << "\",\"electrical_marking\":" << (p.electrical_marking?"true":"false")
      << ",\"emissive\":" << (p.emissive?"true":"false") << ",\"parent\":\"" << lv_escape(p.parent)
      << "\",\"primitive\":\"" << lv_escape(p.primitive) << "\",\"rotation_degrees\":" << p.rotation_degrees
      << ",\"semantic_role\":\"" << lv_escape(p.semantic_role) << "\",\"size_x\":" << p.size_x
      << ",\"size_y\":" << p.size_y << ",\"size_z\":" << p.size_z << ",\"x\":" << p.x << ",\"y\":" << p.y
      << ",\"z\":" << p.z << ",\"z_order\":" << p.z_order << "}";
  }
  o << "}"; return o.str();
}

void lv_validate_input(const LivingVisualPackageInput& in) {
  auto add = [](auto& diags, std::string code, std::string msg) { diags.push_back({std::move(code), std::move(msg)}); };
  std::vector<Diagnostic> diags;
  if (in.frames.size() != 48) add(diags, "LV_INPUT_FRAMES", "expected 48 frames");
  if (in.generated_clips.size() != 9) add(diags, "LV_INPUT_CLIPS", "expected 9 clips");
  if (in.samples.size() != 48) add(diags, "LV_INPUT_SAMPLES", "expected 48 samples");
  if (in.transformation_morphologies.size() != 10) add(diags, "LV_INPUT_TRANS_MORPHS", "expected 10 transformation morphologies");
  if (in.base_morphology.empty()) add(diags, "LV_INPUT_BASE_MORPH", "base morphology empty");
  if (in.storm_morphology.empty()) add(diags, "LV_INPUT_STORM_MORPH", "storm morphology empty");
  std::set<std::string> frame_ids, clip_ids;
  for (auto const& f : in.frames) {
    if (f.id.empty()) { add(diags, "LV_INPUT_FRAME_ID", "empty frame id"); continue; }
    if (!frame_ids.insert(f.id).second) add(diags, "LV_INPUT_DUP_FRAME", "duplicate frame id: "+f.id);
  }
  for (auto const& c : in.generated_clips) {
    if (c.id.empty()) { add(diags, "LV_INPUT_CLIP_ID", "empty clip id"); continue; }
    if (!clip_ids.insert(c.id).second) add(diags, "LV_INPUT_DUP_CLIP", "duplicate clip id: "+c.id);
  }
  for (auto const& c : in.generated_clips)
    for (auto const& fid : c.frame_ids)
      if (!frame_ids.contains(fid)) add(diags, "LV_INPUT_CLIP_REF", "clip "+c.id+" references unknown frame: "+fid);
  for (auto const& s : in.samples) {
    if (!frame_ids.contains(s.frame_id)) add(diags, "LV_INPUT_SAMPLE_FRAME", "sample references unknown frame: "+s.frame_id);
    if (!clip_ids.contains(s.clip_id)) add(diags, "LV_INPUT_SAMPLE_CLIP", "sample references unknown clip: "+s.clip_id);
    if (s.frame_hash.size() != 64 || s.pose_hash.size() != 64) add(diags, "LV_INPUT_SAMPLE_HASH", "invalid hash length");
  }
  for (auto const& e : in.events) {
    if (!clip_ids.contains(e.clip_id)) add(diags, "LV_INPUT_EVENT_CLIP", "event references unknown clip: "+e.clip_id);
    if (!frame_ids.contains(e.frame_id)) add(diags, "LV_INPUT_EVENT_FRAME", "event references unknown frame: "+e.frame_id);
  }
  // Check for encoded path collisions
  std::set<std::string> encoded_paths;
  for (auto const& f : in.frames) {
    auto p = "frames/" + encode_pc(f.id) + ".png";
    if (!encoded_paths.insert(p).second) add(diags, "LV_INPUT_PATH_COLLISION", "encoded path collision: "+p);
  }
  for (auto const& ch : in.channels) {
    auto p = "channels/" + encode_pc(ch.id) + ".png";
    if (!encoded_paths.insert(p).second) add(diags, "LV_INPUT_PATH_COLLISION", "encoded path collision: "+p);
    if (!frame_ids.contains(ch.target_frame_id)) add(diags, "LV_INPUT_CH_TARGET", "channel targets unknown frame: "+ch.target_frame_id);
  }
  if (!diags.empty()) {
    std::ostringstream msgs;
    for (auto const& d : diags) msgs << d.code << ": " << d.message << "; ";
    throw std::invalid_argument(msgs.str());
  }
}

// BoundedJsonReader helpers for manifest/manifest-like parsing
static std::string lv_rd_str(gspl::BoundedJsonReader& r, std::string_view /*path*/) {
  auto res = r.read_string_result();
  if (!res.ok()) throw std::runtime_error("expected string");
  return std::move(*res.value);
}
static std::uint32_t lv_rd_u32(gspl::BoundedJsonReader& r, std::string_view /*path*/) {
  auto res = r.read_uint32_result();
  if (!res.ok()) return 0;
  return *res.value;
}
static std::uint64_t lv_rd_u64(gspl::BoundedJsonReader& r, std::string_view /*path*/) {
  auto res = r.read_uint64_result();
  if (!res.ok()) return 0;
  return *res.value;
}
static double lv_rd_dbl(gspl::BoundedJsonReader& r, std::string_view /*path*/) {
  auto res = r.read_double_result();
  if (!res.ok()) return 0.0;
  return *res.value;
}
static bool lv_rd_bool(gspl::BoundedJsonReader& r, std::string_view /*path*/) {
  auto res = r.read_bool_result();
  return res.ok() && *res.value;
}

// Fast key reader: read string into key buffer, match against expected value.
// Used for manifest-level introspection (not deep seed parsing).
static std::string lv_manifest_str(std::string_view json, std::string_view key) {
  gspl::BoundedJsonConfig cfg{};
  gspl::BoundedJsonReader r(json, cfg);
  if (!r.begin_object("manifest")) return {};
  while (r.has_more() && !r.has_error()) {
    auto k = r.read_string_result();
    if (!k.ok()) break;
    if (!r.require(':', "manifest")) break;
    if (*k.value == key) {
      auto v = r.read_string_result();
      return v.ok() ? std::move(*v.value) : std::string{};
    }
    r.skip_value();
    r.record_object_member("manifest");
    if (!r.next_object_member("manifest")) break;
  }
  return {};
}
static std::uint32_t lv_manifest_int(std::string_view json, std::string_view key) {
  gspl::BoundedJsonConfig cfg{};
  gspl::BoundedJsonReader r(json, cfg);
  if (!r.begin_object("manifest")) return 0;
  while (r.has_more() && !r.has_error()) {
    auto k = r.read_string_result();
    if (!k.ok()) break;
    if (!r.require(':', "manifest")) break;
    if (*k.value == key) {
      auto v = r.read_uint32_result();
      return v.ok() ? *v.value : 0;
    }
    r.skip_value();
    r.record_object_member("manifest");
    if (!r.next_object_member("manifest")) break;
  }
  return 0;
}
// Full SpriteSeed deserialization from canonical JSON using BoundedJsonReader
static SpriteSeed lv_parse_seed_json(std::string_view json) {
  SpriteSeed s;
  gspl::BoundedJsonConfig cfg{}; cfg.max_object_members = 8192; cfg.max_array_length = 8192;
  gspl::BoundedJsonReader r(json, cfg);
  if (!r.begin_object("seed")) return s;
  while (r.has_more() && !r.has_error()) {
    auto key = r.read_string_result();
    if (!key.ok()) break;
    if (!r.require(':', "seed")) break;
    auto const& k = *key.value;
    if (k == "id")              { s.stable_id = lv_rd_str(r, "seed.id"); }
    else if (k == "name")       { s.name = lv_rd_str(r, "seed.name"); }
    else if (k == "schema")     { s.schema = lv_rd_str(r, "seed.schema"); }
    else if (k == "classification") { s.classification = lv_rd_str(r, "seed.classification"); }
    else if (k == "rights")     { auto v = lv_rd_str(r, "seed.rights"); if (v == "ORIGINAL_USER_CREATION") s.rights = RightsClass::original_user_creation; else if (v == "USER_OWNED_REFERENCE") s.rights = RightsClass::user_owned; else if (v == "LICENSED_REFERENCE") s.rights = RightsClass::licensed; else if (v == "PUBLIC_DOMAIN") s.rights = RightsClass::public_domain; else if (v == "PERMISSIVELY_LICENSED") s.rights = RightsClass::permissive; }
    else if (k == "entropyRoot") { s.entropy_root = lv_rd_u64(r, "seed.entropyRoot"); }
    else if (k == "colors") {
      if (!r.begin_object("seed.colors")) break;
      while (r.has_more() && !r.has_error()) {
        auto ck = r.read_string_result(); if (!ck.ok()) break;
        if (!r.require(':', "seed.colors")) break;
        if (*ck.value == "primary")     s.primary_color = lv_rd_str(r, "seed.colors.primary");
        else if (*ck.value == "accent") s.accent_color = lv_rd_str(r, "seed.colors.accent");
        else if (*ck.value == "stormPrimary") s.storm_primary_color = lv_rd_str(r, "seed.colors.stormPrimary");
        else if (*ck.value == "stormAccent")  s.storm_accent_color = lv_rd_str(r, "seed.colors.stormAccent");
        else if (*ck.value == "emissive")     s.emissive_color = lv_rd_str(r, "seed.colors.emissive");
        else if (*ck.value == "aura")         s.aura_color = lv_rd_str(r, "seed.colors.aura");
        else r.skip_value();
        r.record_object_member("seed.colors");
        if (!r.next_object_member("seed.colors")) break;
      }
      r.end_object("seed.colors");
    }
    else if (k == "abilities") {
      if (!r.begin_array("seed.abilities")) break;
      while (r.has_more() && !r.has_error()) {
        if (!r.begin_object("ability")) break;
        AbilitySeed a;
        while (r.has_more() && !r.has_error()) {
          auto ak = r.read_string_result(); if (!ak.ok()) break;
          if (!r.require(':', "ability")) break;
          if (*ak.value == "id") a.id = lv_rd_str(r, "ability.id");
          else if (*ak.value == "effect") a.effect = lv_rd_str(r, "ability.effect");
          else if (*ak.value == "cost") a.cost = lv_rd_u32(r, "ability.cost");
          else if (*ak.value == "cooldownTicks") a.cooldown_ticks = lv_rd_u32(r, "ability.cooldownTicks");
          else if (*ak.value == "activeTicks") a.active_ticks = lv_rd_u32(r, "ability.activeTicks");
          else r.skip_value();
          r.record_object_member("ability");
          if (!r.next_object_member("ability")) break;
        }
        r.end_object("ability");
        if (!a.id.empty()) s.abilities.push_back(std::move(a));
        r.record_array_element("seed.abilities");
        if (!r.next_array_element("seed.abilities")) break;
      }
      r.end_array("seed.abilities");
    }
    else if (k == "stormAbilities") {
      if (!r.begin_array("seed.stormAbilities")) break;
      while (r.has_more() && !r.has_error()) {
        if (!r.begin_object("stormAbility")) break;
        AbilitySeed a;
        while (r.has_more() && !r.has_error()) {
          auto ak = r.read_string_result(); if (!ak.ok()) break;
          if (!r.require(':', "stormAbility")) break;
          if (*ak.value == "id") a.id = lv_rd_str(r, "stormAbility.id");
          else if (*ak.value == "effect") a.effect = lv_rd_str(r, "stormAbility.effect");
          else if (*ak.value == "cost") a.cost = lv_rd_u32(r, "stormAbility.cost");
          else if (*ak.value == "cooldownTicks") a.cooldown_ticks = lv_rd_u32(r, "stormAbility.cooldownTicks");
          else if (*ak.value == "activeTicks") a.active_ticks = lv_rd_u32(r, "stormAbility.activeTicks");
          else r.skip_value();
          r.record_object_member("stormAbility");
          if (!r.next_object_member("stormAbility")) break;
        }
        r.end_object("stormAbility");
        if (!a.id.empty()) s.storm_abilities.push_back(std::move(a));
        r.record_array_element("seed.stormAbilities");
        if (!r.next_array_element("seed.stormAbilities")) break;
      }
      r.end_array("seed.stormAbilities");
    }
    else if (k == "forms") {
      if (!r.begin_array("seed.forms")) break;
      while (r.has_more() && !r.has_error()) {
        if (!r.begin_object("form")) break;
        FormSeed fs;
        while (r.has_more() && !r.has_error()) {
          auto fk = r.read_string_result(); if (!fk.ok()) break;
          if (!r.require(':', "form")) break;
          if (*fk.value == "id") fs.id = lv_rd_str(r, "form.id");
          else if (*fk.value == "transformationIds") {
            if (!r.begin_array("form.transformationIds")) break;
            while (r.has_more() && !r.has_error()) {
              fs.transformation_ids.push_back(lv_rd_str(r, "form.transformationId"));
              r.record_array_element("form.transformationIds");
              if (!r.next_array_element("form.transformationIds")) break;
            }
            r.end_array("form.transformationIds");
          } else r.skip_value();
          r.record_object_member("form");
          if (!r.next_object_member("form")) break;
        }
        r.end_object("form");
        if (!fs.id.empty()) s.forms.push_back(std::move(fs));
        r.record_array_element("seed.forms");
        if (!r.next_array_element("seed.forms")) break;
      }
      r.end_array("seed.forms");
    }
    else if (k == "formAttributes") {
      if (!r.begin_object("seed.formAttributes")) break;
      while (r.has_more() && !r.has_error()) {
        auto fid = r.read_string_result(); if (!fid.ok()) break;
        if (!r.require(':', "seed.formAttributes")) break;
        if (!r.begin_object("formAttr")) break;
        FormAttributes fa;
        while (r.has_more() && !r.has_error()) {
          auto fk = r.read_string_result(); if (!fk.ok()) break;
          if (!r.require(':', "formAttr")) break;
          if (*fk.value == "maxHealth") fa.max_health = lv_rd_u32(r, "formAttr.maxHealth");
          else if (*fk.value == "resourceCapacity") fa.resource_capacity = lv_rd_u32(r, "formAttr.resourceCapacity");
          else if (*fk.value == "collisionScale") fa.collision_scale = lv_rd_dbl(r, "formAttr.collisionScale");
          else if (*fk.value == "abilityEnvelope") fa.ability_envelope = lv_rd_dbl(r, "formAttr.abilityEnvelope");
          else r.skip_value();
          r.record_object_member("formAttr");
          if (!r.next_object_member("formAttr")) break;
        }
        r.end_object("formAttr");
        s.form_attributes[std::move(*fid.value)] = fa;
        r.record_object_member("seed.formAttributes");
        if (!r.next_object_member("seed.formAttributes")) break;
      }
      r.end_object("seed.formAttributes");
    }
    else if (k == "transformations") {
      if (!r.begin_array("seed.transformations")) break;
      while (r.has_more() && !r.has_error()) {
        if (!r.begin_object("transformation")) break;
        TransformationSeed ts;
        while (r.has_more() && !r.has_error()) {
          auto tk = r.read_string_result(); if (!tk.ok()) break;
          if (!r.require(':', "transformation")) break;
          if (*tk.value == "id") ts.id = lv_rd_str(r, "transformation.id");
          else if (*tk.value == "fromForm") ts.from_form = lv_rd_str(r, "transformation.fromForm");
          else if (*tk.value == "toForm") ts.to_form = lv_rd_str(r, "transformation.toForm");
          else if (*tk.value == "triggerCondition") ts.trigger_condition = lv_rd_str(r, "transformation.triggerCondition");
          else if (*tk.value == "durationTicks") ts.duration_ticks = lv_rd_u32(r, "transformation.durationTicks");
          else if (*tk.value == "resourceCost") ts.resource_cost = lv_rd_u32(r, "transformation.resourceCost");
          else r.skip_value();
          r.record_object_member("transformation");
          if (!r.next_object_member("transformation")) break;
        }
        r.end_object("transformation");
        if (!ts.id.empty()) s.transformations.push_back(std::move(ts));
        r.record_array_element("seed.transformations");
        if (!r.next_array_element("seed.transformations")) break;
      }
      r.end_array("seed.transformations");
    }
    else if (k == "collisions") {
      if (!r.begin_object("seed.collisions")) break;
      while (r.has_more() && !r.has_error()) {
        auto ck = r.read_string_result(); if (!ck.ok()) break;
        if (!r.require(':', "seed.collisions")) break;
        if (*ck.value == "shapes") {
          if (!r.begin_array("seed.collisions.shapes")) break;
          while (r.has_more() && !r.has_error()) {
            if (!r.begin_object("collisionShape")) break;
            CollisionShape cs;
            while (r.has_more() && !r.has_error()) {
              auto sk = r.read_string_result(); if (!sk.ok()) break;
              if (!r.require(':', "collisionShape")) break;
              if (*sk.value == "id") cs.id = lv_rd_str(r, "cs.id");
              else if (*sk.value == "attachmentId") cs.bone_id = lv_rd_str(r, "cs.attachmentId");
              else if (*sk.value == "kind") { auto v = lv_rd_str(r, "cs.kind"); cs.kind = (v == "CIRCLE") ? CollisionKind::circle : CollisionKind::axis_aligned_box; }
              else if (*sk.value == "offsetX") cs.offset_x = lv_rd_dbl(r, "cs.offsetX");
              else if (*sk.value == "offsetY") cs.offset_y = lv_rd_dbl(r, "cs.offsetY");
              else if (*sk.value == "extentX") cs.extent_x = lv_rd_dbl(r, "cs.extentX");
              else if (*sk.value == "extentY") cs.extent_y = lv_rd_dbl(r, "cs.extentY");
              else r.skip_value();
              r.record_object_member("collisionShape");
              if (!r.next_object_member("collisionShape")) break;
            }
            r.end_object("collisionShape");
            if (!cs.id.empty()) s.collision_shapes.push_back(std::move(cs));
            r.record_array_element("seed.collisions.shapes");
            if (!r.next_array_element("seed.collisions.shapes")) break;
          }
          r.end_array("seed.collisions.shapes");
        } else if (*ck.value == "windows") {
          if (!r.begin_array("seed.collisions.windows")) break;
          while (r.has_more() && !r.has_error()) {
            if (!r.begin_object("collisionWindow")) break;
            CollisionWindow cw;
            while (r.has_more() && !r.has_error()) {
              auto wk = r.read_string_result(); if (!wk.ok()) break;
              if (!r.require(':', "collisionWindow")) break;
              if (*wk.value == "shapeId") cw.shape_id = lv_rd_str(r, "cw.shapeId");
              else if (*wk.value == "abilityId") cw.ability_id = lv_rd_str(r, "cw.abilityId");
              else if (*wk.value == "startTick") cw.start_tick = lv_rd_u32(r, "cw.startTick");
              else if (*wk.value == "endTick") cw.end_tick = lv_rd_u32(r, "cw.endTick");
              else if (*wk.value == "dealsDamage") cw.deals_damage = lv_rd_bool(r, "cw.dealsDamage");
              else r.skip_value();
              r.record_object_member("collisionWindow");
              if (!r.next_object_member("collisionWindow")) break;
            }
            r.end_object("collisionWindow");
            if (!cw.shape_id.empty()) s.collision_windows.push_back(std::move(cw));
            r.record_array_element("seed.collisions.windows");
            if (!r.next_array_element("seed.collisions.windows")) break;
          }
          r.end_array("seed.collisions.windows");
        } else r.skip_value();
        r.record_object_member("seed.collisions");
        if (!r.next_object_member("seed.collisions")) break;
      }
      r.end_object("seed.collisions");
    }
    else if (k == "morphology") {
      if (!r.begin_object("seed.morphology")) break;
      while (r.has_more() && !r.has_error()) {
        auto part_id = r.read_string_result(); if (!part_id.ok()) break;
        if (!r.require(':', "seed.morphology")) break;
        if (!r.begin_object("morphPart")) break;
        MorphologyPart mp;
        while (r.has_more() && !r.has_error()) {
          auto mk = r.read_string_result(); if (!mk.ok()) break;
          if (!r.require(':', "morphPart")) break;
          if (*mk.value == "boneId") mp.bone_id = lv_rd_str(r, "mp.boneId");
          else if (*mk.value == "color") mp.color = lv_rd_str(r, "mp.color");
          else if (*mk.value == "parent") mp.parent = lv_rd_str(r, "mp.parent");
          else if (*mk.value == "primitive") mp.primitive = lv_rd_str(r, "mp.primitive");
          else if (*mk.value == "semanticRole") mp.semantic_role = lv_rd_str(r, "mp.semanticRole");
          else if (*mk.value == "x") mp.x = lv_rd_dbl(r, "mp.x");
          else if (*mk.value == "y") mp.y = lv_rd_dbl(r, "mp.y");
          else if (*mk.value == "z") mp.z = lv_rd_dbl(r, "mp.z");
          else if (*mk.value == "sizeX") mp.size_x = lv_rd_dbl(r, "mp.sizeX");
          else if (*mk.value == "sizeY") mp.size_y = lv_rd_dbl(r, "mp.sizeY");
          else if (*mk.value == "sizeZ") mp.size_z = lv_rd_dbl(r, "mp.sizeZ");
          else if (*mk.value == "rotationDegrees") mp.rotation_degrees = lv_rd_dbl(r, "mp.rotationDegrees");
          else if (*mk.value == "zOrder") mp.z_order = static_cast<std::int32_t>(lv_rd_u32(r, "mp.zOrder"));
          else if (*mk.value == "emissive") mp.emissive = lv_rd_bool(r, "mp.emissive");
          else if (*mk.value == "electricalMarking") mp.electrical_marking = lv_rd_bool(r, "mp.electricalMarking");
          else r.skip_value();
          r.record_object_member("morphPart");
          if (!r.next_object_member("morphPart")) break;
        }
        r.end_object("morphPart");
        if (part_id.ok() && !part_id.value->empty()) s.morphology[std::move(*part_id.value)] = mp;
        r.record_object_member("seed.morphology");
        if (!r.next_object_member("seed.morphology")) break;
      }
      r.end_object("seed.morphology");
    }
    else if (k == "runtime") {
      if (!r.begin_object("seed.runtime")) break;
      if (!s.runtime) s.runtime = RuntimeAttributes{};  // preserve intents parsed earlier
      while (r.has_more() && !r.has_error()) {
        auto rk = r.read_string_result(); if (!rk.ok()) break;
        if (!r.require(':', "seed.runtime")) break;
        if (*rk.value == "aggression") s.runtime->aggression = lv_rd_u32(r, "rt.aggression");
        else if (*rk.value == "curiosity") s.runtime->curiosity = lv_rd_u32(r, "rt.curiosity");
        else if (*rk.value == "energy") s.runtime->energy = lv_rd_u32(r, "rt.energy");
        else if (*rk.value == "loyalty") s.runtime->loyalty = lv_rd_u32(r, "rt.loyalty");
        else r.skip_value();
        r.record_object_member("seed.runtime");
        if (!r.next_object_member("seed.runtime")) break;
      }
      r.end_object("seed.runtime");
    }
    else if (k == "animationIntents") {
      if (!r.begin_array("seed.animationIntents")) break;
      if (!s.runtime) s.runtime = RuntimeAttributes{};
      while (r.has_more() && !r.has_error()) {
        if (!r.begin_object("animIntent")) break;
        std::string behavior, clip;
        while (r.has_more() && !r.has_error()) {
          auto ak2 = r.read_string_result(); if (!ak2.ok()) break;
          if (!r.require(':', "animIntent")) break;
          if (*ak2.value == "behavior") behavior = lv_rd_str(r, "ai.behavior");
          else if (*ak2.value == "clip") clip = lv_rd_str(r, "ai.clip");
          else r.skip_value();
          r.record_object_member("animIntent");
          if (!r.next_object_member("animIntent")) break;
        }
        r.end_object("animIntent");
        if (!behavior.empty()) s.runtime->animation_intents.emplace_back(std::move(behavior), std::move(clip));
        r.record_array_element("seed.animationIntents");
        if (!r.next_array_element("seed.animationIntents")) break;
      }
      r.end_array("seed.animationIntents");
    }
    else { r.skip_value(); }
    r.record_object_member("seed");
    if (!r.next_object_member("seed")) break;
  }
  r.end_object("seed");
  return s;
}

} // anonymous namespace

/* === Public API === */

std::string encode_package_component(std::string_view id) { return encode_pc(id); }

std::string decode_package_component(std::string_view encoded) {
  // Injective hex decoder: strip "id-" prefix, decode hex pairs
  if (!encoded.starts_with("id-")) return std::string(encoded);
  encoded = encoded.substr(3);
  if (encoded.size() % 2 != 0) throw std::runtime_error("invalid hex encoding length");
  std::string out; out.reserve(encoded.size() / 2);
  for (std::size_t i = 0; i < encoded.size(); i += 2) {
    char hex[3] = { encoded[i], encoded[i+1], '\0' };
    unsigned int v = 0;
    auto [ptr,ec] = std::from_chars(hex, hex+2, v, 16);
    if (ec != std::errc{}) throw std::runtime_error("invalid hex in encoded component");
    out += static_cast<char>(v);
  }
  return out;
}

/* -- build_living_visual_package -- */
void build_living_visual_package(const LivingVisualPackageInput& input, const std::filesystem::path& output) {
  if (output.empty()) throw std::invalid_argument("empty output path");
  if (std::filesystem::exists(output)) throw std::runtime_error("output exists: "+output.string());
  lv_validate_input(input);
  auto staging = output; staging += ".staging";
  if (std::filesystem::exists(staging)) throw std::runtime_error("staging exists: "+staging.string());
  std::filesystem::create_directories(staging);
  std::filesystem::create_directories(staging/"frames");
  std::filesystem::create_directories(staging/"channels");
  std::filesystem::create_directories(staging/"sheet");

  try {
    auto seed_json = canonicalize(input.seed);
    auto seed_id = sha256(seed_json);
    lv_write(staging/"seed.json", std::string("{\"schema\":\"") + std::string(kSchemaLivingSeed) + "\",\"data\":" + seed_json + "}");
    lv_write(staging/"seed-identity.txt", seed_id);
    for (auto const& f : input.frames) {
      auto png = encode_png(f.image);
      lv_write(staging/"frames"/(encode_pc(f.id)+".png"), std::string_view(reinterpret_cast<const char*>(png.data()), png.size()));
    }
    auto atlas_png = encode_png(input.sheet.atlas.image);
    lv_write(staging/"sheet"/"atlas.png", std::string_view(reinterpret_cast<const char*>(atlas_png.data()), atlas_png.size()));
    lv_write(staging/"sheet"/"atlas.json", std::string("{\"schema\":\"") + std::string(kSchemaSpriteSheet) + "\",\"data\":" + input.sheet.metadata + "}");

    // Source skeletal animations
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaSourceSkeletalAnimations << "\",\"clips\":[";
      for (std::size_t i=0; i<input.seed.clips.size(); ++i) {
        if (i) o << ","; auto const& c = input.seed.clips[i];
        o << "{\"id\":\"" << lv_escape(c.id) << "\",\"duration_ticks\":" << c.duration_ticks << ",\"looping\":" << (c.looping?"true":"false") << ",\"tracks\":[";
        for (std::size_t j=0; j<c.tracks.size(); ++j) {
          if (j) o << ","; auto const& t = c.tracks[j];
          o << "{\"bone_id\":\"" << lv_escape(t.bone_id) << "\",\"keys\":[";
          for (std::size_t k=0; k<t.keys.size(); ++k) {
            if (k) o << ","; auto const& key = t.keys[k];
            o << "{\"tick\":" << key.tick << ",\"x\":" << key.transform.x << ",\"y\":" << key.transform.y
              << ",\"rotation_degrees\":" << key.transform.rotation_degrees << ",\"scale_x\":" << key.transform.scale_x << ",\"scale_y\":" << key.transform.scale_y << "}";
          }
          o << "]}";
        }
        o << "],\"events\":[";
        for (std::size_t j=0; j<c.events.size(); ++j) { if (j) o << ","; o << "{\"name\":\"" << lv_escape(c.events[j].first) << "\",\"tick\":" << c.events[j].second << "}"; }
        o << "]}";
      }
      o << "]}"; lv_write(staging/"source-skeletal-animations.json", o.str());
    }

    // Generated 2D animations
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaGeneratedAnimation2d << "\",\"clips\":[";
      for (std::size_t i=0; i<input.generated_clips.size(); ++i) {
        if (i) o << ","; auto const& c = input.generated_clips[i];
        o << "{\"id\":\"" << lv_escape(c.id) << "\",\"looping\":" << (c.looping?"true":"false") << ",\"frame_ids\":[";
        for (std::size_t j=0; j<c.frame_ids.size(); ++j) { if (j) o << ","; o << "\"" << lv_escape(c.frame_ids[j]) << "\""; }
        o << "],\"frame_durations\":[";
        for (std::size_t j=0; j<c.frame_durations.size(); ++j) { if (j) o << ","; o << c.frame_durations[j]; }
        o << "],\"events\":[";
        for (std::size_t j=0; j<c.events.size(); ++j) { if (j) o << ","; o << "{\"id\":\"" << lv_escape(c.events[j].id) << "\",\"tick\":" << c.events[j].tick << "}"; }
        o << "]}";
      }
      o << "]}"; lv_write(staging/"animations-2d.json", o.str());
    }

    // Frame samples
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaFrameSamples << "\",\"samples\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) {
        if (i) o << ","; auto const& s = input.samples[i];
        o << "{\"clip_id\":\"" << lv_escape(s.clip_id) << "\",\"frame_id\":\"" << lv_escape(s.frame_id)
          << "\",\"frame_index\":" << s.frame_index << ",\"source_tick\":" << s.source_tick
          << ",\"pose_hash\":\"" << lv_escape(s.pose_hash) << "\",\"frame_hash\":\"" << lv_escape(s.frame_hash) << "\"}";
      }
      o << "]}"; lv_write(staging/"frame-samples.json", o.str());
    }

    // Pose hashes
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaPoseHashes << "\",\"hashes\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) { if (i) o << ","; o << "\"" << lv_escape(input.samples[i].pose_hash) << "\""; }
      o << "]}"; lv_write(staging/"pose-hashes.json", o.str());
    }
    // Frame hashes
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaFrameHashes << "\",\"hashes\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) { if (i) o << ","; o << "\"" << lv_escape(input.samples[i].frame_hash) << "\""; }
      o << "]}"; lv_write(staging/"frame-hashes.json", o.str());
    }
    // Generated events
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaGeneratedAnimationEvents << "\",\"events\":[";
      for (std::size_t i=0; i<input.events.size(); ++i) {
        if (i) o << ","; auto const& e = input.events[i];
        o << "{\"clip_id\":\"" << lv_escape(e.clip_id) << "\",\"event_id\":\"" << lv_escape(e.event_id)
          << "\",\"authored_tick\":" << e.authored_tick << ",\"frame_index\":" << e.frame_index
          << ",\"frame_id\":\"" << lv_escape(e.frame_id) << "\"}";
      }
      o << "]}"; lv_write(staging/"animation-events.json", o.str());
    }
    // Channels
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaChannelMaps << "\",\"maps\":[";
      for (std::size_t i=0; i<input.channels.size(); ++i) {
        if (i) o << ","; auto const& ch = input.channels[i];
        o << "{\"id\":\"" << lv_escape(ch.id) << "\",\"target_frame_id\":\"" << lv_escape(ch.target_frame_id)
          << "\",\"kind\":" << static_cast<int>(ch.kind) << ",\"width\":" << ch.image.width << ",\"height\":" << ch.image.height << "}";
        auto ch_png = encode_png(ch.image);
        lv_write(staging/"channels"/(encode_pc(ch.id)+".png"), std::string_view(reinterpret_cast<const char*>(ch_png.data()), ch_png.size()));
      }
      o << "]}"; lv_write(staging/"channels.json", o.str());
    }
    // Collisions
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaCollisions2d << "\",\"shapes\":[";
      for (std::size_t i=0; i<input.collision_shapes.size(); ++i) {
        if (i) o << ","; auto const& s = input.collision_shapes[i];
        o << "{\"id\":\"" << lv_escape(s.id) << "\",\"kind\":" << static_cast<int>(s.kind)
          << ",\"bone_id\":\"" << lv_escape(s.bone_id) << "\",\"offset_x\":" << s.offset_x
          << ",\"offset_y\":" << s.offset_y << ",\"extent_x\":" << s.extent_x << ",\"extent_y\":" << s.extent_y << "}";
      }
      o << "],\"windows\":[";
      for (std::size_t i=0; i<input.collision_windows.size(); ++i) {
        if (i) o << ","; auto const& w = input.collision_windows[i];
        o << "{\"shape_id\":\"" << lv_escape(w.shape_id) << "\",\"start_tick\":" << w.start_tick
          << ",\"end_tick\":" << w.end_tick << ",\"deals_damage\":" << (w.deals_damage?"true":"false")
          << ",\"ability_id\":\"" << lv_escape(w.ability_id) << "\"}";
      }
      o << "]}"; lv_write(staging/"collisions-2d.json", o.str());
    }
    // Resolved morphologies
    lv_write(staging/"resolved-base-morphology.json", std::string("{\"schema\":\"") + std::string(kSchemaEffectiveMorphology) + "\",\"parts\":" + lv_morph_json(input.base_morphology) + "}");
    lv_write(staging/"resolved-storm-morphology.json", std::string("{\"schema\":\"") + std::string(kSchemaEffectiveMorphology) + "\",\"parts\":" + lv_morph_json(input.storm_morphology) + "}");
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaTransformationMorphologies << "\",\"morphologies\":[";
      for (std::size_t i=0; i<input.transformation_morphologies.size(); ++i) { if (i) o << ","; o << lv_morph_json(input.transformation_morphologies[i]); }
      o << "]}"; lv_write(staging/"transformation-morphologies.json", o.str());
    }

    // Build manifest
    std::vector<std::string> paths, hashes;
    auto add_artifact = [&](std::string path) { auto bytes = lv_read(staging/path, 512ULL*1024*1024); paths.push_back(std::move(path)); hashes.push_back(sha256(bytes)); };
    add_artifact("seed.json"); add_artifact("seed-identity.txt");
    for (auto const& f : input.frames) add_artifact("frames/"+encode_pc(f.id)+".png");
    add_artifact("sheet/atlas.png"); add_artifact("sheet/atlas.json");
    add_artifact("source-skeletal-animations.json"); add_artifact("animations-2d.json");
    add_artifact("frame-samples.json"); add_artifact("pose-hashes.json"); add_artifact("frame-hashes.json");
    add_artifact("animation-events.json");
    for (auto const& ch : input.channels) add_artifact("channels/"+encode_pc(ch.id)+".png");
    add_artifact("channels.json"); add_artifact("collisions-2d.json");
    add_artifact("resolved-base-morphology.json"); add_artifact("resolved-storm-morphology.json");
    add_artifact("transformation-morphologies.json");

    std::vector<std::size_t> order(paths.size());
    for (std::size_t i=0; i<order.size(); ++i) order[i]=i;
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) { return paths[a] < paths[b]; });

    // Build manifest preimage (without packageIdentity)
    std::ostringstream m; m << "{\"format\":\"" << kSchemaLivingVisualPackage << "\""
      << ",\"identityVersion\":\"" << kIdentityPreimageVersion << "\""
      << ",\"entityId\":\"" << lv_escape(input.seed.stable_id) << "\""
      << ",\"seedIdentity\":\"" << seed_id << "\""
      << ",\"frameCount\":" << input.frames.size()
      << ",\"clipCount\":" << input.generated_clips.size()
      << ",\"sampleCount\":" << input.samples.size()
      << ",\"eventCount\":" << input.events.size()
      << ",\"channelCount\":" << input.channels.size()
      << ",\"collisionShapeCount\":" << input.collision_shapes.size()
      << ",\"collisionWindowCount\":" << input.collision_windows.size()
      << ",\"artifacts\":[";
    for (std::size_t i=0; i<order.size(); ++i) {
      if (i) m << ",";
      m << "{\"path\":\"" << lv_escape(paths[order[i]]) << "\",\"sha256\":\"" << hashes[order[i]] << "\"}";
    }
    m << "]}";
    auto manifest_preimage = m.str();

    // Package identity = SHA-256(kIdentityPreimageVersion + "\n" + manifest preimage)
    auto pkg_id = sha256(std::string(kIdentityPreimageVersion) + "\n" + manifest_preimage);
    auto full_manifest = std::string("{\"packageIdentity\":\"") + pkg_id + "\"," + manifest_preimage.substr(1);
    lv_write(staging/"manifest.json", full_manifest);
    std::filesystem::rename(staging, output);
  } catch (...) { std::filesystem::remove_all(staging); throw; }
}

/* -- read_living_visual_package -- */
LivingVisualPackageReadResult read_living_visual_package(const std::filesystem::path& package_path, const PackageReadLimits& limits) {
  LivingVisualPackageReadResult result;
  auto add = [&](std::string code, std::string msg) { result.diagnostics.diagnostics.push_back({std::move(code), std::move(msg)}); };
  try {
    if (!std::filesystem::exists(package_path) || !std::filesystem::is_directory(package_path))
      { add("LV_READ_NO_DIR", "package path not a directory"); return result; }
    auto manifest_bytes = lv_read(package_path/"manifest.json", limits.max_manifest_bytes);
    auto entity_id = lv_manifest_str(manifest_bytes, "entityId");
    auto stored_pkg_id = lv_manifest_str(manifest_bytes, "packageIdentity");
    if (entity_id.empty()) { add("LV_READ_NO_ENTITY", "manifest missing entityId"); return result; }

    // Load and parse seed using proper JSON parsing of the wrapper
    auto seed_wrapper = lv_read(package_path/"seed.json", limits.max_artifact_bytes);
    std::string seed_json;
    {
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader wr(seed_wrapper, cfg);
      if (wr.begin_object("seed-wrapper")) {
        while (wr.has_more() && !wr.has_error()) {
          auto wk = wr.read_string_result(); if (!wk.ok()) break;
          if (!wr.require(':', "seed-wrapper")) break;
          if (*wk.value == "data") {
            seed_json = wr.read_typed_value();
          } else if (*wk.value == "schema") { wr.skip_value(); }
          else { wr.skip_value(); }
          wr.record_object_member("seed-wrapper");
          if (!wr.next_object_member("seed-wrapper")) break;
        }
        wr.end_object("seed-wrapper");
      }
    }
    if (seed_json.empty()) { add("LV_READ_NO_DATA", "seed wrapper missing data field"); return result; }
    auto seed = lv_parse_seed_json(seed_json);
    if (seed.stable_id.empty()) seed.stable_id = entity_id;
    auto computed_seed_id = sha256(seed_json);
    auto manifest_seed_id = lv_manifest_str(manifest_bytes, "seedIdentity");

    LoadedLivingVisualPackage pkg;
    pkg.schema = std::string(kSchemaLivingVisualPackage);
    pkg.entity_id = entity_id;
    pkg.seed_identity = computed_seed_id;
    pkg.package_identity = stored_pkg_id;
    pkg.seed = std::move(seed);

    // Verify seed identity matches
    if (!manifest_seed_id.empty() && manifest_seed_id != computed_seed_id)
      add("LV_READ_SEED_MISMATCH", "seed identity mismatch: manifest vs computed");

    result.value = std::move(pkg);
  } catch (std::exception const& e) { add("LV_READ_ERROR", e.what()); }
  return result;
}

/* -- verify_living_visual_package -- */
LivingVisualPackageVerificationResult verify_living_visual_package(const std::filesystem::path& package_path, const PackageVerificationOptions& options) {
  LivingVisualPackageVerificationResult result;
  auto add = [&](std::string code, std::string msg) { result.validation.diagnostics.push_back({std::move(code), std::move(msg)}); };
  try {
    if (!std::filesystem::exists(package_path) || !std::filesystem::is_directory(package_path))
      { add("LV_VERIFY_NO_DIR", "package path not found"); return result; }
    if (options.require_no_symlinks && std::filesystem::is_symlink(std::filesystem::symlink_status(package_path)))
      { add("LV_VERIFY_SYMLINK_ROOT", "package root is a symlink"); return result; }

    auto manifest_bytes = lv_read(package_path/"manifest.json", 4ULL*1024*1024);

    // Verify identity version
    auto ident_ver = lv_manifest_str(manifest_bytes, "identityVersion");
    if (ident_ver.empty()) add("LV_VERIFY_NO_IDENT_VER", "manifest missing identityVersion");
    else if (ident_ver != kIdentityPreimageVersion) add("LV_VERIFY_BAD_IDENT_VER", "unsupported identityVersion: "+ident_ver);

    // Verify package identity: recompute from preimage
    auto pkg_id_start = manifest_bytes.find("\"packageIdentity\"");
    if (pkg_id_start == std::string_view::npos) {
      add("LV_VERIFY_NO_PKG_ID", "manifest missing packageIdentity");
    } else {
      auto stored_pkg_id = lv_manifest_str(manifest_bytes, "packageIdentity");
      if (stored_pkg_id.size() != 64) add("LV_VERIFY_BAD_PKG_ID_LEN", "packageIdentity not 64 hex chars");
      else if (!lowercase_sha256(stored_pkg_id)) add("LV_VERIFY_BAD_PKG_ID_FMT", "packageIdentity not lowercase hex");
      auto colon_pos = manifest_bytes.find(':', pkg_id_start);
      auto val_end = manifest_bytes.find('"', manifest_bytes.find('"', colon_pos + 1) + 1);
      auto comma_pos = manifest_bytes.find(',', val_end + 1);
      std::string manifest_without_pkg_id = "{";
      if (comma_pos != std::string_view::npos) manifest_without_pkg_id += std::string(manifest_bytes.substr(comma_pos + 1));
      std::string preimage = std::string(kIdentityPreimageVersion) + "\n" + manifest_without_pkg_id;
      auto computed_pkg_id = sha256(preimage);
      result.package_identity = computed_pkg_id;
      if (computed_pkg_id != stored_pkg_id)
        add("LV_VERIFY_PKG_ID", "package identity mismatch");
    }

    // Verify per-artifact file existence and SHA-256 from manifest
    result.frame_count = lv_manifest_int(manifest_bytes, "frameCount");
    result.clip_count = lv_manifest_int(manifest_bytes, "clipCount");
    result.sample_count = lv_manifest_int(manifest_bytes, "sampleCount");
    result.event_count = lv_manifest_int(manifest_bytes, "eventCount");

    if (result.frame_count != 48) add("LV_VERIFY_FRAME_COUNT", "expected 48 frames");
    if (result.clip_count != 9) add("LV_VERIFY_CLIP_COUNT", "expected 9 clips");
    if (result.sample_count != 48) add("LV_VERIFY_SAMPLE_COUNT", "expected 48 samples");

    // Verify manifest artifacts: parse artifacts array, check each file exists and SHA-256 matches
    {
      auto art_start = manifest_bytes.find("\"artifacts\"");
      if (art_start != std::string_view::npos) {
        gspl::BoundedJsonConfig cfg{};
        gspl::BoundedJsonReader mr(manifest_bytes, cfg);
        if (mr.begin_object("manifest")) {
          while (mr.has_more() && !mr.has_error()) {
            auto mk = mr.read_string_result(); if (!mk.ok()) break;
            if (!mr.require(':', "manifest")) break;
            if (*mk.value == "artifacts") {
              if (!mr.begin_array("manifest.artifacts")) break;
              std::set<std::string> declared;
              while (mr.has_more() && !mr.has_error()) {
                if (!mr.begin_object("artifact")) break;
                std::string path, hash;
                while (mr.has_more() && !mr.has_error()) {
                  auto ak = mr.read_string_result(); if (!ak.ok()) break;
                  if (!mr.require(':', "artifact")) break;
                  if (*ak.value == "path") path = lv_rd_str(mr, "artifact.path");
                  else if (*ak.value == "sha256") hash = lv_rd_str(mr, "artifact.sha256");
                  else mr.skip_value();
                  mr.record_object_member("artifact");
                  if (!mr.next_object_member("artifact")) break;
                }
                mr.end_object("artifact");
                if (!path.empty()) {
                  if (!declared.insert(path).second) add("LV_VERIFY_DUP_PATH", "duplicate artifact path: "+path);
                  else {
                    auto artifact_path = package_path/path;
                    if (!std::filesystem::is_regular_file(artifact_path))
                      add("LV_VERIFY_MISSING_FILE", "declared artifact missing: "+path);
                    else if (!hash.empty()) {
                      try {
                        auto file_bytes = lv_read(artifact_path, 512ULL*1024*1024);
                        auto file_hash = sha256(file_bytes);
                        if (file_hash != hash)
                          add("LV_VERIFY_HASH_MISMATCH", "artifact hash mismatch: "+path);
                      } catch (...) { add("LV_VERIFY_READ_ERR", "cannot read artifact: "+path); }
                    }
                  }
                }
                mr.record_array_element("manifest.artifacts");
                if (!mr.next_array_element("manifest.artifacts")) break;
              }
              mr.end_array("manifest.artifacts");

              // Check for undeclared files
              if (options.require_no_undeclared_files && std::filesystem::exists(package_path/"frames")) {
                for (auto const& e : std::filesystem::recursive_directory_iterator(package_path)) {
                  if (e.is_regular_file() && !e.is_symlink()) {
                    auto rel = e.path().lexically_relative(package_path).generic_string();
                    if (rel != "manifest.json" && !declared.contains(rel))
                      add("LV_VERIFY_UNDECLARED", "undeclared file: "+rel);
                  }
                }
              }
            } else mr.skip_value();
            mr.record_object_member("manifest");
            if (!mr.next_object_member("manifest")) break;
          }
          mr.end_object("manifest");
        }
      }
    }

    // Required artifact presence check
    static constexpr std::array required = {
      "seed.json", "seed-identity.txt",
      "source-skeletal-animations.json", "animations-2d.json",
      "frame-samples.json", "animation-events.json",
      "pose-hashes.json", "frame-hashes.json",
      "channels.json", "collisions-2d.json",
      "resolved-base-morphology.json", "resolved-storm-morphology.json",
      "transformation-morphologies.json",
      "sheet/atlas.png", "sheet/atlas.json"
    };
    for (auto r : required)
      if (!std::filesystem::exists(package_path/r)) add("LV_VERIFY_MISSING", "missing: "+std::string(r));

    // Seed identity verification
    if (std::filesystem::exists(package_path/"seed-identity.txt")) {
      auto seed_id_bytes = lv_read(package_path/"seed-identity.txt", 256);
      seed_id_bytes = seed_id_bytes.substr(0, seed_id_bytes.find_last_not_of(" \t\n\r")+1);
      auto manifest_seed_id = lv_manifest_str(manifest_bytes, "seedIdentity");
      if (!manifest_seed_id.empty() && manifest_seed_id != seed_id_bytes)
        add("LV_VERIFY_SEED_ID", "seed identity mismatch: manifest vs seed-identity.txt");
      result.seed_identity = std::string(seed_id_bytes);
    }

    // Frame file count check
    if (std::filesystem::exists(package_path/"frames")) {
      std::uint32_t actual_frames = 0;
      for (auto const& e : std::filesystem::directory_iterator(package_path/"frames"))
        if (e.is_regular_file() && e.path().extension() == ".png") ++actual_frames;
      if (actual_frames != result.frame_count)
        add("LV_VERIFY_FRAME_FILES", "frame file count mismatch: expected "+std::to_string(result.frame_count)+" got "+std::to_string(actual_frames));
    }
  } catch (std::exception const& e) { add("LV_VERIFY_ERROR", e.what()); }
  return result;
}

} // namespace gspl::sprites
