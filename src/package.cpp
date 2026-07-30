#include "gspl_sprites/package.hpp"

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

/* -- Safe path component encoding -- */
std::string encode_pc(std::string_view id) {
  std::string out; out.reserve(id.size());
  for (unsigned char c : id) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.') {
      out += static_cast<char>(c);
    } else {
      out += '_';
      char hex[3]{}; auto [ptr,ec] = std::to_chars(hex, hex+2, c, 16);
      if (ec != std::errc{}) throw std::runtime_error("path encoding failed");
      if (hex[1] == '\0') { out += '0'; out += hex[0]; }
      else { out += hex[0]; out += hex[1]; }
    }
  }
  if (out.empty() || out.size() > 200) throw std::runtime_error("invalid encoded component length");
  static constexpr std::array reserved{"CON","PRN","AUX","NUL","COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9","LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"};
  for (auto r : reserved) if (out == r) { out += "_"; break; }
  return out;
}

/* -- Validate path safety -- */
bool lv_safe_path(std::string_view path) {
  if (path.empty() || path.front() == '/' || path.back() == '/') return false;
  if (path.find("..") != std::string_view::npos) return false;
  if (path.find('\\') != std::string_view::npos) return false;
  if (path.find(':') != std::string_view::npos) return false;
  if (path.size() > 1024) return false;
  for (unsigned char c : path) if (c < 0x20 || c >= 0x7f) return false;
  std::size_t s = 0;
  while (s < path.size()) {
    auto e = path.find('/', s);
    auto part = path.substr(s, e == std::string_view::npos ? path.size() - s : e - s);
    if (part.empty() || part == "." || part == "..") return false;
    if (e == std::string_view::npos) break;
    s = e + 1;
  }
  return true;
}

/* -- Canonical morphology JSON -- */
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

/* -- Pre-validate build input -- */
void lv_validate_input(const LivingVisualPackageInput& in) {
  auto add = [](auto& diags, std::string code, std::string msg) { diags.push_back({std::move(code), std::move(msg)}); };
  std::vector<Diagnostic> diags;
  if (in.frames.size() != 48) add(diags, "LV_INPUT_FRAMES", "expected 48 frames");
  if (in.generated_clips.size() != 9) add(diags, "LV_INPUT_CLIPS", "expected 9 clips");
  if (in.samples.size() != 48) add(diags, "LV_INPUT_SAMPLES", "expected 48 samples");
  if (in.transformation_morphologies.size() != 10) add(diags, "LV_INPUT_TRANS_MORPHS", "expected 10 transformation morphologies");
  if (in.base_morphology.empty()) add(diags, "LV_INPUT_BASE_MORPH", "base morphology empty");
  if (in.storm_morphology.empty()) add(diags, "LV_INPUT_STORM_MORPH", "storm morphology empty");
  std::set<std::string> frame_ids;
  for (auto const& f : in.frames) {
    if (f.id.empty()) { add(diags, "LV_INPUT_FRAME_ID", "empty frame id"); continue; }
    if (!frame_ids.insert(f.id).second) add(diags, "LV_INPUT_DUP_FRAME", "duplicate frame id: "+f.id);
  }
  std::set<std::string> clip_ids;
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
    if (s.frame_hash.size() != 64 || s.pose_hash.size() != 64)
      add(diags, "LV_INPUT_SAMPLE_HASH", "invalid hash length in sample for frame: "+s.frame_id);
  }
  for (auto const& e : in.events) {
    if (!clip_ids.contains(e.clip_id)) add(diags, "LV_INPUT_EVENT_CLIP", "event references unknown clip: "+e.clip_id);
    if (!frame_ids.contains(e.frame_id)) add(diags, "LV_INPUT_EVENT_FRAME", "event references unknown frame: "+e.frame_id);
  }
  std::set<std::string> ch_ids;
  for (auto const& ch : in.channels) {
    if (!ch_ids.insert(ch.id).second) add(diags, "LV_INPUT_DUP_CH", "duplicate channel id: "+ch.id);
    if (!frame_ids.contains(ch.target_frame_id)) add(diags, "LV_INPUT_CH_TARGET", "channel targets unknown frame: "+ch.target_frame_id);
  }
  if (!diags.empty()) {
    std::ostringstream msgs;
    for (auto const& d : diags) msgs << d.code << ": " << d.message << "; ";
    throw std::invalid_argument(msgs.str());
  }
}

/* -- Extract JSON values (for reader/verifier) -- */
std::string lv_json_str(std::string_view data, std::string_view key) {
  auto kp = data.find(std::string("\"") + std::string(key) + "\"");
  if (kp == std::string_view::npos) return {};
  auto start = data.find('"', kp + key.size() + 2);
  if (start == std::string_view::npos) return {};
  auto end = data.find('"', start+1);
  if (end == std::string_view::npos) return {};
  return std::string(data.substr(start+1, end-start-1));
}

std::uint32_t lv_json_int(std::string_view data, std::string_view key) {
  auto kp = data.find(std::string("\"") + std::string(key) + "\"");
  if (kp == std::string_view::npos) return 0;
  auto start = kp + key.size() + 2;
  while (start < data.size() && (data[start] == ':' || data[start] == ' ' || data[start] == '\t')) ++start;
  std::uint32_t v=0;
  while (start < data.size() && data[start] >= '0' && data[start] <= '9') { v = v*10 + (data[start]-'0'); ++start; }
  return v;
}

/* -- Parse canonical seed JSON back to minimal SpriteSeed -- */
SpriteSeed lv_parse_seed_json(std::string_view json) {
  SpriteSeed s;
  s.stable_id = lv_json_str(json, "id");
  s.name = lv_json_str(json, "name");
  s.schema = lv_json_str(json, "schema");
  s.classification = lv_json_str(json, "classification");
  auto colors_pos = json.find("\"colors\"");
  if (colors_pos != std::string_view::npos) {
    s.primary_color = lv_json_str(json.substr(colors_pos), "primary");
    s.accent_color = lv_json_str(json.substr(colors_pos), "accent");
  }
  return s;
}

} // anonymous namespace

/* === Public API === */

std::string encode_package_component(std::string_view id) { return encode_pc(id); }

std::string decode_package_component(std::string_view encoded) {
  std::string out; out.reserve(encoded.size());
  for (std::size_t i = 0; i < encoded.size(); ++i) {
    if (encoded[i] == '_' && i + 2 < encoded.size()) {
      char hex[3] = { encoded[i+1], encoded[i+2], '\0' };
      unsigned int v = 0;
      auto [ptr,ec] = std::from_chars(hex, hex+2, v, 16);
      if (ec == std::errc{}) { out += static_cast<char>(v); i += 2; continue; }
    }
    out += encoded[i];
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
    // -- Seed (canonical JSON wrapped with schema) --
    auto seed_json = canonicalize(input.seed);
    auto seed_id = sha256(seed_json);
    lv_write(staging/"seed.json", std::string("{\"schema\":\"") + std::string(kSchemaLivingSeed) + "\",\"data\":" + seed_json + "}");
    lv_write(staging/"seed-identity.txt", seed_id);

    // -- Frames as PNG --
    for (auto const& f : input.frames) {
      auto png = encode_png(f.image);
      auto safe_name = encode_pc(f.id) + ".png";
      lv_write(staging/"frames"/safe_name, std::string_view(reinterpret_cast<const char*>(png.data()), png.size()));
    }

    // -- Sprite sheet --
    auto atlas_png = encode_png(input.sheet.atlas.image);
    lv_write(staging/"sheet"/"atlas.png", std::string_view(reinterpret_cast<const char*>(atlas_png.data()), atlas_png.size()));
    lv_write(staging/"sheet"/"atlas.json", std::string("{\"schema\":\"") + std::string(kSchemaSpriteSheet) + "\",\"data\":" + input.sheet.metadata + "}");

    // -- Source skeletal animations --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaSourceSkeletalAnimations << "\",\"clips\":[";
      for (std::size_t i=0; i<input.seed.clips.size(); ++i) {
        if (i) o << ","; auto const& c = input.seed.clips[i];
        o << "{\"id\":\"" << lv_escape(c.id) << "\",\"duration_ticks\":" << c.duration_ticks
          << ",\"looping\":" << (c.looping?"true":"false") << ",\"tracks\":[";
        for (std::size_t j=0; j<c.tracks.size(); ++j) {
          if (j) o << ","; auto const& t = c.tracks[j];
          o << "{\"bone_id\":\"" << lv_escape(t.bone_id) << "\",\"keys\":[";
          for (std::size_t k=0; k<t.keys.size(); ++k) {
            if (k) o << ","; auto const& key = t.keys[k];
            o << "{\"tick\":" << key.tick << ",\"x\":" << key.transform.x << ",\"y\":" << key.transform.y
              << ",\"rotation_degrees\":" << key.transform.rotation_degrees
              << ",\"scale_x\":" << key.transform.scale_x << ",\"scale_y\":" << key.transform.scale_y << "}";
          }
          o << "]}";
        }
        o << "],\"events\":[";
        for (std::size_t j=0; j<c.events.size(); ++j) {
          if (j) o << ","; o << "{\"name\":\"" << lv_escape(c.events[j].first) << "\",\"tick\":" << c.events[j].second << "}";
        }
        o << "]}";
      }
      o << "]}";
      lv_write(staging/"source-skeletal-animations.json", o.str());
    }

    // -- Generated 2D animations --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaGeneratedAnimation2d << "\",\"clips\":[";
      for (std::size_t i=0; i<input.generated_clips.size(); ++i) {
        if (i) o << ","; auto const& c = input.generated_clips[i];
        o << "{\"id\":\"" << lv_escape(c.id) << "\",\"looping\":" << (c.looping?"true":"false")
          << ",\"frame_ids\":[";
        for (std::size_t j=0; j<c.frame_ids.size(); ++j) {
          if (j) o << ","; o << "\"" << lv_escape(c.frame_ids[j]) << "\"";
        }
        o << "],\"frame_durations\":[";
        for (std::size_t j=0; j<c.frame_durations.size(); ++j) {
          if (j) o << ","; o << c.frame_durations[j];
        }
        o << "],\"events\":[";
        for (std::size_t j=0; j<c.events.size(); ++j) {
          if (j) o << ","; o << "{\"id\":\"" << lv_escape(c.events[j].id) << "\",\"tick\":" << c.events[j].tick << "}";
        }
        o << "]}";
      }
      o << "]}";
      lv_write(staging/"animations-2d.json", o.str());
    }

    // -- Frame samples --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaFrameSamples << "\",\"samples\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) {
        if (i) o << ","; auto const& s = input.samples[i];
        o << "{\"clip_id\":\"" << lv_escape(s.clip_id) << "\",\"frame_id\":\"" << lv_escape(s.frame_id)
          << "\",\"frame_index\":" << s.frame_index << ",\"source_tick\":" << s.source_tick
          << ",\"pose_hash\":\"" << lv_escape(s.pose_hash) << "\",\"frame_hash\":\"" << lv_escape(s.frame_hash) << "\"}";
      }
      o << "]}";
      lv_write(staging/"frame-samples.json", o.str());
    }

    // -- Pose hashes --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaPoseHashes << "\",\"hashes\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) {
        if (i) o << ","; o << "\"" << lv_escape(input.samples[i].pose_hash) << "\"";
      }
      o << "]}";
      lv_write(staging/"pose-hashes.json", o.str());
    }

    // -- Frame hashes --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaFrameHashes << "\",\"hashes\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) {
        if (i) o << ","; o << "\"" << lv_escape(input.samples[i].frame_hash) << "\"";
      }
      o << "]}";
      lv_write(staging/"frame-hashes.json", o.str());
    }

    // -- Generated events --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaGeneratedAnimationEvents << "\",\"events\":[";
      for (std::size_t i=0; i<input.events.size(); ++i) {
        if (i) o << ","; auto const& e = input.events[i];
        o << "{\"clip_id\":\"" << lv_escape(e.clip_id) << "\",\"event_id\":\"" << lv_escape(e.event_id)
          << "\",\"authored_tick\":" << e.authored_tick << ",\"frame_index\":" << e.frame_index
          << ",\"frame_id\":\"" << lv_escape(e.frame_id) << "\"}";
      }
      o << "]}";
      lv_write(staging/"animation-events.json", o.str());
    }

    // -- Channels --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaChannelMaps << "\",\"maps\":[";
      for (std::size_t i=0; i<input.channels.size(); ++i) {
        if (i) o << ","; auto const& ch = input.channels[i];
        o << "{\"id\":\"" << lv_escape(ch.id) << "\",\"target_frame_id\":\"" << lv_escape(ch.target_frame_id)
          << "\",\"kind\":" << static_cast<int>(ch.kind) << ",\"width\":" << ch.image.width
          << ",\"height\":" << ch.image.height << "}";
        auto ch_png = encode_png(ch.image);
        lv_write(staging/"channels"/(encode_pc(ch.id)+".png"), std::string_view(reinterpret_cast<const char*>(ch_png.data()), ch_png.size()));
      }
      o << "]}";
      lv_write(staging/"channels.json", o.str());
    }

    // -- Collisions --
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaCollisions2d << "\",\"shapes\":[";
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
      o << "]}";
      lv_write(staging/"collisions-2d.json", o.str());
    }

    // -- Resolved morphologies --
    lv_write(staging/"resolved-base-morphology.json", std::string("{\"schema\":\"") + std::string(kSchemaEffectiveMorphology) + "\",\"parts\":" + lv_morph_json(input.base_morphology) + "}");
    lv_write(staging/"resolved-storm-morphology.json", std::string("{\"schema\":\"") + std::string(kSchemaEffectiveMorphology) + "\",\"parts\":" + lv_morph_json(input.storm_morphology) + "}");
    {
      std::ostringstream o; o << "{\"schema\":\"" << kSchemaTransformationMorphologies << "\",\"morphologies\":[";
      for (std::size_t i=0; i<input.transformation_morphologies.size(); ++i) {
        if (i) o << ","; o << lv_morph_json(input.transformation_morphologies[i]);
      }
      o << "]}";
      lv_write(staging/"transformation-morphologies.json", o.str());
    }

    // -- Build manifest (without packageIdentity first, for preimage hashing) --
    std::vector<std::string> paths;
    std::vector<std::string> hashes;
    auto add_artifact = [&](std::string path) {
      auto bytes = lv_read(staging/path, 512ULL*1024*1024);
      paths.push_back(std::move(path));
      hashes.push_back(sha256(bytes));
    };
    add_artifact("seed.json");
    add_artifact("seed-identity.txt");
    for (auto const& f : input.frames) add_artifact("frames/"+encode_pc(f.id)+".png");
    add_artifact("sheet/atlas.png");
    add_artifact("sheet/atlas.json");
    add_artifact("source-skeletal-animations.json");
    add_artifact("animations-2d.json");
    add_artifact("frame-samples.json");
    add_artifact("pose-hashes.json");
    add_artifact("frame-hashes.json");
    add_artifact("animation-events.json");
    for (auto const& ch : input.channels) add_artifact("channels/"+encode_pc(ch.id)+".png");
    add_artifact("channels.json");
    add_artifact("collisions-2d.json");
    add_artifact("resolved-base-morphology.json");
    add_artifact("resolved-storm-morphology.json");
    add_artifact("transformation-morphologies.json");

    // Sort by path
    std::vector<std::size_t> order(paths.size());
    for (std::size_t i=0; i<order.size(); ++i) order[i]=i;
    std::ranges::sort(order, [&](std::size_t a, std::size_t b) { return paths[a] < paths[b]; });

    // Build manifest preimage (without packageIdentity)
    std::ostringstream m; m << "{\"format\":\"" << kSchemaLivingVisualPackage << "\""
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

    // Package identity = SHA-256(schema + "\n" + manifest preimage)
    auto pkg_id = sha256(std::string(kSchemaLivingVisualPackage) + "\n" + manifest_preimage);

    // Insert packageIdentity at the start of the full manifest
    auto full_manifest = std::string("{\"packageIdentity\":\"") + pkg_id + "\","
                       + manifest_preimage.substr(1);
    lv_write(staging/"manifest.json", full_manifest);

    std::filesystem::rename(staging, output);
  } catch (...) {
    std::filesystem::remove_all(staging);
    throw;
  }
}

/* -- read_living_visual_package -- */
LivingVisualPackageReadResult read_living_visual_package(const std::filesystem::path& package_path,
                                                           const PackageReadLimits& limits) {
  LivingVisualPackageReadResult result;
  auto add = [&](std::string code, std::string msg) { result.diagnostics.diagnostics.push_back({std::move(code), std::move(msg)}); };
  try {
    if (!std::filesystem::exists(package_path) || !std::filesystem::is_directory(package_path))
      { add("LV_READ_NO_DIR", "package path not a directory"); return result; }

    auto manifest_bytes = lv_read(package_path/"manifest.json", limits.max_manifest_bytes);
    auto entity_id = lv_json_str(manifest_bytes, "entityId");
    auto stored_pkg_id = lv_json_str(manifest_bytes, "packageIdentity");
    if (entity_id.empty()) { add("LV_READ_NO_ENTITY", "manifest missing entityId"); return result; }

    // Read seed
    auto seed_wrapper = lv_read(package_path/"seed.json", limits.max_artifact_bytes);
    auto seed_data_pos = seed_wrapper.find("\"data\":");
    std::string_view seed_json = seed_wrapper;
    if (seed_data_pos != std::string_view::npos) {
      seed_json = seed_wrapper.substr(seed_data_pos + 7);
      if (!seed_json.empty() && seed_json.back() == '}') seed_json.remove_suffix(1);
    }
    auto seed = lv_parse_seed_json(seed_json);
    if (seed.stable_id.empty()) seed.stable_id = entity_id;

    // Build LoadedLivingVisualPackage
    LoadedLivingVisualPackage pkg;
    pkg.schema = std::string(kSchemaLivingVisualPackage);
    pkg.entity_id = entity_id;
    pkg.seed_identity = sha256(std::string(seed_json));
    pkg.package_identity = stored_pkg_id;
    pkg.seed = std::move(seed);

    result.value = std::move(pkg);
  } catch (std::exception const& e) {
    add("LV_READ_ERROR", e.what());
  }
  return result;
}

/* -- verify_living_visual_package -- */
LivingVisualPackageVerificationResult verify_living_visual_package(const std::filesystem::path& package_path,
                                                                     const PackageVerificationOptions& options) {
  LivingVisualPackageVerificationResult result;
  auto add = [&](std::string code, std::string msg) { result.validation.diagnostics.push_back({std::move(code), std::move(msg)}); };
  try {
    if (!std::filesystem::exists(package_path) || !std::filesystem::is_directory(package_path))
      { add("LV_VERIFY_NO_DIR", "package path not found"); return result; }
    if (options.require_no_symlinks && std::filesystem::is_symlink(std::filesystem::symlink_status(package_path)))
      { add("LV_VERIFY_SYMLINK_ROOT", "package root is a symlink"); return result; }

    auto manifest_bytes = lv_read(package_path/"manifest.json", 4ULL*1024*1024);

    // Verify package identity: recompute from preimage
    // Manifest = {"packageIdentity":"...","format":...}
    // Preimage = "gspl.living-visual-package/0.1\n" + {"format":...}
    auto pkg_id_start = manifest_bytes.find("\"packageIdentity\"");
    if (pkg_id_start != std::string_view::npos) {
      auto stored_pkg_id = lv_json_str(manifest_bytes, "packageIdentity");
      auto colon_pos = manifest_bytes.find(':', pkg_id_start);
      auto val_end = manifest_bytes.find('"', manifest_bytes.find('"', colon_pos + 1) + 1);
      auto comma_pos = manifest_bytes.find(',', val_end + 1);
      std::string manifest_without_pkg_id = "{";
      if (comma_pos != std::string_view::npos)
        manifest_without_pkg_id += std::string(manifest_bytes.substr(comma_pos + 1));
      std::string preimage = std::string(kSchemaLivingVisualPackage) + "\n" + manifest_without_pkg_id;
      auto computed_pkg_id = sha256(preimage);
      result.package_identity = computed_pkg_id;
      if (computed_pkg_id != stored_pkg_id)
        add("LV_VERIFY_PKG_ID", "package identity mismatch: stored="+stored_pkg_id.substr(0,16)+" computed="+computed_pkg_id.substr(0,16));
    }

    // Check required files
    static constexpr std::array required = {
      "seed.json", "seed-identity.txt", "manifest.json",
      "source-skeletal-animations.json", "animations-2d.json",
      "frame-samples.json", "animation-events.json",
      "pose-hashes.json", "frame-hashes.json",
      "channels.json", "collisions-2d.json",
      "resolved-base-morphology.json", "resolved-storm-morphology.json",
      "transformation-morphologies.json",
      "sheet/atlas.png", "sheet/atlas.json"
    };
    for (auto r : required) {
      if (!std::filesystem::exists(package_path/r))
        add("LV_VERIFY_MISSING", "missing: "+std::string(r));
    }

    // Read manifest counters
    result.frame_count = lv_json_int(manifest_bytes, "frameCount");
    result.clip_count = lv_json_int(manifest_bytes, "clipCount");
    result.sample_count = lv_json_int(manifest_bytes, "sampleCount");
    result.event_count = lv_json_int(manifest_bytes, "eventCount");

    if (result.frame_count != 48) add("LV_VERIFY_FRAME_COUNT", "expected 48 frames, got "+std::to_string(result.frame_count));
    if (result.clip_count != 9) add("LV_VERIFY_CLIP_COUNT", "expected 9 clips, got "+std::to_string(result.clip_count));
    if (result.sample_count != 48) add("LV_VERIFY_SAMPLE_COUNT", "expected 48 samples, got "+std::to_string(result.sample_count));

    // Verify seed identity
    auto seed_id_bytes = lv_read(package_path/"seed-identity.txt", 256);
    seed_id_bytes = seed_id_bytes.substr(0, seed_id_bytes.find_last_not_of(" \t\n\r")+1);
    auto manifest_seed_id = lv_json_str(manifest_bytes, "seedIdentity");
    if (manifest_seed_id != seed_id_bytes)
      add("LV_VERIFY_SEED_ID", "seed identity mismatch in manifest");

    // Verify frame count from filesystem
    if (std::filesystem::exists(package_path/"frames")) {
      std::uint32_t actual_frames = 0;
      for (auto const& e : std::filesystem::directory_iterator(package_path/"frames"))
        if (e.is_regular_file() && e.path().extension() == ".png") ++actual_frames;
      if (actual_frames != result.frame_count)
        add("LV_VERIFY_FRAME_FILES", "frame file count mismatch: manifest="+std::to_string(result.frame_count)+" disk="+std::to_string(actual_frames));
    }

  } catch (std::exception const& e) {
    add("LV_VERIFY_ERROR", e.what());
  }
  return result;
}

} // namespace gspl::sprites
