#include "gspl_sprites/package.hpp"
#include "gspl_sprites/animation_sampling.hpp"

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
#include <span>
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
} // anonymous namespace

/* ── Artifact kind ↔ string helpers ── */
std::string_view artifact_kind_string(LivingArtifactKind kind) noexcept {
  switch (kind) {
  case LivingArtifactKind::living_seed: return "living-seed";
  case LivingArtifactKind::seed_identity: return "seed-identity";
  case LivingArtifactKind::source_skeletal_animations: return "source-skeletal-animations";
  case LivingArtifactKind::frame_metadata: return "frame-metadata";
  case LivingArtifactKind::frame_image: return "frame-image";
  case LivingArtifactKind::generated_animation: return "generated-animation";
  case LivingArtifactKind::frame_samples: return "frame-samples";
  case LivingArtifactKind::generated_events: return "generated-events";
  case LivingArtifactKind::pose_hashes: return "pose-hashes";
  case LivingArtifactKind::frame_hashes: return "frame-hashes";
  case LivingArtifactKind::channel_metadata: return "channel-metadata";
  case LivingArtifactKind::channel_image: return "channel-image";
  case LivingArtifactKind::collision_metadata: return "collision-metadata";
  case LivingArtifactKind::effective_morphology: return "effective-morphology";
  case LivingArtifactKind::transformation_morphologies: return "transformation-morphologies";
  case LivingArtifactKind::sprite_atlas: return "sprite-atlas";
  case LivingArtifactKind::sprite_atlas_metadata: return "sprite-atlas-metadata";
  }
  return "";  // unreachable — validated model ensures valid enum before serialization
}

std::string_view artifact_schema_for_kind(LivingArtifactKind kind) noexcept {
  switch (kind) {
  case LivingArtifactKind::living_seed: return kSchemaLivingSeed;
  case LivingArtifactKind::source_skeletal_animations: return kSchemaSourceSkeletalAnimations;
  case LivingArtifactKind::frame_metadata: return kSchemaFrames2d;
  case LivingArtifactKind::generated_animation: return kSchemaGeneratedAnimation2d;
  case LivingArtifactKind::frame_samples: return kSchemaFrameSamples;
  case LivingArtifactKind::generated_events: return kSchemaGeneratedAnimationEvents;
  case LivingArtifactKind::pose_hashes: return kSchemaPoseHashes;
  case LivingArtifactKind::frame_hashes: return kSchemaFrameHashes;
  case LivingArtifactKind::channel_metadata: return kSchemaChannelMaps;
  case LivingArtifactKind::collision_metadata: return kSchemaCollisions2d;
  case LivingArtifactKind::effective_morphology: return kSchemaEffectiveMorphology;
  case LivingArtifactKind::transformation_morphologies: return kSchemaTransformationMorphologies;
  case LivingArtifactKind::sprite_atlas_metadata: return kSchemaSpriteSheet;
  case LivingArtifactKind::sprite_atlas: return "";
  case LivingArtifactKind::channel_image: return "";
  case LivingArtifactKind::seed_identity: return "";
  case LivingArtifactKind::frame_image: return "";
  }
  return "";
}

std::optional<LivingArtifactKind> artifact_kind_from_string(std::string_view s) noexcept {
  if (s == "living-seed") return LivingArtifactKind::living_seed;
  if (s == "seed-identity") return LivingArtifactKind::seed_identity;
  if (s == "source-skeletal-animations") return LivingArtifactKind::source_skeletal_animations;
  if (s == "frame-metadata") return LivingArtifactKind::frame_metadata;
  if (s == "frame-image") return LivingArtifactKind::frame_image;
  if (s == "generated-animation") return LivingArtifactKind::generated_animation;
  if (s == "frame-samples") return LivingArtifactKind::frame_samples;
  if (s == "generated-events") return LivingArtifactKind::generated_events;
  if (s == "pose-hashes") return LivingArtifactKind::pose_hashes;
  if (s == "frame-hashes") return LivingArtifactKind::frame_hashes;
  if (s == "channel-metadata") return LivingArtifactKind::channel_metadata;
  if (s == "channel-image") return LivingArtifactKind::channel_image;
  if (s == "collision-metadata") return LivingArtifactKind::collision_metadata;
  if (s == "effective-morphology") return LivingArtifactKind::effective_morphology;
  if (s == "transformation-morphologies") return LivingArtifactKind::transformation_morphologies;
  if (s == "sprite-atlas") return LivingArtifactKind::sprite_atlas;
  if (s == "sprite-atlas-metadata") return LivingArtifactKind::sprite_atlas_metadata;
  return std::nullopt;
}

/* ── Manifest model validation ── */
ValidationResult validate_manifest_model(const LivingPackageManifest& m, const PackageReadLimits& limits) {
  ValidationResult r;
  auto add = [&](std::string code, std::string msg) { r.diagnostics.push_back({std::move(code), std::move(msg)}); };

  if (m.format != kSchemaLivingVisualPackage)
    add("LV_MANIFEST_FORMAT", "unsupported manifest format");
  if (m.identity_version != kIdentityPreimageVersion)
    add("LV_MANIFEST_IDV", "unsupported identity version");
  if (m.entity_id.empty())
    add("LV_MANIFEST_NO_ENTITY", "missing entity ID");
  if (m.canonical_entity_identity.empty() || !lowercase_sha256(m.canonical_entity_identity))
    add("LV_MANIFEST_NO_CANON", "missing or malformed canonical entity identity");
  if (!lowercase_sha256(m.seed_identity))
    add("LV_MANIFEST_SEEDID", "malformed seed identity");
  if (!m.package_identity.empty() && !lowercase_sha256(m.package_identity))
    add("LV_MANIFEST_PKGID", "malformed package identity");

  if (m.artifact_count != static_cast<std::uint32_t>(m.artifacts.size()))
    add("LV_MANIFEST_ACOUNT", "artifact_count != artifacts.size()");
  if (m.artifacts.empty())
    add("LV_MANIFEST_NO_ARTIFACTS", "empty artifacts array");

  if (m.artifacts.size() > limits.max_artifacts)
    add("LV_MANIFEST_ALIMIT", "artifact count exceeds limit");

  std::set<std::string> paths;
  std::set<std::string> paths_lower;
  // First pass: collect all paths & check uniqueness/sorting
  {
    std::string prev_path;
    for (auto const& a : m.artifacts) {
      if (a.path.empty()) {
        add("LV_MANIFEST_EMPTY_PATH", "artifact has empty path");
        continue;
      }
      if (a.path.size() > limits.max_path_bytes)
        add("LV_MANIFEST_PATH_LEN", "artifact path exceeds limit: " + a.path);
      if (!paths.insert(a.path).second)
        add("LV_MANIFEST_DUP_PATH", "duplicate artifact path: " + a.path);
      std::string lower = a.path;
      for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      if (!paths_lower.insert(lower).second)
        add("LV_MANIFEST_CASE_COLLISION", "case-fold path collision: " + a.path);
      if (!prev_path.empty() && a.path <= prev_path)
        add("LV_MANIFEST_UNSORTED", "artifact paths not strictly sorted: " + a.path + " <= " + prev_path);
      prev_path = a.path;
    }
  }
  // Second pass: validate schemas, hashes, dependencies, provenance
  for (auto const& a : m.artifacts) {
    if (a.path.empty()) continue;

    // Reject invalid artifact kind (out-of-range enum)
    {
      std::string_view ks = artifact_kind_string(a.kind);
      if (ks.empty()) {
        add("LV_MANIFEST_INVALID_KIND", "invalid/out-of-range artifact kind for: " + a.path);
        continue;
      }
      // Check that kind string round-trips correctly
      auto back = artifact_kind_from_string(ks);
      if (!back || *back != a.kind)
        add("LV_MANIFEST_INVALID_KIND", "artifact kind does not round-trip for: " + a.path);
    }

    auto expected_schema = artifact_schema_for_kind(a.kind);
    if (a.schema != expected_schema)
      add("LV_MANIFEST_SCHEMA", "schema mismatch for " + a.path + ": expected '" + std::string(expected_schema) + "' got '" + a.schema + "'");

    if (!lowercase_sha256(a.sha256))
      add("LV_MANIFEST_HASH", "malformed SHA-256 for: " + a.path);

    if (a.provenance_identity.empty() || !lowercase_sha256(a.provenance_identity))
      add("LV_MANIFEST_NO_PROV", "missing or malformed provenance identity for: " + a.path);

    // Validate dependencies
    std::set<std::string> dep_set;
    std::string prev_dep;
    for (auto const& d : a.dependencies) {
      if (d == a.path)
        add("LV_MANIFEST_SELF_DEP", "self-dependency in: " + a.path);
      if (!dep_set.insert(d).second)
        add("LV_MANIFEST_DUP_DEP", "duplicate dependency in " + a.path + ": " + d);
      if (!prev_dep.empty() && d <= prev_dep)
        add("LV_MANIFEST_UNSORTED_DEP", "unsorted dependency in " + a.path + ": " + d + " <= " + prev_dep);
      prev_dep = d;
      if (!paths.contains(d))
        add("LV_MANIFEST_UNKNOWN_DEP", "unknown dependency in " + a.path + ": " + d);
    }
  }

  return r;
}

/* ── Embedded schema extraction from JSON artifact ── */
std::string extract_embedded_schema(std::string_view json_bytes, const PackageReadLimits& limits) {
  gspl::BoundedJsonConfig cfg{};
  cfg.max_object_members = 64;
  cfg.max_input_bytes = json_bytes.size() + 1;
  cfg.max_nesting_depth = limits.max_json_nesting;
  gspl::BoundedJsonReader r(json_bytes, cfg);
  if (!r.begin_object("schema-extract")) return "";
  std::string schema;
  while (r.has_more() && !r.has_error()) {
    auto key = r.read_string_result();
    if (!key.ok()) break;
    if (!r.require(':', "schema-extract")) break;
    if (*key.value == "schema") {
      auto schema_result = r.read_string_result();
      if (schema_result.ok()) schema = std::move(*schema_result.value);
      else r.skip_value();
    } else r.skip_value();
    r.record_object_member("schema-extract");
    if (!r.next_object_member("schema-extract")) break;
  }
  r.end_object("schema-extract");
  return schema;
}

/* ── Path-confinement lexical validation (no filesystem access) ── */
namespace {
bool lexically_safe_package_path(std::string_view path, std::uint32_t max_bytes) {
  if (path.empty() || path.size() > max_bytes) return false;
  if (path.front() == '/' || path.back() == '/') return false;
  if (path.find(':') != std::string_view::npos) return false;
  if (path.find('\\') != std::string_view::npos) return false;
  if (path.find("//") != std::string_view::npos) return false;
  for (unsigned char c : path) if (c < 0x20 || c > 0x7e) return false;
  std::size_t start = 0;
  while (start < path.size()) {
    auto end = path.find('/', start);
    auto part = path.substr(start, end == std::string_view::npos ? path.size() - start : end - start);
    if (part.empty() || part == "." || part == "..") return false;
    if (part.back() == ' ' || part.back() == '.') return false;
    if (end == std::string_view::npos) break;
    start = end + 1;
  }
  return true;
}
} // anonymous namespace

/* ── Validated artifact inventory (single authority for manifest path → OS path) ── */
LivingPackageInventoryResult validate_living_package_inventory(
    const std::filesystem::path& package_root,
    const LivingPackageManifest& manifest,
    const PackageReadLimits& limits,
    const PackageVerificationOptions& options) {
  LivingPackageInventoryResult result;
  auto add = [&](std::string code, std::string msg) { result.diagnostics.diagnostics.push_back({std::move(code), std::move(msg)}); };

  try {
    auto canonical_root = std::filesystem::weakly_canonical(package_root);
    ValidatedLivingPackageInventory inv;
    inv.canonical_root = canonical_root;

    // Phase 1: Lexical validation + inventory construction
    for (auto const& art : manifest.artifacts) {
      // Lexical safety
      if (!lexically_safe_package_path(art.path, limits.max_path_bytes)) {
        add("LV_INV_PATH_UNSAFE", "lexically unsafe artifact path: " + art.path);
        continue;
      }

      // Build candidate absolute path
      auto candidate = canonical_root / art.path;

      // Walk every existing component for symlinks
      bool symlink_found = false;
      {
        auto cur = canonical_root;
        std::size_t start = 0;
        while (start < art.path.size()) {
          auto end = art.path.find('/', start);
          auto part = art.path.substr(start, end == std::string_view::npos ? art.path.size() - start : end - start);
          cur /= part;
          if (std::filesystem::exists(cur)) {
            auto st = std::filesystem::symlink_status(cur);
            if (std::filesystem::is_symlink(st)) { symlink_found = true; break; }
          }
          if (end == std::string_view::npos) break;
          start = end + 1;
        }
      }
      if (symlink_found && options.require_no_symlinks) {
        add("LV_INV_SYMLINK", "symlink in artifact path: " + art.path);
        continue;
      }

      // Final path confinement: canonicalize and prove stays under root
      auto canon = std::filesystem::weakly_canonical(candidate);
      auto cs = canon.string();
      auto rs = canonical_root.string();
      if (cs.size() < rs.size() || cs.compare(0, rs.size(), rs) != 0) {
        add("LV_INV_PATH_ESCAPE", "path escapes package root: " + art.path);
        continue;
      }
      if (cs.size() > rs.size() && cs[rs.size()] != '/' && cs[rs.size()] != '\\') {
        add("LV_INV_PATH_ESCAPE", "path escapes package root: " + art.path);
        continue;
      }

      // Must exist and be regular file
      if (!std::filesystem::is_regular_file(canon)) {
        add("LV_INV_MISSING", "declared artifact missing: " + art.path);
        continue;
      }

      // Verify byte size
      auto fsz = std::filesystem::file_size(canon);
      if (fsz != art.byte_size) {
        add("LV_INV_SIZE", "byte size mismatch: " + art.path);
        continue;
      }

      // Enforce byte limits
      if (fsz > limits.max_artifact_bytes) {
        add("LV_INV_ARTIFACT_LIMIT", "artifact exceeds byte limit: " + art.path);
        continue;
      }
      if (inv.total_artifact_bytes > limits.max_total_bytes - fsz) {
        add("LV_INV_TOTAL_LIMIT", "total package byte limit exceeded");
        continue;
      }
      inv.total_artifact_bytes += fsz;

      // Enforce artifact count
      if (inv.by_path.size() >= limits.max_artifacts) {
        add("LV_INV_COUNT_LIMIT", "artifact count exceeds limit");
        break;
      }

      // Read and verify SHA-256
      auto file_bytes = read_bounded(canon, limits.max_artifact_bytes);
      auto computed_hash = sha256(file_bytes);
      if (computed_hash != art.sha256) {
        add("LV_INV_HASH", "artifact hash mismatch: " + art.path);
        continue;
      }

      // Register
      ValidatedLivingArtifact va;
      va.record = &art;
      va.absolute_path = canon;
      va.verified_bytes = std::make_shared<const std::string>(std::move(file_bytes));
      va.verified_byte_size = fsz;
      va.verified_sha256 = computed_hash;
      inv.by_path[art.path] = std::move(va);
      inv.paths_by_kind.insert({art.kind, art.path});
    }

    // Phase 2: Directory enumeration (undeclared files, entry limits, symlinks)
    std::set<std::string> actual_files;
    try {
      for (auto const& entry : std::filesystem::recursive_directory_iterator(canonical_root)) {
        if (++inv.directory_entry_count > limits.max_directory_entries) {
          add("LV_INV_ENTRY_LIMIT", "directory entry count exceeds limit");
          break;
        }

        auto st = entry.symlink_status();
        if (std::filesystem::is_symlink(st)) {
          if (options.require_no_symlinks) {
            auto rel = entry.path().lexically_relative(canonical_root).generic_string();
            add("LV_INV_SYMLINK_ENTRY", "symlink in package tree: " + rel);
          }
          continue;
        }

        if (entry.is_regular_file()) {
          auto rel = entry.path().lexically_relative(canonical_root).generic_string();
          if (rel != "manifest.json")
            actual_files.insert(rel);
        } else if (entry.is_directory()) {
          // allow directories
        } else {
          auto rel = entry.path().lexically_relative(canonical_root).generic_string();
          add("LV_INV_SPECIAL", "special/non-regular file in package: " + rel);
        }
      }
    } catch (...) {
      add("LV_INV_ENUM_ERR", "directory enumeration failed");
    }

    // Phase 3: File-set equality
    std::set<std::string> declared_paths;
    for (auto const& art : manifest.artifacts) declared_paths.insert(art.path);

    // Check for missing declared files (those not in inventory)
    for (auto const& dp : declared_paths) {
      if (!inv.by_path.contains(dp))
        add("LV_INV_DECLARED_MISSING", "declared artifact not found on disk: " + dp);
    }

    // Check for undeclared files
    if (options.require_no_undeclared_files) {
      for (auto const& af : actual_files) {
        if (!declared_paths.contains(af))
          add("LV_INV_UNDECLARED", "undeclared file: " + af);
      }
    }

    // Check for declared but missing files from actual set
    // (actual_files - declared_paths handled by LV_INV_UNDECLARED above;
    //  declared_paths - actual_files handled by LV_INV_DECLARED_MISSING above;
    //  inventory vs. actual mismatch is an internal invariant; we trust the inventory)

    if (!result.diagnostics.ok()) return result;
    result.value = std::move(inv);
  } catch (std::exception const& e) { add("LV_INV_ERROR", e.what()); }
  return result;
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

// ── Fail-closed BoundedJsonReader helpers ──
// Every helper throws on parse failure or wrong type.
// PackageDiagnostic carries stable code + message for the caller.
struct PackageDiagnostic { std::string code; std::string message; };
[[noreturn]] static void lv_fail(std::string code, std::string msg) {
  throw std::runtime_error(code + ": " + msg);
}
static std::string lv_rd_str(gspl::BoundedJsonReader& r, std::string_view path) {
  auto res = r.read_string_result();
  if (!res.ok()) lv_fail("LV_TYPE_STR", std::string(path) + ": expected string");
  return std::move(*res.value);
}
static std::uint32_t lv_rd_u32(gspl::BoundedJsonReader& r, std::string_view path) {
  auto res = r.read_uint32_result();
  if (!res.ok()) lv_fail("LV_TYPE_U32", std::string(path) + ": expected uint32");
  return *res.value;
}
static std::int32_t lv_rd_i32(gspl::BoundedJsonReader& r, std::string_view path) {
  auto res = r.read_int32_result();
  if (!res.ok()) lv_fail("LV_TYPE_I32", std::string(path) + ": expected int32");
  return *res.value;
}
static std::uint64_t lv_rd_u64(gspl::BoundedJsonReader& r, std::string_view path) {
  auto res = r.read_uint64_result();
  if (!res.ok()) lv_fail("LV_TYPE_U64", std::string(path) + ": expected uint64");
  return *res.value;
}
static double lv_rd_dbl(gspl::BoundedJsonReader& r, std::string_view path) {
  auto res = r.read_double_result();
  if (!res.ok()) lv_fail("LV_TYPE_F64", std::string(path) + ": expected finite double");
  double v = *res.value;
  if (!std::isfinite(v)) lv_fail("LV_NONFINITE", std::string(path) + ": non-finite double");
  return v;
}
static bool lv_rd_bool(gspl::BoundedJsonReader& r, std::string_view path) {
  auto res = r.read_bool_result();
  if (!res.ok()) lv_fail("LV_TYPE_BOOL", std::string(path) + ": expected bool");
  return *res.value;
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
          else if (*mk.value == "zOrder") mp.z_order = lv_rd_i32(r, "mp.zOrder");
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
    // ── Previously skipped canonical fields ──
    else if (k == "animationClips") {
      if (!r.begin_object("seed.animationClips")) break;
      while (r.has_more() && !r.has_error()) {
        auto ck = r.read_string_result(); if (!ck.ok()) break;
        if (!r.require(':', "seed.animationClips")) break;
        if (*ck.value == "clips" && !r.has_error()) {
          if (!r.begin_array("seed.animationClips.clips")) break;
          while (r.has_more() && !r.has_error()) {
            if (!r.begin_object("skeletalClip")) break;
            SkeletalClip clip;
            while (r.has_more() && !r.has_error()) {
              auto sk = r.read_string_result(); if (!sk.ok()) break;
              if (!r.require(':', "skeletalClip")) break;
              if (*sk.value == "id") clip.id = lv_rd_str(r, "clip.id");
              else if (*sk.value == "durationTicks") clip.duration_ticks = lv_rd_u32(r, "clip.durationTicks");
              else if (*sk.value == "looping") clip.looping = lv_rd_bool(r, "clip.looping");
              else if (*sk.value == "tracks") {
                if (!r.begin_array("clip.tracks")) break;
                while (r.has_more() && !r.has_error()) {
                  if (!r.begin_object("boneTrack")) break;
                  BoneTrack track;
                  while (r.has_more() && !r.has_error()) {
                    auto tk = r.read_string_result(); if (!tk.ok()) break;
                    if (!r.require(':', "boneTrack")) break;
                    if (*tk.value == "boneId") track.bone_id = lv_rd_str(r, "track.boneId");
                    else if (*tk.value == "keys") {
                      if (!r.begin_array("track.keys")) break;
                      while (r.has_more() && !r.has_error()) {
                        if (!r.begin_object("boneKey")) break;
                        BoneKeyframe bkf;
                        while (r.has_more() && !r.has_error()) {
                          auto kk = r.read_string_result(); if (!kk.ok()) break;
                          if (!r.require(':', "boneKey")) break;
                          if (*kk.value == "tick") bkf.tick = lv_rd_u32(r, "key.tick");
                          else if (*kk.value == "transform") {
                            if (!r.begin_object("key.transform")) break;
                            while (r.has_more() && !r.has_error()) {
                              auto xk = r.read_string_result(); if (!xk.ok()) break;
                              if (!r.require(':', "key.transform")) break;
                              if (*xk.value == "x") bkf.transform.x = lv_rd_dbl(r, "trans.x");
                              else if (*xk.value == "y") bkf.transform.y = lv_rd_dbl(r, "trans.y");
                              else if (*xk.value == "rotationDegrees") bkf.transform.rotation_degrees = lv_rd_dbl(r, "trans.rot");
                              else if (*xk.value == "scaleX") bkf.transform.scale_x = lv_rd_dbl(r, "trans.sx");
                              else if (*xk.value == "scaleY") bkf.transform.scale_y = lv_rd_dbl(r, "trans.sy");
                              else r.skip_value();
                              r.record_object_member("key.transform");
                              if (!r.next_object_member("key.transform")) break;
                            }
                            r.end_object("key.transform");
                          } else r.skip_value();
                          r.record_object_member("boneKey");
                          if (!r.next_object_member("boneKey")) break;
                        }
                        r.end_object("boneKey");
                        track.keys.push_back(std::move(bkf));
                        r.record_array_element("track.keys");
                        if (!r.next_array_element("track.keys")) break;
                      }
                      r.end_array("track.keys");
                    } else r.skip_value();
                    r.record_object_member("boneTrack");
                    if (!r.next_object_member("boneTrack")) break;
                  }
                  r.end_object("boneTrack");
                  if (!track.bone_id.empty()) clip.tracks.push_back(std::move(track));
                  r.record_array_element("clip.tracks");
                  if (!r.next_array_element("clip.tracks")) break;
                }
                r.end_array("clip.tracks");
              } else if (*sk.value == "events") {
                if (!r.begin_array("clip.events")) break;
                while (r.has_more() && !r.has_error()) {
                  if (!r.begin_object("clipEvent")) break;
                  std::string ev_id; std::uint32_t ev_tick = 0;
                  while (r.has_more() && !r.has_error()) {
                    auto ek = r.read_string_result(); if (!ek.ok()) break;
                    if (!r.require(':', "clipEvent")) break;
                    if (*ek.value == "id") ev_id = lv_rd_str(r, "ev.id");
                    else if (*ek.value == "tick") ev_tick = lv_rd_u32(r, "ev.tick");
                    else r.skip_value();
                    r.record_object_member("clipEvent");
                    if (!r.next_object_member("clipEvent")) break;
                  }
                  r.end_object("clipEvent");
                  if (!ev_id.empty()) clip.events.emplace_back(std::move(ev_id), ev_tick);
                  r.record_array_element("clip.events");
                  if (!r.next_array_element("clip.events")) break;
                }
                r.end_array("clip.events");
              } else r.skip_value();
              r.record_object_member("skeletalClip");
              if (!r.next_object_member("skeletalClip")) break;
            }
            r.end_object("skeletalClip");
            if (!clip.id.empty()) s.clips.push_back(std::move(clip));
            r.record_array_element("seed.animationClips.clips");
            if (!r.next_array_element("seed.animationClips.clips")) break;
          }
          r.end_array("seed.animationClips.clips");
        } else r.skip_value();
        r.record_object_member("seed.animationClips");
        if (!r.next_object_member("seed.animationClips")) break;
      }
      r.end_object("seed.animationClips");
    }
    else if (k == "rig") {
      if (r.peek() == 'n') { r.skip_value(); } // null
      else if (!r.begin_object("seed.rig")) break;
      else {
        RigDefinition rig;
        while (r.has_more() && !r.has_error()) {
          auto rk = r.read_string_result(); if (!rk.ok()) break;
          if (!r.require(':', "seed.rig")) break;
          if (*rk.value == "id") rig.id = lv_rd_str(r, "rig.id");
          else if (*rk.value == "bones") {
            if (!r.begin_array("rig.bones")) break;
            while (r.has_more() && !r.has_error()) {
              if (!r.begin_object("bone")) break;
              BoneDefinition bone;
              while (r.has_more() && !r.has_error()) {
                auto bk = r.read_string_result(); if (!bk.ok()) break;
                if (!r.require(':', "bone")) break;
                if (*bk.value == "id") bone.id = lv_rd_str(r, "bone.id");
                else if (*bk.value == "length") bone.length = lv_rd_dbl(r, "bone.length");
                else if (*bk.value == "limit") {
                  if (!r.begin_object("bone.limit")) break;
                  while (r.has_more() && !r.has_error()) {
                    auto lk = r.read_string_result(); if (!lk.ok()) break;
                    if (!r.require(':', "bone.limit")) break;
                    if (*lk.value == "maximumDegrees") bone.limit.maximum_degrees = lv_rd_dbl(r, "limit.max");
                    else if (*lk.value == "minimumDegrees") bone.limit.minimum_degrees = lv_rd_dbl(r, "limit.min");
                    else r.skip_value();
                    r.record_object_member("bone.limit");
                    if (!r.next_object_member("bone.limit")) break;
                  }
                  r.end_object("bone.limit");
                } else if (*bk.value == "parentId") {
                  if (r.peek() == 'n') r.skip_value();
                  else bone.parent_id = lv_rd_str(r, "bone.parentId");
                } else if (*bk.value == "rest") {
                  if (!r.begin_object("bone.rest")) break;
                  while (r.has_more() && !r.has_error()) {
                    auto rk2 = r.read_string_result(); if (!rk2.ok()) break;
                    if (!r.require(':', "bone.rest")) break;
                    if (*rk2.value == "x") bone.rest.x = lv_rd_dbl(r, "rest.x");
                    else if (*rk2.value == "y") bone.rest.y = lv_rd_dbl(r, "rest.y");
                    else if (*rk2.value == "rotationDegrees") bone.rest.rotation_degrees = lv_rd_dbl(r, "rest.rot");
                    else if (*rk2.value == "scaleX") bone.rest.scale_x = lv_rd_dbl(r, "rest.sx");
                    else if (*rk2.value == "scaleY") bone.rest.scale_y = lv_rd_dbl(r, "rest.sy");
                    else r.skip_value();
                    r.record_object_member("bone.rest");
                    if (!r.next_object_member("bone.rest")) break;
                  }
                  r.end_object("bone.rest");
                } else r.skip_value();
                r.record_object_member("bone");
                if (!r.next_object_member("bone")) break;
              }
              r.end_object("bone");
              if (!bone.id.empty()) rig.bones.push_back(std::move(bone));
              r.record_array_element("rig.bones");
              if (!r.next_array_element("rig.bones")) break;
            }
            r.end_array("rig.bones");
          } else if (*rk.value == "sockets") {
            if (!r.begin_array("rig.sockets")) break;
            while (r.has_more() && !r.has_error()) {
              if (!r.begin_object("socket")) break;
              SocketDefinition sock;
              while (r.has_more() && !r.has_error()) {
                auto sk = r.read_string_result(); if (!sk.ok()) break;
                if (!r.require(':', "socket")) break;
                if (*sk.value == "id") sock.id = lv_rd_str(r, "sock.id");
                else if (*sk.value == "boneId") sock.bone_id = lv_rd_str(r, "sock.boneId");
                else if (*sk.value == "local") {
                  if (!r.begin_object("sock.local")) break;
                  while (r.has_more() && !r.has_error()) {
                    auto slk = r.read_string_result(); if (!slk.ok()) break;
                    if (!r.require(':', "sock.local")) break;
                    if (*slk.value == "x") sock.local.x = lv_rd_dbl(r, "local.x");
                    else if (*slk.value == "y") sock.local.y = lv_rd_dbl(r, "local.y");
                    else if (*slk.value == "rotationDegrees") sock.local.rotation_degrees = lv_rd_dbl(r, "local.rot");
                    else if (*slk.value == "scaleX") sock.local.scale_x = lv_rd_dbl(r, "local.sx");
                    else if (*slk.value == "scaleY") sock.local.scale_y = lv_rd_dbl(r, "local.sy");
                    else r.skip_value();
                    r.record_object_member("sock.local");
                    if (!r.next_object_member("sock.local")) break;
                  }
                  r.end_object("sock.local");
                } else r.skip_value();
                r.record_object_member("socket");
                if (!r.next_object_member("socket")) break;
              }
              r.end_object("socket");
              if (!sock.id.empty()) rig.sockets.push_back(std::move(sock));
              r.record_array_element("rig.sockets");
              if (!r.next_array_element("rig.sockets")) break;
            }
            r.end_array("rig.sockets");
          } else r.skip_value();
          r.record_object_member("seed.rig");
          if (!r.next_object_member("seed.rig")) break;
        }
        r.end_object("seed.rig");
        s.rig = std::move(rig);
      }
    }
    else if (k == "animationStateGraph") {
      if (r.peek() == 'n') { r.skip_value(); } // null
      else if (!r.begin_object("seed.animationStateGraph")) break;
      else {
        AnimationStateGraph graph;
        while (r.has_more() && !r.has_error()) {
          auto gk = r.read_string_result(); if (!gk.ok()) break;
          if (!r.require(':', "seed.animationStateGraph")) break;
          if (*gk.value == "initialState") graph.initial_state = lv_rd_str(r, "graph.initialState");
          else if (*gk.value == "states") {
            if (!r.begin_array("graph.states")) break;
            while (r.has_more() && !r.has_error()) {
              if (!r.begin_object("animState")) break;
              AnimationState state;
              while (r.has_more() && !r.has_error()) {
                auto ask = r.read_string_result(); if (!ask.ok()) break;
                if (!r.require(':', "animState")) break;
                if (*ask.value == "id") state.id = lv_rd_str(r, "state.id");
                else if (*ask.value == "clipId") state.clip_id = lv_rd_str(r, "state.clipId");
                else if (*ask.value == "transitions") {
                  if (!r.begin_array("state.transitions")) break;
                  while (r.has_more() && !r.has_error()) {
                    if (!r.begin_object("transition")) break;
                    AnimationTransition trans;
                    while (r.has_more() && !r.has_error()) {
                      auto tk = r.read_string_result(); if (!tk.ok()) break;
                      if (!r.require(':', "transition")) break;
                      if (*tk.value == "targetState") trans.target_state = lv_rd_str(r, "trans.targetState");
                      else if (*tk.value == "parameter") trans.parameter = lv_rd_str(r, "trans.parameter");
                      else if (*tk.value == "comparison") {
                        auto cv = lv_rd_str(r, "trans.comparison");
                        if (cv == "equal") trans.comparison = Comparison::equal;
                        else if (cv == "not_equal") trans.comparison = Comparison::not_equal;
                        else if (cv == "less") trans.comparison = Comparison::less;
                        else if (cv == "greater") trans.comparison = Comparison::greater;
                        else if (cv == "less_equal") trans.comparison = Comparison::less_equal;
                        else if (cv == "greater_equal") trans.comparison = Comparison::greater_equal;
                      }
                      else if (*tk.value == "threshold") trans.threshold = lv_rd_dbl(r, "trans.threshold");
                      else if (*tk.value == "minimumStateTicks") trans.minimum_state_ticks = lv_rd_u32(r, "trans.minTicks");
                      else if (*tk.value == "blendTicks") trans.blend_ticks = lv_rd_u32(r, "trans.blendTicks");
                      else if (*tk.value == "priority") trans.priority = lv_rd_u32(r, "trans.priority");
                      else r.skip_value();
                      r.record_object_member("transition");
                      if (!r.next_object_member("transition")) break;
                    }
                    r.end_object("transition");
                    state.transitions.push_back(std::move(trans));
                    r.record_array_element("state.transitions");
                    if (!r.next_array_element("state.transitions")) break;
                  }
                  r.end_array("state.transitions");
                } else r.skip_value();
                r.record_object_member("animState");
                if (!r.next_object_member("animState")) break;
              }
              r.end_object("animState");
              if (!state.id.empty()) graph.states.push_back(std::move(state));
              r.record_array_element("graph.states");
              if (!r.next_array_element("graph.states")) break;
            }
            r.end_array("graph.states");
          } else r.skip_value();
          r.record_object_member("seed.animationStateGraph");
          if (!r.next_object_member("seed.animationStateGraph")) break;
        }
        r.end_object("seed.animationStateGraph");
        if (!graph.initial_state.empty()) s.animation_graph = std::move(graph);
      }
    }
    else if (k == "morphologyOverrides") {
      if (!r.begin_object("seed.morphologyOverrides")) break;
      while (r.has_more() && !r.has_error()) {
        auto form_id = r.read_string_result(); if (!form_id.ok()) break;
        if (!r.require(':', "seed.morphologyOverrides")) break;
        if (!r.begin_object("formOverride")) break;
        std::map<std::string, MorphologyPart, std::less<>> parts;
        while (r.has_more() && !r.has_error()) {
          auto part_id = r.read_string_result(); if (!part_id.ok()) break;
          if (!r.require(':', "formOverride")) break;
          if (!r.begin_object("morphPartO")) break;
          MorphologyPart mp;
          while (r.has_more() && !r.has_error()) {
            auto mk = r.read_string_result(); if (!mk.ok()) break;
            if (!r.require(':', "morphPartO")) break;
            if (*mk.value == "boneId") mp.bone_id = lv_rd_str(r, "mp2.boneId");
            else if (*mk.value == "color") mp.color = lv_rd_str(r, "mp2.color");
            else if (*mk.value == "parent") mp.parent = lv_rd_str(r, "mp2.parent");
            else if (*mk.value == "primitive") mp.primitive = lv_rd_str(r, "mp2.primitive");
            else if (*mk.value == "semanticRole") mp.semantic_role = lv_rd_str(r, "mp2.semanticRole");
            else if (*mk.value == "x") mp.x = lv_rd_dbl(r, "mp2.x");
            else if (*mk.value == "y") mp.y = lv_rd_dbl(r, "mp2.y");
            else if (*mk.value == "z") mp.z = lv_rd_dbl(r, "mp2.z");
            else if (*mk.value == "sizeX") mp.size_x = lv_rd_dbl(r, "mp2.sizeX");
            else if (*mk.value == "sizeY") mp.size_y = lv_rd_dbl(r, "mp2.sizeY");
            else if (*mk.value == "sizeZ") mp.size_z = lv_rd_dbl(r, "mp2.sizeZ");
            else if (*mk.value == "rotationDegrees") mp.rotation_degrees = lv_rd_dbl(r, "mp2.rotationDegrees");
            else if (*mk.value == "zOrder") mp.z_order = lv_rd_i32(r, "mp2.zOrder");
            else if (*mk.value == "emissive") mp.emissive = lv_rd_bool(r, "mp2.emissive");
            else if (*mk.value == "electricalMarking") mp.electrical_marking = lv_rd_bool(r, "mp2.electricalMarking");
            else r.skip_value();
            r.record_object_member("morphPartO");
            if (!r.next_object_member("morphPartO")) break;
          }
          r.end_object("morphPartO");
          if (part_id.ok() && !part_id.value->empty()) parts[std::move(*part_id.value)] = mp;
          r.record_object_member("formOverride");
          if (!r.next_object_member("formOverride")) break;
        }
        r.end_object("formOverride");
        if (form_id.ok() && !form_id.value->empty()) s.form_morphology_overrides[std::move(*form_id.value)] = std::move(parts);
        r.record_object_member("seed.morphologyOverrides");
        if (!r.next_object_member("seed.morphologyOverrides")) break;
      }
      r.end_object("seed.morphologyOverrides");
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

/* ── Shared canonical semantic identity preimage functions ── */
/* Builder and verifier both call these to ensure identical preimage construction. */
namespace {

std::string canonicalize_frame_set_preimage(std::span<const FrameSource> frames) {
  auto sorted = std::vector<FrameSource>(frames.begin(), frames.end());
  std::ranges::sort(sorted, {}, &FrameSource::id);
  std::string preimage;
  for (auto const& f : sorted) {
    preimage += f.id + "\n";
    preimage += f.frame_hash + "\n";
    preimage += std::to_string(f.image.width) + "x" + std::to_string(f.image.height) + "\n";
    preimage += std::to_string(f.pivot_x) + "," + std::to_string(f.pivot_y) + "\n";
    preimage += std::to_string(f.duration_ticks) + "\n";
    // color_space and alpha_mode excluded: PNG encode/decode round-trip
    // may not preserve these values faithfully across all codec paths.
  }
  return preimage;
}

std::string canonicalize_sample_table_preimage(std::span<const GeneratedFrameSample> samples) {
  auto sorted = std::vector<GeneratedFrameSample>(samples.begin(), samples.end());
  std::ranges::sort(sorted, {}, [](auto const& s) { return std::make_tuple(s.clip_id, s.frame_index, s.frame_id); });
  std::string preimage;
  for (auto const& s : sorted) {
    preimage += s.clip_id + "|" + std::to_string(s.frame_index) + "\n";
    preimage += s.frame_id + "\n";
    preimage += std::to_string(s.source_tick) + "\n";
    preimage += s.frame_hash + "\n";
    preimage += s.pose_hash + "\n";
  }
  return preimage;
}

std::string canonicalize_event_schedule_preimage(std::span<const GeneratedAnimationEvent> events) {
  auto sorted = std::vector<GeneratedAnimationEvent>(events.begin(), events.end());
  std::ranges::sort(sorted, {}, [](auto const& e) { return std::make_tuple(e.clip_id, e.event_id, e.authored_tick, e.frame_index, e.frame_id); });
  std::string preimage;
  for (auto const& ev : sorted) {
    preimage += ev.clip_id + "\n";
    preimage += ev.event_id + "\n";
    preimage += std::to_string(ev.authored_tick) + "\n";
    preimage += std::to_string(ev.frame_index) + "\n";
    preimage += ev.frame_id + "\n";
    preimage += std::to_string(ev.mapped_source_tick) + "\n";
  }
  return preimage;
}

std::string canonicalize_pose_table_preimage(std::span<const PoseHashRecord> records) {
  auto sorted = std::vector<PoseHashRecord>(records.begin(), records.end());
  std::ranges::sort(sorted, {}, [](auto const& r) { return std::make_tuple(r.clip_id, r.frame_index, r.frame_id); });
  std::string preimage;
  for (auto const& r : sorted) {
    preimage += r.clip_id + "|" + std::to_string(r.frame_index) + "\n";
    preimage += r.frame_id + "\n";
    preimage += r.pose_hash + "\n";
  }
  return preimage;
}

std::string canonicalize_frame_hash_table_preimage(std::span<const FrameHashRecord> records) {
  auto sorted = std::vector<FrameHashRecord>(records.begin(), records.end());
  std::ranges::sort(sorted, {}, &FrameHashRecord::frame_id);
  std::string preimage;
  for (auto const& r : sorted) {
    preimage += r.frame_id + "\n";
    preimage += r.frame_hash + "\n";
  }
  return preimage;
}

std::string canonicalize_generated_clip_set_preimage(std::span<const AnimationClip> clips) {
  auto sorted = std::vector<AnimationClip>(clips.begin(), clips.end());
  std::ranges::sort(sorted, {}, &AnimationClip::id);
  std::string preimage;
  for (auto const& c : sorted) {
    preimage += c.id + "\n";
    preimage += std::string(c.looping ? "1" : "0") + "\n";
    for (auto const& fid : c.frame_ids) preimage += fid + "\n";
    for (auto d : c.frame_durations) preimage += std::to_string(d) + "\n";
    for (auto const& ev : c.events) { preimage += ev.id; preimage += "|"; preimage += std::to_string(ev.tick); preimage += "\n"; }
  }
  return preimage;
}

std::string canonicalize_collision_set_preimage(std::span<const CollisionShape> shapes,
                                                 std::span<const CollisionWindow> windows) {
  auto sorted_shapes = std::vector<CollisionShape>(shapes.begin(), shapes.end());
  std::ranges::sort(sorted_shapes, {}, &CollisionShape::id);
  std::string preimage;
  for (auto const& s : sorted_shapes) {
    preimage += s.id + "\n";
    preimage += std::to_string(static_cast<int>(s.kind)) + "\n";
    preimage += s.bone_id + "\n";
    preimage += canonical_double(s.offset_x) + "," + canonical_double(s.offset_y) + "\n";
    preimage += canonical_double(s.extent_x) + "," + canonical_double(s.extent_y) + "\n";
  }
  auto sorted_windows = std::vector<CollisionWindow>(windows.begin(), windows.end());
  std::ranges::sort(sorted_windows, {}, [](auto const& w) { return std::make_tuple(w.start_tick, w.end_tick, w.shape_id, w.ability_id); });
  for (auto const& w : sorted_windows) {
    preimage += w.shape_id + "\n";
    preimage += w.ability_id + "\n";
    preimage += std::to_string(w.start_tick) + "\n";
    preimage += std::to_string(w.end_tick) + "\n";
    preimage += std::string(w.deals_damage ? "1" : "0") + "\n";
  }
  return preimage;
}

std::string canonicalize_channel_set_preimage(std::span<const ChannelMap> channels) {
  auto sorted = std::vector<ChannelMap>(channels.begin(), channels.end());
  std::ranges::sort(sorted, {}, [](auto const& ch) { return std::make_tuple(ch.id, ch.target_frame_id, static_cast<int>(ch.kind)); });
  std::string preimage;
  for (auto const& ch : sorted) {
    preimage += ch.id + "\n";
    preimage += ch.target_frame_id + "\n";
    preimage += std::to_string(static_cast<int>(ch.kind)) + "\n";
  }
  return preimage;
}

std::string canonicalize_morphology_preimage(const EffectiveMorphology& morph) {
  std::string preimage;
  for (auto const& [part_id, mp] : morph) {
    preimage += part_id + "\n";
    preimage += mp.bone_id + "\n";
    preimage += mp.primitive + "\n";
    preimage += mp.semantic_role + "\n";
    preimage += mp.color + "\n";
    preimage += mp.parent + "\n";
    preimage += canonical_double(mp.x) + "," + canonical_double(mp.y) + "," + canonical_double(mp.z) + "\n";
    preimage += canonical_double(mp.size_x) + "," + canonical_double(mp.size_y) + "," + canonical_double(mp.size_z) + "\n";
    preimage += canonical_double(mp.rotation_degrees) + "\n";
    preimage += std::to_string(mp.z_order) + "\n";
  }
  return preimage;
}

std::string canonicalize_atlas_preimage(std::span<const AtlasPlacement> placements,
                                         std::uint32_t atlas_width, std::uint32_t atlas_height) {
  auto sorted = std::vector<AtlasPlacement>(placements.begin(), placements.end());
  std::ranges::sort(sorted, {}, &AtlasPlacement::frame_id);
  std::string preimage;
  preimage += std::to_string(atlas_width) + "x" + std::to_string(atlas_height) + "\n";
  for (auto const& p : sorted) {
    preimage += p.frame_id + "\n";
    preimage += std::to_string(p.x) + "," + std::to_string(p.y) + "\n";
    preimage += std::to_string(p.width) + "x" + std::to_string(p.height) + "\n";
    preimage += std::to_string(p.pivot_x) + "," + std::to_string(p.pivot_y) + "\n";
    preimage += std::to_string(p.duration_ticks) + "\n";
  }
  return preimage;
}

} // anonymous namespace

/* ── Shared canonical source animation preimage ── */
std::string canonicalize_source_animation_set_preimage(std::span<const SkeletalClip> clips) {
  auto sorted = std::vector<SkeletalClip>(clips.begin(), clips.end());
  std::ranges::sort(sorted, {}, &SkeletalClip::id);
  std::string preimage;
  for (auto const& c : sorted) {
    preimage += c.id + "\n";
    preimage += std::to_string(c.duration_ticks) + "\n";
    preimage += std::string(c.looping ? "1" : "0") + "\n";
    for (auto const& t : c.tracks) {
      preimage += t.bone_id + "^{";
      for (auto const& k : t.keys) {
        preimage += std::to_string(k.tick) + ",";
        preimage += canonical_double(k.transform.x) + "," + canonical_double(k.transform.y) + ",";
        preimage += canonical_double(k.transform.rotation_degrees) + ",";
        preimage += canonical_double(k.transform.scale_x) + "," + canonical_double(k.transform.scale_y) + ";";
      }
      preimage += "}";
    }
    preimage += "\n";
    for (auto const& [ev_name, ev_tick] : c.events)
      preimage += ev_name + "|" + std::to_string(ev_tick) + ";";
    preimage += "\n";
  }
  return preimage;
}

static std::string compute_domain_id_impl(std::string_view domain, std::string_view preimage) {
  return sha256(std::string(domain) + "\n" + std::string(preimage));
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

    // Pose hashes — ID-addressed records
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaPoseHashes << "\",\"poses\":[";
      for (std::size_t i=0; i<input.samples.size(); ++i) {
        if (i) o << ","; auto const& s = input.samples[i];
        o << "{\"clip_id\":\"" << lv_escape(s.clip_id) << "\",\"frame_id\":\"" << lv_escape(s.frame_id)
          << "\",\"frame_index\":" << s.frame_index << ",\"pose_hash\":\"" << lv_escape(s.pose_hash) << "\"}";
      }
      o << "]}"; lv_write(staging/"pose-hashes.json", o.str());
    }
    // Frame hashes — ID-addressed records (single authority: use same FrameHashRecord vector as provenance)
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaFrameHashes << "\",\"frames\":[";
      for (std::size_t i=0; i<input.frames.size(); ++i) {
        if (i) o << ","; auto const& f = input.frames[i];
        o << "{\"frame_id\":\"" << lv_escape(f.id) << "\",\"frame_hash\":\"" << lv_escape(f.frame_hash) << "\"}";
      }
      o << "]}"; lv_write(staging/"frame-hashes.json", o.str());
    }
    // frames.json — governed frame metadata
    { std::ostringstream o; o << "{\"schema\":\"gspl.frames-2d/0.1\",\"frames\":[";
      for (std::size_t i=0; i<input.frames.size(); ++i) {
        if (i) o << ","; auto const& f = input.frames[i];
        o << "{\"id\":\"" << lv_escape(f.id)
          << "\",\"path\":\"frames/" << lv_escape(encode_pc(f.id)) << ".png\""
          << ",\"width\":" << f.image.width << ",\"height\":" << f.image.height
          << ",\"color_space\":\"" << (f.image.color_space == ColorSpace::srgb ? "srgb" : "data") << "\""
          << ",\"alpha_mode\":\"" << (f.image.alpha_mode == AlphaMode::opaque ? "opaque" : "straight") << "\""
          << ",\"pivot_x\":" << f.pivot_x << ",\"pivot_y\":" << f.pivot_y
          << ",\"duration_ticks\":" << f.duration_ticks
          << ",\"frame_hash\":\"" << lv_escape(f.frame_hash) << "\"}";
      }
      o << "]}"; lv_write(staging/"frames.json", o.str());
    }
    // ── Builder event authority validation (reject, never repair) ──
    for (auto const& e : input.events) {
      // Validate clip exists
      auto clip_it = std::find_if(input.generated_clips.begin(), input.generated_clips.end(),
        [&](auto const& c) { return c.id == e.clip_id; });
      if (clip_it == input.generated_clips.end())
        throw std::runtime_error("builder: event clip not found: " + e.clip_id);
      // Validate frame index in range
      if (e.frame_index >= clip_it->frame_ids.size())
        throw std::runtime_error("builder: event frame_index out of range: " + e.event_id);
      // Validate frame ID matches clip
      if (e.frame_id != clip_it->frame_ids[e.frame_index])
        throw std::runtime_error("builder: event frame_id mismatch: " + e.event_id);
      // Validate sample exists
      auto sample_it = std::find_if(input.samples.begin(), input.samples.end(),
        [&](auto const& s) { return s.clip_id == e.clip_id && s.frame_index == e.frame_index; });
      if (sample_it == input.samples.end())
        throw std::runtime_error("builder: no sample for event: " + e.event_id);
      // Validate sample consistency
      if (sample_it->frame_id != e.frame_id)
        throw std::runtime_error("builder: event/sample frame_id mismatch: " + e.event_id);
      if (sample_it->source_tick != e.mapped_source_tick)
        throw std::runtime_error("builder: event mapped_source_tick != sample source_tick: " + e.event_id);
      if (e.mapped_source_tick < e.authored_tick)
        throw std::runtime_error("builder: event mapped_source_tick < authored_tick: " + e.event_id);
      // Independent first-at-or-after proof via shared selector
      {
        auto expected = select_first_retained_sample_at_or_after(
            input.samples, e.clip_id, e.authored_tick);
        if (!expected)
          throw std::runtime_error("builder: no retained sample at-or-after authored tick: " + e.event_id);
        if (expected->get().frame_index != e.frame_index ||
            expected->get().frame_id != e.frame_id ||
            expected->get().source_tick != e.mapped_source_tick)
          throw std::runtime_error("builder: event not first-at-or-after sample: " + e.event_id);
      }
    }
    // Generated events — use synthesis's mapped_source_tick directly
    { std::ostringstream o; o << "{\"schema\":\"" << kSchemaGeneratedAnimationEvents << "\",\"events\":[";
      for (std::size_t i=0; i<input.events.size(); ++i) {
        if (i) o << ","; auto const& e = input.events[i];
        o << "{\"clip_id\":\"" << lv_escape(e.clip_id) << "\",\"event_id\":\"" << lv_escape(e.event_id)
          << "\",\"authored_tick\":" << e.authored_tick << ",\"frame_index\":" << e.frame_index
          << ",\"frame_id\":\"" << lv_escape(e.frame_id) << "\",\"mapped_source_tick\":" << e.mapped_source_tick << "}";
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

    // ── Build typed manifest with real dependencies and provenance ──
    LivingPackageManifest manifest;
    manifest.format = std::string(kSchemaLivingVisualPackage);
    manifest.identity_version = std::string(kIdentityPreimageVersion);
    manifest.entity_id = input.seed.stable_id;
    manifest.canonical_entity_identity = sha256(std::string("gspl.canonical-entity.identity/0.1\n") + seed_json);
    manifest.seed_identity = seed_id;
    manifest.frame_count = static_cast<std::uint32_t>(input.frames.size());
    manifest.clip_count = static_cast<std::uint32_t>(input.generated_clips.size());
    manifest.sample_count = static_cast<std::uint32_t>(input.samples.size());
    manifest.event_count = static_cast<std::uint32_t>(input.events.size());
    manifest.channel_count = static_cast<std::uint32_t>(input.channels.size());
    manifest.collision_shape_count = static_cast<std::uint32_t>(input.collision_shapes.size());
    manifest.collision_window_count = static_cast<std::uint32_t>(input.collision_windows.size());
    manifest.transformation_morphology_count = static_cast<std::uint32_t>(input.transformation_morphologies.size());

    // ── Gather all artifact records with real hashes/sizes ──
    // Helper: add an artifact with proper kind, schema, dependencies, provenance
    auto add_artifact = [&](std::string path, LivingArtifactKind kind,
                             std::vector<std::string> deps, std::string provenance) {
      auto bytes = lv_read(staging/path, 512ULL*1024*1024);
      LivingPackageArtifactRecord rec;
      rec.path = std::move(path);
      rec.kind = kind;
      rec.schema = std::string(artifact_schema_for_kind(kind));
      rec.byte_size = bytes.size();
      rec.sha256 = sha256(bytes);
      std::sort(deps.begin(), deps.end());
      rec.dependencies = std::move(deps);
      rec.provenance_identity = std::move(provenance);
      manifest.artifacts.push_back(std::move(rec));
    };

    // ── Compute governed provenance identities (shared functions ensure
    //     builder and verifier produce identical preimages) ──
    std::string frame_set_id = compute_domain_id_impl(
        kDomainFrameSet, canonicalize_frame_set_preimage(input.frames));
    std::string sample_table_id = compute_domain_id_impl(
        kDomainSampleTable, canonicalize_sample_table_preimage(input.samples));
    std::string event_sched_id = compute_domain_id_impl(
        kDomainEventSchedule, canonicalize_event_schedule_preimage(input.events));
    // Pose provenance from the exact records written to pose-hashes.json
    std::string pose_table_id;
    {
      std::vector<PoseHashRecord> pose_recs;
      for (auto const& s : input.samples)
        pose_recs.push_back({s.clip_id, s.frame_index, s.frame_id, s.pose_hash});
      pose_table_id = compute_domain_id_impl(
          kDomainPoseTable, canonicalize_pose_table_preimage(pose_recs));
    }
    std::string clip_set_id = compute_domain_id_impl(
        kDomainGeneratedClipSet, canonicalize_generated_clip_set_preimage(input.generated_clips));
    std::string collision_set_id = compute_domain_id_impl(
        kDomainCollisionSet, canonicalize_collision_set_preimage(input.collision_shapes, input.collision_windows));
    std::string channel_set_id = compute_domain_id_impl(
        kDomainChannelSet, canonicalize_channel_set_preimage(input.channels));
    std::string morph_set_id = compute_domain_id_impl(
        kDomainMorphologySet, canonicalize_morphology_preimage(input.base_morphology));
    std::string storm_morph_id = compute_domain_id_impl(
        kDomainMorphologySet, canonicalize_morphology_preimage(input.storm_morphology));
    std::string trans_morph_id;
    {
      std::string trans_preimage;
      for (std::size_t i = 0; i < input.transformation_morphologies.size(); ++i) {
        trans_preimage += std::to_string(i) + "\n";
        trans_preimage += canonicalize_morphology_preimage(input.transformation_morphologies[i]);
      }
      trans_morph_id = compute_domain_id_impl(kDomainMorphologySet, trans_preimage);
    }
    std::string atlas_id = compute_domain_id_impl(
        kDomainSpriteAtlas, canonicalize_atlas_preimage(
            input.sheet.atlas.placements, input.sheet.atlas.image.width, input.sheet.atlas.image.height));
    // Source-animation identity: semantic provenance from seed clips
    std::string source_anim_id;
    {
      source_anim_id = compute_domain_id_impl(
          kDomainSourceAnimSet,
          canonicalize_source_animation_set_preimage(input.seed.clips));
    }

    // Seed artifacts
    add_artifact("seed.json", LivingArtifactKind::living_seed, {}, seed_id);
    add_artifact("seed-identity.txt", LivingArtifactKind::seed_identity, {}, seed_id);

    // Frame metadata + images
    for (auto const& f : input.frames) {
      auto fpath = "frames/" + encode_pc(f.id) + ".png";
      add_artifact(fpath, LivingArtifactKind::frame_image, {"frames.json"}, f.frame_hash);
    }
    add_artifact("frames.json", LivingArtifactKind::frame_metadata, {}, frame_set_id);

    // Source skeletal animations
    add_artifact("source-skeletal-animations.json", LivingArtifactKind::source_skeletal_animations,
                 {"seed.json"}, source_anim_id);

    // Generated 2D animations
    add_artifact("animations-2d.json", LivingArtifactKind::generated_animation,
                 {"frames.json", "source-skeletal-animations.json"}, clip_set_id);

    // Frame samples
    add_artifact("frame-samples.json", LivingArtifactKind::frame_samples,
                 {"animations-2d.json", "frames.json"}, sample_table_id);

    // Frame-hash table identity (independent from frame-set)
    std::string frame_hash_table_id;
    {
      std::vector<FrameHashRecord> fh_recs;
      for (auto const& f : input.frames)
        fh_recs.push_back({f.id, f.frame_hash});
      frame_hash_table_id = compute_domain_id_impl(
          kDomainFrameHashTable, canonicalize_frame_hash_table_preimage(fh_recs));
    }

    // Frame hashes
    add_artifact("frame-hashes.json", LivingArtifactKind::frame_hashes,
                 {"frames.json", "frame-samples.json"}, frame_hash_table_id);

    // Pose hashes
    add_artifact("pose-hashes.json", LivingArtifactKind::pose_hashes,
                 {"frame-samples.json"}, pose_table_id);

    // Generated events
    add_artifact("animation-events.json", LivingArtifactKind::generated_events,
                 {"animations-2d.json", "frame-samples.json"}, event_sched_id);

    // Channels
    for (auto const& ch : input.channels) {
      auto ch_hash = sha256(std::string("gspl.channel-pixel.identity/0.1\n") + seed_id + ":" + ch.id);
      add_artifact("channels/" + encode_pc(ch.id) + ".png", LivingArtifactKind::channel_image,
                   {"channels.json", ("frames/" + encode_pc(ch.target_frame_id) + ".png")}, ch_hash);
    }
    add_artifact("channels.json", LivingArtifactKind::channel_metadata, {"frames.json"}, channel_set_id);

    // Collisions
    add_artifact("collisions-2d.json", LivingArtifactKind::collision_metadata, {"seed.json"}, collision_set_id);

    // Morphologies
    add_artifact("resolved-base-morphology.json", LivingArtifactKind::effective_morphology, {"seed.json"}, morph_set_id);
    add_artifact("resolved-storm-morphology.json", LivingArtifactKind::effective_morphology, {"seed.json"}, storm_morph_id);
    add_artifact("transformation-morphologies.json", LivingArtifactKind::transformation_morphologies,
                 {"seed.json"}, trans_morph_id);

    // Atlas — depends on all frame images + atlas metadata
    {
      std::vector<std::string> atlas_deps = {"sheet/atlas.json"};
      for (auto const& f : input.frames)
        atlas_deps.push_back("frames/" + encode_pc(f.id) + ".png");
      std::sort(atlas_deps.begin(), atlas_deps.end());
      add_artifact("sheet/atlas.png", LivingArtifactKind::sprite_atlas, std::move(atlas_deps), atlas_id);
    }
    add_artifact("sheet/atlas.json", LivingArtifactKind::sprite_atlas_metadata,
                 {"frames.json"}, atlas_id);

    manifest.artifact_count = static_cast<std::uint32_t>(manifest.artifacts.size());

    // Sort artifacts by path for canonical order
    std::sort(manifest.artifacts.begin(), manifest.artifacts.end(),
              [](auto const& a, auto const& b) { return a.path < b.path; });

    // Validate typed model
    PackageReadLimits vlim{};
    vlim.max_artifacts = 4096;
    vlim.max_path_bytes = 1024;
    vlim.max_json_tokens = 262144;
    vlim.max_json_nesting = 32;
    auto validation = validate_manifest_model(manifest, vlim);
    if (!validation.ok()) {
      std::ostringstream msgs;
      for (auto const& d : validation.diagnostics) msgs << d.code << ": " << d.message << "; ";
      throw std::runtime_error("manifest validation failed: " + msgs.str());
    }

    // Canonicalize without package identity, compute identity
    auto manifest_preimage = canonicalize_manifest(manifest, false);
    manifest.package_identity = sha256(std::string(kIdentityPreimageVersion) + "\n" + manifest_preimage);

    // Emit final manifest
    auto full_manifest = canonicalize_manifest(manifest, true);
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

    // ── Parse manifest through the typed strict parser ──
    auto manifest_bytes = lv_read(package_path/"manifest.json", limits.max_manifest_bytes);
    auto manifest_parse = parse_living_package_manifest(manifest_bytes, limits);
    if (!manifest_parse.ok()) {
      for (auto const& d : manifest_parse.diagnostics.diagnostics)
        result.diagnostics.diagnostics.push_back(d);
      add("LV_READ_MANIFEST_PARSE", "typed manifest parse/validation failed");
      return result;
    }

    // Require canonical manifest bytes — reject noncanonical ordering
    auto canonical_bytes = canonicalize_manifest(*manifest_parse.value, true);
    if (canonical_bytes != manifest_bytes)
      { add("LV_READ_MANIFEST_NONCANONICAL", "manifest bytes are not canonical"); return result; }

    // Independently recompute package identity
    auto manifest_preimage = canonicalize_manifest(*manifest_parse.value, false);
    auto computed_pkg_id = sha256(std::string(kIdentityPreimageVersion) + "\n" + manifest_preimage);
    if (computed_pkg_id != manifest_parse.value->package_identity)
      add("LV_READ_PKG_ID", "package identity recomputation mismatch");

    auto& m = *manifest_parse.value;

    LoadedLivingVisualPackage pkg;
    pkg.schema = m.format;
    pkg.entity_id = m.entity_id;
    pkg.canonical_entity_identity = m.canonical_entity_identity;
    pkg.seed_identity = m.seed_identity;
    pkg.package_identity = m.package_identity;
    pkg.manifest = std::move(m);

    // ── Validate artifact inventory (path confinement, byte sizes, SHA-256, file-set equality) ──
    PackageVerificationOptions inv_opts{};
    inv_opts.require_no_symlinks = true;
    inv_opts.require_no_undeclared_files = true;
    auto inv_result = validate_living_package_inventory(package_path, pkg.manifest, limits, inv_opts);
    if (!inv_result.ok()) {
      for (auto const& d : inv_result.diagnostics.diagnostics)
        add(d.code, d.message);
      return result;  // Stop before any semantic artifact access
    }
    auto& inv = *inv_result.value;

    // Inventory-based read helper: resolves path through validated inventory
    // Inventory helpers: inv_bytes returns verified immutable bytes, inv_read copies.
    auto inv_bytes = [&](std::string_view rel_path) -> std::string_view {
      auto it = inv.by_path.find(std::string(rel_path));
      if (it == inv.by_path.end())
        throw std::runtime_error("artifact not in inventory: " + std::string(rel_path));
      return *it->second.verified_bytes;
    };
    auto inv_read = [&](std::string_view rel_path, std::uint64_t max_bytes) -> std::string {
      auto b = inv_bytes(rel_path);
      if (b.size() > max_bytes)
        throw std::runtime_error("artifact exceeds byte limit: " + std::string(rel_path));
      return std::string(b);
    };

    // Embedded schema cross-check helper
    auto check_embedded_schema = [&](std::string_view rel_path, std::string_view bytes) {
      for (auto const& art : pkg.manifest.artifacts) {
        if (art.path == rel_path) {
          auto expected_schema = artifact_schema_for_kind(art.kind);
          auto embedded = extract_embedded_schema(bytes, limits);
          if (embedded != expected_schema)
            add("LV_READ_EMBEDDED_SCHEMA", std::string("embedded schema mismatch for ") + std::string(rel_path) + ": expected '" + std::string(expected_schema) + "' got '" + embedded + "'");
          return;
        }
      }
    };

    // ── Load and parse seed ──
    auto seed_wrapper = inv_read("seed.json", limits.max_artifact_bytes);
    check_embedded_schema("seed.json", seed_wrapper);
    std::string seed_json;
    {
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader wr(seed_wrapper, cfg);
      if (wr.begin_object("seed-wrapper")) {
        while (wr.has_more() && !wr.has_error()) {
          auto wk = wr.read_string_result(); if (!wk.ok()) break;
          if (!wr.require(':', "seed-wrapper")) break;
          if (*wk.value == "data") seed_json = wr.read_typed_value();
          else if (*wk.value == "schema") { wr.skip_value(); }
          else { wr.skip_value(); }
          wr.record_object_member("seed-wrapper");
          if (!wr.next_object_member("seed-wrapper")) break;
        }
        wr.end_object("seed-wrapper");
      }
    }
    if (seed_json.empty()) { add("LV_READ_NO_DATA", "seed wrapper missing data field"); return result; }
    pkg.seed = lv_parse_seed_json(seed_json);
    if (pkg.seed.stable_id.empty()) pkg.seed.stable_id = pkg.entity_id;
    pkg.seed_identity = sha256(seed_json);
    if (pkg.manifest.seed_identity != pkg.seed_identity)
      add("LV_READ_SEED_MISMATCH", "seed identity mismatch: manifest vs computed");

    // Reconstructed source skeletal animations (same data as written to source-skeletal-animations.json)
    pkg.source_skeletal_animations = pkg.seed.clips;

    // ── Reconstruct frames from frames.json + PNG files ──
    {
      auto fm_bytes = inv_read("frames.json", limits.max_artifact_bytes);
      check_embedded_schema("frames.json", fm_bytes);
      gspl::BoundedJsonConfig fcfg{}; fcfg.max_object_members = 512;
      gspl::BoundedJsonReader fmr(fm_bytes, fcfg);
      bool frames_schema_ok = false;
      if (fmr.begin_object("frames-json")) {
        while (fmr.has_more() && !fmr.has_error()) {
          auto fk = fmr.read_string_result(); if (!fk.ok()) break;
          if (!fmr.require(':', "frames-json")) break;
          if (*fk.value == "schema") {
            auto fschema = lv_rd_str(fmr, "frames.schema");
            if (fschema != kSchemaFrames2d)
              add("LV_READ_FRAMES_SCHEMA", "frames.json schema mismatch, expected " + std::string(kSchemaFrames2d));
            else frames_schema_ok = true;
          }
          else if (*fk.value == "frames") {
            if (!fmr.begin_array("frames-json.frames")) break;
            while (fmr.has_more() && !fmr.has_error()) {
              if (!fmr.begin_object("frame")) break;
              FrameSource fs;
              std::uint32_t declared_w = 0, declared_h = 0;
              while (fmr.has_more() && !fmr.has_error()) {
                auto fsk = fmr.read_string_result(); if (!fsk.ok()) break;
                if (!fmr.require(':', "frame")) break;
                if (*fsk.value == "id") fs.id = lv_rd_str(fmr, "frame.id");
                else if (*fsk.value == "path") { fmr.skip_value(); }
                else if (*fsk.value == "width") declared_w = lv_rd_u32(fmr, "frame.width");
                else if (*fsk.value == "height") declared_h = lv_rd_u32(fmr, "frame.height");
                else if (*fsk.value == "pivot_x") fs.pivot_x = lv_rd_i32(fmr, "frame.pivot_x");
                else if (*fsk.value == "pivot_y") fs.pivot_y = lv_rd_i32(fmr, "frame.pivot_y");
                else if (*fsk.value == "duration_ticks") fs.duration_ticks = lv_rd_u32(fmr, "frame.duration");
                else if (*fsk.value == "frame_hash") fs.frame_hash = lv_rd_str(fmr, "frame.hash");
                else if (*fsk.value == "color_space" || *fsk.value == "alpha_mode" || *fsk.value == "schema") { fmr.skip_value(); }
                else fmr.skip_value();
                fmr.record_object_member("frame");
                if (!fmr.next_object_member("frame")) break;
              }
              fmr.end_object("frame");
              // Load frame PNG through inventory
              auto frame_png_path = std::string("frames/") + encode_pc(fs.id) + ".png";
              try {
                auto png_bytes = inv_read(frame_png_path, limits.max_artifact_bytes);
                ImageLimits img_lim{};
                img_lim.max_width = limits.max_image_width;
                img_lim.max_height = limits.max_image_height;
                img_lim.max_decoded_bytes = limits.max_decoded_pixel_bytes;
                img_lim.max_input_bytes = limits.max_artifact_bytes;
                fs.image = decode_png(std::span<const std::byte>(
                    reinterpret_cast<const std::byte*>(png_bytes.data()), png_bytes.size()), img_lim);
                if (fs.image.width != declared_w || fs.image.height != declared_h)
                  add("LV_READ_FRAME_DIMS", "frame dimensions mismatch: " + fs.id);
                auto computed_hash = compute_frame_hash(fs.image);
                if (!fs.frame_hash.empty() && computed_hash != fs.frame_hash)
                  add("LV_READ_FRAME_HASH", "frame hash mismatch: " + fs.id);
                else if (fs.frame_hash.empty()) fs.frame_hash = computed_hash;
              } catch (std::exception const& ex) {
                add("LV_READ_FRAME_PNG", std::string("failed to decode frame PNG: ") + fs.id + " — " + ex.what());
              }
              if (!fs.id.empty()) pkg.frames.push_back(std::move(fs));
              fmr.record_array_element("frames-json.frames");
              if (!fmr.next_array_element("frames-json.frames")) break;
            }
            fmr.end_array("frames-json.frames");
          } else fmr.skip_value();
          fmr.record_object_member("frames-json");
          if (!fmr.next_object_member("frames-json")) break;
        }
        fmr.end_object("frames-json");
      }
      if (!frames_schema_ok) add("LV_READ_FRAMES_SCHEMA", "frames.json missing or invalid schema");
      if (pkg.frames.size() != 48)
        add("LV_READ_FRAME_COUNT", "expected 48 reconstructed frames, got " + std::to_string(pkg.frames.size()));
    }

    // ── Reconstruct generated clips from animations-2d.json ──
    {
      auto anim_bytes = inv_read("animations-2d.json", limits.max_artifact_bytes);
      check_embedded_schema("animations-2d.json", anim_bytes);
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader ar(anim_bytes, cfg);
      if (ar.begin_object("animations-2d")) {
        while (ar.has_more() && !ar.has_error()) {
          auto ak = ar.read_string_result(); if (!ak.ok()) break;
          if (!ar.require(':', "animations-2d")) break;
          if (*ak.value == "clips") {
            if (!ar.begin_array("anim.clips")) break;
            while (ar.has_more() && !ar.has_error()) {
              if (!ar.begin_object("genClip")) break;
              AnimationClip clip;
              while (ar.has_more() && !ar.has_error()) {
                auto ck = ar.read_string_result(); if (!ck.ok()) break;
                if (!ar.require(':', "genClip")) break;
                if (*ck.value == "id") clip.id = lv_rd_str(ar, "clip.id");
                else if (*ck.value == "looping") clip.looping = lv_rd_bool(ar, "clip.looping");
                else if (*ck.value == "frame_ids") {
                  if (!ar.begin_array("clip.frame_ids")) break;
                  while (ar.has_more() && !ar.has_error()) {
                    clip.frame_ids.push_back(lv_rd_str(ar, "clip.fid"));
                    ar.record_array_element("clip.frame_ids");
                    if (!ar.next_array_element("clip.frame_ids")) break;
                  }
                  ar.end_array("clip.frame_ids");
                } else if (*ck.value == "frame_durations") {
                  if (!ar.begin_array("clip.frame_durations")) break;
                  while (ar.has_more() && !ar.has_error()) {
                    clip.frame_durations.push_back(lv_rd_u32(ar, "clip.fdur"));
                    ar.record_array_element("clip.frame_durations");
                    if (!ar.next_array_element("clip.frame_durations")) break;
                  }
                  ar.end_array("clip.frame_durations");
                } else if (*ck.value == "events") {
                  if (ar.begin_array("clip.events")) {
                    while (ar.has_more() && !ar.has_error()) {
                      if (ar.begin_object("clipEv")) {
                        std::string eid; std::uint32_t etick = 0;
                        while (ar.has_more() && !ar.has_error()) {
                          auto ek = ar.read_string_result(); if (!ek.ok()) break;
                          if (!ar.require(':', "clipEv")) break;
                          if (*ek.value == "id") eid = lv_rd_str(ar, "ce.id");
                          else if (*ek.value == "tick") etick = lv_rd_u32(ar, "ce.tick");
                          else ar.skip_value();
                          ar.record_object_member("clipEv");
                          if (!ar.next_object_member("clipEv")) break;
                        }
                        ar.end_object("clipEv");
                        if (!eid.empty()) clip.events.push_back({eid, etick});
                      }
                      ar.record_array_element("clip.events");
                      if (!ar.next_array_element("clip.events")) break;
                    }
                    ar.end_array("clip.events");
                  }
                }
                else ar.skip_value();
                ar.record_object_member("genClip");
                if (!ar.next_object_member("genClip")) break;
              }
              ar.end_object("genClip");
              if (!clip.id.empty()) pkg.generated_clips.push_back(std::move(clip));
              ar.record_array_element("anim.clips");
              if (!ar.next_array_element("anim.clips")) break;
            }
            ar.end_array("anim.clips");
          } else ar.skip_value();
          ar.record_object_member("animations-2d");
          if (!ar.next_object_member("animations-2d")) break;
        }
        ar.end_object("animations-2d");
      }

      // Parse frame-samples.json
      {
        auto fs_bytes = inv_read("frame-samples.json", limits.max_artifact_bytes);
        check_embedded_schema("frame-samples.json", fs_bytes);
        gspl::BoundedJsonConfig cfg2{};
        gspl::BoundedJsonReader fsr(fs_bytes, cfg2);
        if (fsr.begin_object("frame-samples")) {
          while (fsr.has_more() && !fsr.has_error()) {
            auto fsk = fsr.read_string_result(); if (!fsk.ok()) break;
            if (!fsr.require(':', "frame-samples")) break;
            if (*fsk.value == "samples") {
              if (!fsr.begin_array("fs.samples")) break;
              while (fsr.has_more() && !fsr.has_error()) {
                if (!fsr.begin_object("sample")) break;
                GeneratedFrameSample s;
                while (fsr.has_more() && !fsr.has_error()) {
                  auto sk = fsr.read_string_result(); if (!sk.ok()) break;
                  if (!fsr.require(':', "sample")) break;
                  if (*sk.value == "clip_id") s.clip_id = lv_rd_str(fsr, "s.clip_id");
                  else if (*sk.value == "frame_id") s.frame_id = lv_rd_str(fsr, "s.frame_id");
                  else if (*sk.value == "frame_index") s.frame_index = lv_rd_u32(fsr, "s.frame_index");
                  else if (*sk.value == "source_tick") s.source_tick = lv_rd_u32(fsr, "s.source_tick");
                  else if (*sk.value == "pose_hash") s.pose_hash = lv_rd_str(fsr, "s.pose_hash");
                  else if (*sk.value == "frame_hash") s.frame_hash = lv_rd_str(fsr, "s.frame_hash");
                  else fsr.skip_value();
                  fsr.record_object_member("sample");
                  if (!fsr.next_object_member("sample")) break;
                }
                fsr.end_object("sample");
                if (!s.clip_id.empty()) pkg.samples.push_back(std::move(s));
                fsr.record_array_element("fs.samples");
                if (!fsr.next_array_element("fs.samples")) break;
              }
              fsr.end_array("fs.samples");
            } else fsr.skip_value();
            fsr.record_object_member("frame-samples");
            if (!fsr.next_object_member("frame-samples")) break;
          }
          fsr.end_object("frame-samples");
        }
      }

      // Parse animation-events.json
      {
        auto ev_bytes = inv_read("animation-events.json", limits.max_artifact_bytes);
        check_embedded_schema("animation-events.json", ev_bytes);
        gspl::BoundedJsonConfig cfg3{};
        gspl::BoundedJsonReader evr(ev_bytes, cfg3);
        if (evr.begin_object("anim-events")) {
          while (evr.has_more() && !evr.has_error()) {
            auto evk = evr.read_string_result(); if (!evk.ok()) break;
            if (!evr.require(':', "anim-events")) break;
            if (*evk.value == "events") {
              if (!evr.begin_array("ev.events")) break;
              while (evr.has_more() && !evr.has_error()) {
                if (!evr.begin_object("genEvent")) break;
                GeneratedAnimationEvent e;
                while (evr.has_more() && !evr.has_error()) {
                  auto ek = evr.read_string_result(); if (!ek.ok()) break;
                  if (!evr.require(':', "genEvent")) break;
                  if (*ek.value == "clip_id") e.clip_id = lv_rd_str(evr, "e.clip_id");
                  else if (*ek.value == "event_id") e.event_id = lv_rd_str(evr, "e.event_id");
                  else if (*ek.value == "authored_tick") e.authored_tick = lv_rd_u32(evr, "e.authored_tick");
                  else if (*ek.value == "frame_index") e.frame_index = lv_rd_u32(evr, "e.frame_index");
                  else if (*ek.value == "frame_id") e.frame_id = lv_rd_str(evr, "e.frame_id");
                  else if (*ek.value == "mapped_source_tick") e.mapped_source_tick = lv_rd_u32(evr, "e.mapped_tick");
                  else evr.skip_value();
                  evr.record_object_member("genEvent");
                  if (!evr.next_object_member("genEvent")) break;
                }
                evr.end_object("genEvent");
                if (!e.event_id.empty()) pkg.events.push_back(std::move(e));
                evr.record_array_element("ev.events");
                if (!evr.next_array_element("ev.events")) break;
              }
              evr.end_array("ev.events");
            } else evr.skip_value();
            evr.record_object_member("anim-events");
            if (!evr.next_object_member("anim-events")) break;
          }
          evr.end_object("anim-events");
        }
      }
    }

    // ── Reconstruct channels ──
    {
      auto ch_bytes = inv_read("channels.json", limits.max_artifact_bytes);
      check_embedded_schema("channels.json", ch_bytes);
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader chr(ch_bytes, cfg);
      if (chr.begin_object("channels")) {
        while (chr.has_more() && !chr.has_error()) {
          auto chk = chr.read_string_result(); if (!chk.ok()) break;
          if (!chr.require(':', "channels")) break;
          if (*chk.value == "maps") {
            if (!chr.begin_array("ch.maps")) break;
            while (chr.has_more() && !chr.has_error()) {
              if (!chr.begin_object("chMap")) break;
              ChannelMap cm;
              while (chr.has_more() && !chr.has_error()) {
                auto mk = chr.read_string_result(); if (!mk.ok()) break;
                if (!chr.require(':', "chMap")) break;
                if (*mk.value == "id") cm.id = lv_rd_str(chr, "cm.id");
                else if (*mk.value == "target_frame_id") cm.target_frame_id = lv_rd_str(chr, "cm.tid");
                else if (*mk.value == "kind") cm.kind = static_cast<ChannelMapKind>(lv_rd_u32(chr, "cm.kind"));
                else if (*mk.value == "width") { /* skip, loaded from PNG */ chr.skip_value(); }
                else if (*mk.value == "height") { chr.skip_value(); }
                else chr.skip_value();
                chr.record_object_member("chMap");
                if (!chr.next_object_member("chMap")) break;
              }
              chr.end_object("chMap");
              // Load channel PNG through inventory
              {
                auto ch_png_path = std::string("channels/") + encode_pc(cm.id) + ".png";
                auto ch_png = inv_read(ch_png_path, limits.max_artifact_bytes);
                ImageLimits ch_lim{};
                ch_lim.max_width = limits.max_image_width;
                ch_lim.max_height = limits.max_image_height;
                ch_lim.max_decoded_bytes = limits.max_decoded_pixel_bytes;
                ch_lim.max_input_bytes = limits.max_artifact_bytes;
                cm.image = decode_png(std::span<const std::byte>(
                    reinterpret_cast<const std::byte*>(ch_png.data()), ch_png.size()), ch_lim);
              }
              if (!cm.id.empty()) pkg.channels.push_back(std::move(cm));
              chr.record_array_element("ch.maps");
              if (!chr.next_array_element("ch.maps")) break;
            }
            chr.end_array("ch.maps");
          } else chr.skip_value();
          chr.record_object_member("channels");
          if (!chr.next_object_member("channels")) break;
        }
        chr.end_object("channels");
      }
    }

    // ── Reconstruct collisions ──
    {
      auto col_bytes = inv_read("collisions-2d.json", limits.max_artifact_bytes);
      check_embedded_schema("collisions-2d.json", col_bytes);
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader cr(col_bytes, cfg);
      if (cr.begin_object("collisions")) {
        while (cr.has_more() && !cr.has_error()) {
          auto ck = cr.read_string_result(); if (!ck.ok()) break;
          if (!cr.require(':', "collisions")) break;
          if (*ck.value == "shapes") {
            if (!cr.begin_array("col.shapes")) break;
            while (cr.has_more() && !cr.has_error()) {
              if (!cr.begin_object("colShape")) break;
              CollisionShape cs;
              while (cr.has_more() && !cr.has_error()) {
                auto sk = cr.read_string_result(); if (!sk.ok()) break;
                if (!cr.require(':', "colShape")) break;
                if (*sk.value == "id") cs.id = lv_rd_str(cr, "cs2.id");
                else if (*sk.value == "kind") cs.kind = static_cast<CollisionKind>(lv_rd_u32(cr, "cs2.kind"));
                else if (*sk.value == "bone_id") cs.bone_id = lv_rd_str(cr, "cs2.bone_id");
                else if (*sk.value == "offset_x") cs.offset_x = lv_rd_dbl(cr, "cs2.offset_x");
                else if (*sk.value == "offset_y") cs.offset_y = lv_rd_dbl(cr, "cs2.offset_y");
                else if (*sk.value == "extent_x") cs.extent_x = lv_rd_dbl(cr, "cs2.extent_x");
                else if (*sk.value == "extent_y") cs.extent_y = lv_rd_dbl(cr, "cs2.extent_y");
                else cr.skip_value();
                cr.record_object_member("colShape");
                if (!cr.next_object_member("colShape")) break;
              }
              cr.end_object("colShape");
              if (!cs.id.empty()) pkg.collision_shapes.push_back(std::move(cs));
              cr.record_array_element("col.shapes");
              if (!cr.next_array_element("col.shapes")) break;
            }
            cr.end_array("col.shapes");
          } else if (*ck.value == "windows") {
            if (!cr.begin_array("col.windows")) break;
            while (cr.has_more() && !cr.has_error()) {
              if (!cr.begin_object("colWin")) break;
              CollisionWindow cw;
              while (cr.has_more() && !cr.has_error()) {
                auto wk = cr.read_string_result(); if (!wk.ok()) break;
                if (!cr.require(':', "colWin")) break;
                if (*wk.value == "shape_id") cw.shape_id = lv_rd_str(cr, "cw2.shape_id");
                else if (*wk.value == "start_tick") cw.start_tick = lv_rd_u32(cr, "cw2.start");
                else if (*wk.value == "end_tick") cw.end_tick = lv_rd_u32(cr, "cw2.end");
                else if (*wk.value == "deals_damage") cw.deals_damage = lv_rd_bool(cr, "cw2.deals");
                else if (*wk.value == "ability_id") cw.ability_id = lv_rd_str(cr, "cw2.ability");
                else cr.skip_value();
                cr.record_object_member("colWin");
                if (!cr.next_object_member("colWin")) break;
              }
              cr.end_object("colWin");
              if (!cw.shape_id.empty()) pkg.collision_windows.push_back(std::move(cw));
              cr.record_array_element("col.windows");
              if (!cr.next_array_element("col.windows")) break;
            }
            cr.end_array("col.windows");
          } else cr.skip_value();
          cr.record_object_member("collisions");
          if (!cr.next_object_member("collisions")) break;
        }
        cr.end_object("collisions");
      }
    }

    // ── Reconstruct morphologies ──
    auto lv_parse_morph_json = [&](std::string_view bytes) -> EffectiveMorphology {
      EffectiveMorphology m;
      if (bytes.empty()) return m;
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader mr(bytes, cfg);
      if (mr.begin_object("morph")) {
        while (mr.has_more() && !mr.has_error()) {
          auto mk = mr.read_string_result(); if (!mk.ok()) break;
          if (!mr.require(':', "morph")) break;
          if (*mk.value == "parts") {
            if (!mr.begin_object("morph.parts")) break;
            while (mr.has_more() && !mr.has_error()) {
              auto pid = mr.read_string_result(); if (!pid.ok()) break;
              if (!mr.require(':', "morph.parts")) break;
              if (!mr.begin_object("morphPart3")) break;
              MorphologyPart mp;
              while (mr.has_more() && !mr.has_error()) {
                auto pk = mr.read_string_result(); if (!pk.ok()) break;
                if (!mr.require(':', "morphPart3")) break;
                if (*pk.value == "bone_id") mp.bone_id = lv_rd_str(mr, "mp3.bone_id");
                else if (*pk.value == "color") mp.color = lv_rd_str(mr, "mp3.color");
                else if (*pk.value == "parent") mp.parent = lv_rd_str(mr, "mp3.parent");
                else if (*pk.value == "primitive") mp.primitive = lv_rd_str(mr, "mp3.primitive");
                else if (*pk.value == "semantic_role") mp.semantic_role = lv_rd_str(mr, "mp3.semantic_role");
                else if (*pk.value == "x") mp.x = lv_rd_dbl(mr, "mp3.x");
                else if (*pk.value == "y") mp.y = lv_rd_dbl(mr, "mp3.y");
                else if (*pk.value == "z") mp.z = lv_rd_dbl(mr, "mp3.z");
                else if (*pk.value == "size_x") mp.size_x = lv_rd_dbl(mr, "mp3.size_x");
                else if (*pk.value == "size_y") mp.size_y = lv_rd_dbl(mr, "mp3.size_y");
                else if (*pk.value == "size_z") mp.size_z = lv_rd_dbl(mr, "mp3.size_z");
                else if (*pk.value == "rotation_degrees") mp.rotation_degrees = lv_rd_dbl(mr, "mp3.rot");
                else if (*pk.value == "z_order") mp.z_order = lv_rd_i32(mr, "mp3.z_order");
                else if (*pk.value == "emissive") mp.emissive = lv_rd_bool(mr, "mp3.emissive");
                else if (*pk.value == "electrical_marking") mp.electrical_marking = lv_rd_bool(mr, "mp3.elec");
                else mr.skip_value();
                mr.record_object_member("morphPart3");
                if (!mr.next_object_member("morphPart3")) break;
              }
              mr.end_object("morphPart3");
              if (pid.ok() && !pid.value->empty()) m[std::move(*pid.value)] = mp;
              mr.record_object_member("morph.parts");
              if (!mr.next_object_member("morph.parts")) break;
            }
            mr.end_object("morph.parts");
          } else mr.skip_value();
          mr.record_object_member("morph");
          if (!mr.next_object_member("morph")) break;
        }
        mr.end_object("morph");
      }
      return m;
    };
    {
      auto base_bytes = inv_read("resolved-base-morphology.json", limits.max_artifact_bytes);
      check_embedded_schema("resolved-base-morphology.json", base_bytes);
      pkg.base_morphology = lv_parse_morph_json(base_bytes);
    }
    {
      auto storm_bytes = inv_read("resolved-storm-morphology.json", limits.max_artifact_bytes);
      check_embedded_schema("resolved-storm-morphology.json", storm_bytes);
      pkg.storm_morphology = lv_parse_morph_json(storm_bytes);
    }
    {
      auto tm_bytes = inv_read("transformation-morphologies.json", limits.max_artifact_bytes);
      check_embedded_schema("transformation-morphologies.json", tm_bytes);
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader tmr(tm_bytes, cfg);
      if (tmr.begin_object("trans-morphs")) {
        while (tmr.has_more() && !tmr.has_error()) {
          auto tmk = tmr.read_string_result(); if (!tmk.ok()) break;
          if (!tmr.require(':', "trans-morphs")) break;
          if (*tmk.value == "morphologies") {
            if (!tmr.begin_array("tm.morphologies")) break;
            while (tmr.has_more() && !tmr.has_error()) {
              auto raw = tmr.read_typed_value();
              // Parse as inline morphology object
              gspl::BoundedJsonReader imr(raw, {});
              EffectiveMorphology em;
              if (imr.begin_object("transMorph")) {
                while (imr.has_more() && !imr.has_error()) {
                  auto pid = imr.read_string_result(); if (!pid.ok()) break;
                  if (!imr.require(':', "transMorph")) break;
                  if (!imr.begin_object("tmPart")) break;
                  MorphologyPart mp;
                  while (imr.has_more() && !imr.has_error()) {
                    auto pk = imr.read_string_result(); if (!pk.ok()) break;
                    if (!imr.require(':', "tmPart")) break;
                    if (*pk.value == "bone_id") mp.bone_id = lv_rd_str(imr, "tm.bone_id");
                    else if (*pk.value == "color") mp.color = lv_rd_str(imr, "tm.color");
                    else if (*pk.value == "parent") mp.parent = lv_rd_str(imr, "tm.parent");
                    else if (*pk.value == "primitive") mp.primitive = lv_rd_str(imr, "tm.primitive");
                    else if (*pk.value == "semantic_role") mp.semantic_role = lv_rd_str(imr, "tm.semantic_role");
                    else if (*pk.value == "x") mp.x = lv_rd_dbl(imr, "tm.x");
                    else if (*pk.value == "y") mp.y = lv_rd_dbl(imr, "tm.y");
                    else if (*pk.value == "z") mp.z = lv_rd_dbl(imr, "tm.z");
                    else if (*pk.value == "size_x") mp.size_x = lv_rd_dbl(imr, "tm.size_x");
                    else if (*pk.value == "size_y") mp.size_y = lv_rd_dbl(imr, "tm.size_y");
                    else if (*pk.value == "size_z") mp.size_z = lv_rd_dbl(imr, "tm.size_z");
                    else if (*pk.value == "rotation_degrees") mp.rotation_degrees = lv_rd_dbl(imr, "tm.rot");
                    else if (*pk.value == "z_order") mp.z_order = lv_rd_i32(imr, "tm.z_order");
                    else if (*pk.value == "emissive") mp.emissive = lv_rd_bool(imr, "tm.emissive");
                    else if (*pk.value == "electrical_marking") mp.electrical_marking = lv_rd_bool(imr, "tm.elec");
                    else imr.skip_value();
                    imr.record_object_member("tmPart");
                    if (!imr.next_object_member("tmPart")) break;
                  }
                  imr.end_object("tmPart");
                  if (pid.ok() && !pid.value->empty()) em[std::move(*pid.value)] = mp;
                  imr.record_object_member("transMorph");
                  if (!imr.next_object_member("transMorph")) break;
                }
                imr.end_object("transMorph");
              }
              pkg.transformation_morphologies.push_back(std::move(em));
              tmr.record_array_element("tm.morphologies");
              if (!tmr.next_array_element("tm.morphologies")) break;
            }
            tmr.end_array("tm.morphologies");
          } else tmr.skip_value();
          tmr.record_object_member("trans-morphs");
          if (!tmr.next_object_member("trans-morphs")) break;
        }
        tmr.end_object("trans-morphs");
      }
    }

    // ── Reconstruct sprite sheet ──
    {
      auto atlas_bytes = inv_read("sheet/atlas.png", limits.max_artifact_bytes);
      ImageLimits atlas_lim{};
      atlas_lim.max_width = limits.max_image_width;
      atlas_lim.max_height = limits.max_image_height;
      atlas_lim.max_decoded_bytes = limits.max_decoded_pixel_bytes;
      atlas_lim.max_input_bytes = limits.max_artifact_bytes;
      pkg.sheet.atlas.image = decode_png(std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(atlas_bytes.data()), atlas_bytes.size()), atlas_lim);
    }
    {
      auto aj_bytes = inv_read("sheet/atlas.json", limits.max_artifact_bytes);
      check_embedded_schema("sheet/atlas.json", aj_bytes);
      // Extract data field from wrapper
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader ajr(aj_bytes, cfg);
      if (ajr.begin_object("atlas-wrapper")) {
        while (ajr.has_more() && !ajr.has_error()) {
          auto ak = ajr.read_string_result(); if (!ak.ok()) break;
          if (!ajr.require(':', "atlas-wrapper")) break;
          if (*ak.value == "data") pkg.sheet.metadata = ajr.read_typed_value();
          else ajr.skip_value();
          ajr.record_object_member("atlas-wrapper");
          if (!ajr.next_object_member("atlas-wrapper")) break;
        }
        ajr.end_object("atlas-wrapper");
      }
    }

    // ── Cross-artifact hash verification: frame-hashes.json (ID-addressed) ──
    {
      auto fh_bytes = inv_read("frame-hashes.json", limits.max_artifact_bytes);
      check_embedded_schema("frame-hashes.json", fh_bytes);
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader fhr(fh_bytes, cfg);
      if (fhr.begin_object("frame-hashes")) {
        std::map<std::string, std::string> fh_map;
        while (fhr.has_more() && !fhr.has_error()) {
          auto fhk = fhr.read_string_result(); if (!fhk.ok()) break;
          if (!fhr.require(':', "frame-hashes")) break;
          if (*fhk.value == "frames") {
            if (!fhr.begin_array("fh.frames")) break;
            while (fhr.has_more() && !fhr.has_error()) {
              if (!fhr.begin_object("fhRec")) break;
              std::string fid, hash;
              while (fhr.has_more() && !fhr.has_error()) {
                auto rk = fhr.read_string_result(); if (!rk.ok()) break;
                if (!fhr.require(':', "fhRec")) break;
                if (*rk.value == "frame_id") fid = lv_rd_str(fhr, "fh.frame_id");
                else if (*rk.value == "frame_hash") hash = lv_rd_str(fhr, "fh.frame_hash");
                else fhr.skip_value();
                fhr.record_object_member("fhRec");
                if (!fhr.next_object_member("fhRec")) break;
              }
              fhr.end_object("fhRec");
              if (!fid.empty()) {
                if (fh_map.contains(fid)) add("LV_READ_FH_DUP", "duplicate frame_hash record: " + fid);
                else { fh_map[fid] = hash; pkg.frame_hash_records.push_back({fid, std::move(hash)}); }
              }
              fhr.record_array_element("fh.frames");
              if (!fhr.next_array_element("fh.frames")) break;
            }
            fhr.end_array("fh.frames");
          } else fhr.skip_value();
          fhr.record_object_member("frame-hashes");
          if (!fhr.next_object_member("frame-hashes")) break;
        }
        fhr.end_object("frame-hashes");
        // Cross-check frame hashes against samples
        for (auto const& s : pkg.samples) {
          auto it = fh_map.find(s.frame_id);
          if (it == fh_map.end()) add("LV_READ_FH_MISSING", "frame-hashes.json missing record for: " + s.frame_id);
          else if (it->second != s.frame_hash) add("LV_READ_FH_MISMATCH", "frame-hashes.json mismatch for: " + s.frame_id);
        }
      }
    }
    // ── Cross-artifact hash verification: pose-hashes.json (ID-addressed) ──
    {
      auto ph_bytes = inv_read("pose-hashes.json", limits.max_artifact_bytes);
      check_embedded_schema("pose-hashes.json", ph_bytes);
      gspl::BoundedJsonConfig cfg{};
      gspl::BoundedJsonReader phr(ph_bytes, cfg);
      if (phr.begin_object("pose-hashes")) {
        // Key: "clip_id|frame_index" → pose_hash
        std::map<std::string, std::string> ph_map;
        while (phr.has_more() && !phr.has_error()) {
          auto phk = phr.read_string_result(); if (!phk.ok()) break;
          if (!phr.require(':', "pose-hashes")) break;
          if (*phk.value == "poses") {
            if (!phr.begin_array("ph.poses")) break;
            while (phr.has_more() && !phr.has_error()) {
              if (!phr.begin_object("phRec")) break;
              std::string cid, fid, hash; std::uint32_t fi = 0;
              while (phr.has_more() && !phr.has_error()) {
                auto rk = phr.read_string_result(); if (!rk.ok()) break;
                if (!phr.require(':', "phRec")) break;
                if (*rk.value == "clip_id") cid = lv_rd_str(phr, "ph.clip_id");
                else if (*rk.value == "frame_id") fid = lv_rd_str(phr, "ph.frame_id");
                else if (*rk.value == "frame_index") fi = lv_rd_u32(phr, "ph.frame_index");
                else if (*rk.value == "pose_hash") hash = lv_rd_str(phr, "ph.pose_hash");
                else phr.skip_value();
                phr.record_object_member("phRec");
                if (!phr.next_object_member("phRec")) break;
              }
              phr.end_object("phRec");
              if (!cid.empty()) {
                auto key = cid + "|" + std::to_string(fi);
                if (ph_map.contains(key)) add("LV_READ_PH_DUP", "duplicate pose_hash record: " + key);
                else { ph_map[key] = hash; pkg.pose_hash_records.push_back({cid, fi, fid, std::move(hash)}); }
              }
              phr.record_array_element("ph.poses");
              if (!phr.next_array_element("ph.poses")) break;
            }
            phr.end_array("ph.poses");
          } else phr.skip_value();
          phr.record_object_member("pose-hashes");
          if (!phr.next_object_member("pose-hashes")) break;
        }
        phr.end_object("pose-hashes");
        // Cross-check pose hashes against samples
        for (auto const& s : pkg.samples) {
          auto key = s.clip_id + "|" + std::to_string(s.frame_index);
          auto it = ph_map.find(key);
          if (it == ph_map.end()) add("LV_READ_PH_MISSING", "pose-hashes.json missing record for: " + key);
          else if (it->second != s.pose_hash) add("LV_READ_PH_MISMATCH", "pose-hashes.json mismatch for: " + key);
        }
      }
    }
    // ── Clip/frame reference validation ──
    {
      std::set<std::string> frame_id_set;
      for (auto const& f : pkg.frames) frame_id_set.insert(f.id);
      std::set<std::string> referenced;
      for (auto const& c : pkg.generated_clips) {
        if (c.frame_ids.size() != c.frame_durations.size())
          add("LV_READ_CLIP_LEN", "clip frame_ids/durations mismatch: " + c.id);
        for (auto const& fid : c.frame_ids) {
          if (!frame_id_set.contains(fid))
            add("LV_READ_CLIP_REF", "clip references unknown frame: " + c.id + " -> " + fid);
          referenced.insert(fid);
        }
        for (auto const& d : c.frame_durations)
          if (d == 0) add("LV_READ_CLIP_DUR_ZERO", "clip has zero-duration frame: " + c.id);
        // Validate clip events have valid IDs
        for (auto const& ev : c.events)
          if (ev.id.empty()) add("LV_READ_CLIP_EV_EMPTY", "clip event with empty id: " + c.id);
      }
      for (auto const& fid : frame_id_set)
        if (!referenced.contains(fid))
          add("LV_READ_ORPHAN", "orphan frame not referenced by any clip: " + fid);
    }

    // ── Sample semantic validation ──
    {
      std::set<std::string> sample_positions;
      for (auto const& s : pkg.samples) {
        auto pos_key = s.clip_id + "|" + std::to_string(s.frame_index);
        if (sample_positions.contains(pos_key))
          add("LV_READ_DUP_SAMPLE", "duplicate sample position: " + pos_key);
        sample_positions.insert(pos_key);
        // Validate clip exists
        bool clip_found = false; std::string expected_fid;
        for (auto const& c : pkg.generated_clips) {
          if (c.id == s.clip_id) {
            clip_found = true;
            if (s.frame_index >= c.frame_ids.size())
              add("LV_READ_SAMPLE_OOB", "sample frame_index out of range: " + s.clip_id + " index=" + std::to_string(s.frame_index));
            else {
              expected_fid = c.frame_ids[s.frame_index];
              if (expected_fid != s.frame_id)
                add("LV_READ_SAMPLE_FID", "sample frame_id doesn't match clip: " + s.clip_id + " expected " + expected_fid + " got " + s.frame_id);
            }
            break;
          }
        }
        if (!clip_found) add("LV_READ_SAMPLE_CLIP", "sample references unknown clip: " + s.clip_id);
        // Validate hash format
        if (s.frame_hash.size() != 64) add("LV_READ_SAMPLE_FH_LEN", "invalid frame_hash length for sample in " + s.clip_id);
        if (s.pose_hash.size() != 64) add("LV_READ_SAMPLE_PH_LEN", "invalid pose_hash length for sample in " + s.clip_id);
      }
    }

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

    // Run comprehensive reader pipeline (schema, hash, reference, pixel validation)
    PackageReadLimits rlim;
    rlim.max_manifest_bytes = 4ULL*1024*1024;
    rlim.max_artifact_bytes = 512ULL*1024*1024;
    rlim.max_total_bytes = 2ULL*1024*1024*1024;
    rlim.max_artifacts = 4096;
    rlim.max_directory_entries = 8192;
    rlim.max_path_bytes = 1024;
    rlim.max_frames = 256;
    rlim.max_channels = 512;
    rlim.max_morphology_parts = 128;
    rlim.max_json_tokens = 262144;
    rlim.max_json_nesting = 32;
    rlim.max_image_width = 4096;
    rlim.max_image_height = 4096;
    rlim.max_decoded_pixel_bytes = 256ULL*1024*1024;
    auto read_result = read_living_visual_package(package_path, rlim);

    // Transfer any reader diagnostics to verifier result
    for (auto const& d : read_result.diagnostics.diagnostics)
      result.validation.diagnostics.push_back(d);

    if (!read_result.value.has_value()) {
      add("LV_VERIFY_READ_FAIL", "package reader failed to reconstruct");
      return result;
    }
    auto& pkg = *read_result.value;

    // Populate verification counts from reconstructed data
    result.package_identity = pkg.package_identity;
    result.seed_identity = pkg.seed_identity;
    result.frame_count = static_cast<std::uint32_t>(pkg.frames.size());
    result.clip_count = static_cast<std::uint32_t>(pkg.generated_clips.size());
    result.sample_count = static_cast<std::uint32_t>(pkg.samples.size());
    result.event_count = static_cast<std::uint32_t>(pkg.events.size());
    result.channel_count = static_cast<std::uint32_t>(pkg.channels.size());
    result.collision_shape_count = static_cast<std::uint32_t>(pkg.collision_shapes.size());
    result.collision_window_count = static_cast<std::uint32_t>(pkg.collision_windows.size());

    // Compare reconstructed counts with typed manifest
    if (result.frame_count != pkg.manifest.frame_count)
      add("LV_VERIFY_FRAME_CNT", "frame count mismatch: manifest=" + std::to_string(pkg.manifest.frame_count) + " reconstructed=" + std::to_string(result.frame_count));
    if (result.clip_count != pkg.manifest.clip_count)
      add("LV_VERIFY_CLIP_CNT", "clip count mismatch");
    if (result.sample_count != pkg.manifest.sample_count)
      add("LV_VERIFY_SAMPLE_CNT", "sample count mismatch");
    if (result.event_count != pkg.manifest.event_count)
      add("LV_VERIFY_EVENT_CNT", "event count mismatch");
    if (result.channel_count != pkg.manifest.channel_count)
      add("LV_VERIFY_CHANNEL_CNT", "channel count mismatch");
    if (result.collision_shape_count != pkg.manifest.collision_shape_count)
      add("LV_VERIFY_CSHAPE_CNT", "collision shape count mismatch");
    if (result.collision_window_count != pkg.manifest.collision_window_count)
      add("LV_VERIFY_CWIN_CNT", "collision window count mismatch");
    if (static_cast<std::uint32_t>(pkg.transformation_morphologies.size()) != pkg.manifest.transformation_morphology_count)
      add("LV_VERIFY_TRANSMORPH_CNT", "transformation morphology count mismatch");

    // Governance counts
    if (result.frame_count != 48) add("LV_VERIFY_FRAME_COUNT", "expected 48 frames");
    if (result.clip_count != 9) add("LV_VERIFY_CLIP_COUNT", "expected 9 clips");
    if (result.sample_count != 48) add("LV_VERIFY_SAMPLE_COUNT", "expected 48 samples");

    // ── Exact nine-clip contract (exact entity-derived IDs, no suffix matching) ──
    {
      struct GovernedClip { std::string id; std::uint32_t frames; bool looping; };
      auto& eid = pkg.entity_id;
      static const char* roles[] = {"base.idle","base.locomotion","base.attack","base.hit",
                                    "storm.idle","storm.locomotion","storm.attack","storm.hit"};
      static const std::uint32_t frame_counts[] = {4,6,6,3,4,6,6,3};
      static const bool loop_flags[] = {true,true,false,false,true,true,false,false};
      std::vector<GovernedClip> expected_clips;
      for (int i=0; i<8; ++i)
        expected_clips.push_back({eid + "." + roles[i], frame_counts[i], loop_flags[i]});
      expected_clips.push_back({eid + ".transform", 10, false});

      // Build index for duplicate detection
      std::map<std::string, const AnimationClip*> clip_index;
      for (auto const& c : pkg.generated_clips) {
        if (clip_index.contains(c.id))
          add("LV_CLIP_DUPLICATE", "duplicate clip ID: " + c.id);
        else
          clip_index[c.id] = &c;
      }

      for (auto const& exp : expected_clips) {
        auto it = clip_index.find(exp.id);
        if (it == clip_index.end()) {
          add("LV_CLIP_MISSING", "missing required clip: " + exp.id);
          continue;
        }
        auto const& c = *it->second;
        if (c.frame_ids.size() != exp.frames)
          add("LV_CLIP_FRAME_COUNT", "clip " + c.id + " frame count " + std::to_string(c.frame_ids.size()) + " != " + std::to_string(exp.frames));
        if (c.looping != exp.looping)
          add("LV_CLIP_LOOP", "clip " + c.id + " looping flag mismatch");
        if (c.frame_ids.size() != c.frame_durations.size())
          add("LV_CLIP_DUR_MISMATCH", "clip " + c.id + " frame/duration vector mismatch");
        for (auto const& fid : c.frame_ids) {
          auto frame_it = std::find_if(pkg.frames.begin(), pkg.frames.end(),
            [&](auto const& f) { return f.id == fid; });
          if (frame_it == pkg.frames.end())
            add("LV_CLIP_UNKNOWN_FRAME", "clip " + c.id + " references unknown frame: " + fid);
        }
        std::set<std::string> pos_seen;
        for (auto const& fid : c.frame_ids) {
          if (!pos_seen.insert(fid).second)
            add("LV_CLIP_DUP_POS", "clip " + c.id + " duplicate frame position: " + fid);
        }
      }
      // Check for extra clips
      for (auto const& c : pkg.generated_clips) {
        bool known = false;
        for (auto const& exp : expected_clips) {
          if (c.id == exp.id) { known = true; break; }
        }
        if (!known)
          add("LV_CLIP_EXTRA", "unexpected clip: " + c.id);
      }
    }

    // ── Exact 48-position sample coverage ──
    {
      std::set<std::tuple<std::string, std::uint32_t>> expected_positions;
      for (auto const& c : pkg.generated_clips) {
        for (std::uint32_t fi = 0; fi < c.frame_ids.size(); ++fi)
          expected_positions.emplace(c.id, fi);
      }
      std::set<std::tuple<std::string, std::uint32_t>> actual_positions;
      for (auto const& s : pkg.samples) {
        auto key = std::make_tuple(s.clip_id, s.frame_index);
        if (!actual_positions.insert(key).second)
          add("LV_SAMPLE_DUP", "duplicate sample: " + s.clip_id + "/" + std::to_string(s.frame_index));
      }
      for (auto const& exp_pos : expected_positions) {
        if (!actual_positions.count(exp_pos))
          add("LV_SAMPLE_MISSING", "missing sample: " + std::get<0>(exp_pos) + "/" + std::to_string(std::get<1>(exp_pos)));
      }
      for (auto const& act_pos : actual_positions) {
        if (!expected_positions.count(act_pos))
          add("LV_SAMPLE_EXTRA", "extra sample: " + std::get<0>(act_pos) + "/" + std::to_string(std::get<1>(act_pos)));
      }
      // Cross-validate sample fields
      for (auto const& s : pkg.samples) {
        auto clip_it = std::find_if(pkg.generated_clips.begin(), pkg.generated_clips.end(),
          [&](auto const& c) { return c.id == s.clip_id; });
        if (clip_it == pkg.generated_clips.end()) continue; // already caught above
        if (s.frame_index >= clip_it->frame_ids.size()) {
          add("LV_SAMPLE_OOB", "sample frame_index out of bounds: " + s.clip_id);
          continue;
        }
        if (s.frame_id != clip_it->frame_ids[s.frame_index])
          add("LV_SAMPLE_FRAME_ID", "sample frame_id mismatch: " + s.clip_id + "/" + std::to_string(s.frame_index));
      }
    }

    // ── Frame-hash exact-set equality ──
    {
      std::set<std::string> frame_ids;
      for (auto const& f : pkg.frames) frame_ids.insert(f.id);
      std::set<std::string> fh_ids;
      for (auto const& fh : pkg.frame_hash_records) {
        if (!fh_ids.insert(fh.frame_id).second)
          add("LV_FH_DUP", "duplicate frame-hash record: " + fh.frame_id);
        if (fh.frame_hash.size() != 64 || !std::all_of(fh.frame_hash.begin(), fh.frame_hash.end(),
              [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }))
          add("LV_FH_HASH_FORMAT", "malformed frame hash: " + fh.frame_id);
      }
      for (auto const& fid : frame_ids) {
        if (!fh_ids.count(fid))
          add("LV_FH_MISSING", "frame-hashes missing record: " + fid);
      }
      for (auto const& fid : fh_ids) {
        if (!frame_ids.count(fid))
          add("LV_FH_EXTRA", "frame-hashes extra record: " + fid);
      }
      // Cross-validate: frame-hash records vs decoded frames
      for (auto const& fh : pkg.frame_hash_records) {
        auto fit = std::find_if(pkg.frames.begin(), pkg.frames.end(),
          [&](auto const& f) { return f.id == fh.frame_id; });
        if (fit != pkg.frames.end() && fit->frame_hash != fh.frame_hash)
          add("LV_FH_FRAME_MISMATCH", "frame-hash record != decoded frame hash: " + fh.frame_id);
      }
      // Cross-validate: samples frame_hash vs frame-hash records
      for (auto const& s : pkg.samples) {
        auto fit = std::find_if(pkg.frame_hash_records.begin(), pkg.frame_hash_records.end(),
          [&](auto const& r) { return r.frame_id == s.frame_id; });
        if (fit != pkg.frame_hash_records.end() && fit->frame_hash != s.frame_hash)
          add("LV_FH_SAMPLE_MISMATCH", "sample frame_hash != frame-hash record: " + s.frame_id);
      }
    }

    // ── Pose exact-set equality ──
    {
      std::set<std::tuple<std::string, std::uint32_t>> sample_keys;
      for (auto const& s : pkg.samples) sample_keys.emplace(s.clip_id, s.frame_index);
      std::set<std::tuple<std::string, std::uint32_t>> pose_keys;
      for (auto const& pr : pkg.pose_hash_records) {
        auto key = std::make_tuple(pr.clip_id, pr.frame_index);
        if (!pose_keys.insert(key).second)
          add("LV_POSE_DUP", "duplicate pose record: " + pr.clip_id + "/" + std::to_string(pr.frame_index));
        if (pr.pose_hash.size() != 64 || !std::all_of(pr.pose_hash.begin(), pr.pose_hash.end(),
              [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }))
          add("LV_POSE_HASH_FORMAT", "malformed pose hash: " + pr.clip_id);
      }
      for (auto const& sk : sample_keys) {
        if (!pose_keys.count(sk))
          add("LV_POSE_MISSING", "pose record missing: " + std::get<0>(sk) + "/" + std::to_string(std::get<1>(sk)));
      }
      for (auto const& pk : pose_keys) {
        if (!sample_keys.count(pk))
          add("LV_POSE_EXTRA", "pose record extra: " + std::get<0>(pk) + "/" + std::to_string(std::get<1>(pk)));
      }
      // Cross-validate pose frame_id and hash against matching sample
      for (auto const& pr : pkg.pose_hash_records) {
        auto sample_it = std::find_if(pkg.samples.begin(), pkg.samples.end(),
          [&](auto const& s) { return s.clip_id == pr.clip_id && s.frame_index == pr.frame_index; });
        if (sample_it == pkg.samples.end()) continue;
        if (pr.frame_id != sample_it->frame_id)
          add("LV_POSE_FRAME_ID", "pose frame_id mismatch: " + pr.clip_id);
        if (pr.pose_hash != sample_it->pose_hash)
          add("LV_POSE_HASH", "pose hash mismatch: " + pr.clip_id);
      }
    }

    // ── Event exact-set: require exactly the 4 governed events (exact entity-derived IDs) ──
    {
      auto& eid = pkg.entity_id;
      struct GovernedEvent { std::string clip_id; std::string event_id; };
      const GovernedEvent governed[] = {
        {eid + ".base.attack", "release"},
        {eid + ".storm.attack", "release"},
        {eid + ".transform", "midpoint"},
        {eid + ".transform", "complete"},
      };
      // Index events for duplicate detection
      std::map<std::string, const GeneratedAnimationEvent*> ev_index;
      for (auto const& e : pkg.events) {
        std::string key = e.clip_id + "|" + e.event_id;
        if (ev_index.contains(key))
          add("LV_EVENT_DUPLICATE", "duplicate event: " + key);
        else
          ev_index[key] = &e;
      }

      for (auto const& gev : governed) {
        std::string gkey = gev.clip_id + "|" + gev.event_id;
        auto it = ev_index.find(gkey);
        if (it == ev_index.end()) {
          add("LV_EVENT_MISSING", "missing governed event: " + gkey);
          continue;
        }
        auto const& e = *it->second;
        // Validate event temporal authority
        if (e.mapped_source_tick < e.authored_tick)
          add("LV_EVENT_BEFORE_AUTHORED", "event mapped_tick < authored_tick: " + e.event_id);
        // Validate sample exists and mapped_tick matches
        auto sample_it = std::find_if(pkg.samples.begin(), pkg.samples.end(),
          [&](auto const& s) { return s.clip_id == e.clip_id && s.frame_index == e.frame_index; });
        if (sample_it == pkg.samples.end())
          add("LV_EVENT_SAMPLE_MISSING", "no sample for event: " + e.event_id);
        else if (sample_it->source_tick != e.mapped_source_tick)
          add("LV_EVENT_MAPPED_TICK", "event mapped_tick != sample source_tick: " + e.event_id);
        // Validate frame_id matches clip position
        auto clip_it = std::find_if(pkg.generated_clips.begin(), pkg.generated_clips.end(),
          [&](auto const& c) { return c.id == e.clip_id; });
        if (clip_it != pkg.generated_clips.end() && e.frame_index < clip_it->frame_ids.size()) {
          if (e.frame_id != clip_it->frame_ids[e.frame_index])
            add("LV_EVENT_FRAME_ID", "event frame_id mismatch: " + e.event_id);
        }
        // Independent first-at-or-after recomputation
        auto selected = select_first_retained_sample_at_or_after(pkg.samples, e.clip_id, e.authored_tick);
        if (!selected)
          add("LV_EVENT_SAMPLE_MISSING", "no retained sample at-or-after auth tick: " + e.event_id);
        else if (selected->get().source_tick != e.mapped_source_tick ||
                 selected->get().frame_index != e.frame_index)
          add("LV_EVENT_NOT_FIRST_AT_OR_AFTER", "event not mapped to first-at-or-after sample: " + e.event_id);
      }
      // Check for extra events
      for (auto const& e : pkg.events) {
        bool known = false;
        for (auto const& gev : governed) {
          if (e.clip_id == gev.clip_id && e.event_id == gev.event_id) { known = true; break; }
        }
        if (!known)
          add("LV_EVENT_EXTRA", "unexpected event: " + e.event_id + " in " + e.clip_id);
      }
      // Validate transform.complete maps to frame index 9 (terminal transform frame)
      std::string transform_id = eid + ".transform";
      for (auto const& e : pkg.events) {
        if (e.event_id == "complete" && e.clip_id == transform_id) {
          if (e.frame_index != 9)
            add("LV_EVENT_TRANSFORM_COMPLETION", "transform completion not at frame index 9 (got " + std::to_string(e.frame_index) + ")");
        }
      }
    }

    // ── Build artifact provenance lookup (O(1) map, not O(n) closure) ──
    std::map<std::string, std::string, std::less<>> art_prov;
    for (auto const& a : pkg.manifest.artifacts)
      art_prov[a.path] = a.provenance_identity;
    auto prov = [&](std::string_view path) -> std::string_view {
      auto it = art_prov.find(std::string(path));
      return it != art_prov.end() ? std::string_view(it->second) : std::string_view{};
    };

    // ── Mandatory semantic provenance (not gated by verify_pixel_hashes) ──
    // Canonical entity identity
    auto recomputed_canonical_entity = sha256(std::string(kDomainCanonicalEntity) + "\n" + canonicalize(pkg.seed));
    if (recomputed_canonical_entity != pkg.manifest.canonical_entity_identity)
      add("LV_PROV_CANONICAL_ENTITY", "canonical entity identity mismatch");

    // Frame-set identity (uses shared canonicalize_frame_set_preimage)
    {
      auto preimage = canonicalize_frame_set_preimage(pkg.frames);
      auto computed = compute_domain_id_impl(kDomainFrameSet, preimage);
      auto stored_frame_set = prov("frames.json");
      if (!stored_frame_set.empty() && computed != stored_frame_set)
        add("LV_PROV_FRAME_SET", "frame-set provenance mismatch");
    }

    // Sample-table identity
    {
      auto preimage = canonicalize_sample_table_preimage(pkg.samples);
      auto computed = compute_domain_id_impl(kDomainSampleTable, preimage);
      auto stored = prov("frame-samples.json");
      if (!stored.empty() && computed != stored)
        add("LV_PROV_SAMPLE_TABLE", "sample-table provenance mismatch");
    }

    // Event-schedule identity (uses shared canonicalize_event_schedule_preimage)
    {
      auto preimage = canonicalize_event_schedule_preimage(pkg.events);
      auto computed = compute_domain_id_impl(kDomainEventSchedule, preimage);
      auto stored = prov("animation-events.json");
      if (!stored.empty() && computed != stored)
        add("LV_PROV_EVENT_SCHEDULE", "event-schedule provenance mismatch");
    }

    // Pose-table identity (from reconstructed typed pose records)
    {
      auto preimage = canonicalize_pose_table_preimage(pkg.pose_hash_records);
      auto computed = compute_domain_id_impl(kDomainPoseTable, preimage);
      auto stored = prov("pose-hashes.json");
      if (!stored.empty() && computed != stored)
        add("LV_PROV_POSE_TABLE", "pose-table provenance mismatch");
    }

    // Frame-hashes.json provenance (independent domain from frame-set)
    if (!pkg.frame_hash_records.empty()) {
      auto preimage = canonicalize_frame_hash_table_preimage(pkg.frame_hash_records);
      auto computed = compute_domain_id_impl(kDomainFrameHashTable, preimage);
      auto stored = prov("frame-hashes.json");
      if (!stored.empty() && computed != stored)
        add("LV_PROV_FRAME_HASH_TABLE", "frame-hash-table provenance mismatch");
    }

    // NOTE: Source-animation provenance (kDomainSourceAnimSet) is declared in the manifest
    // but verification requires parsing source-skeletal-animations.json directly from the
    // immutable inventory bytes rather than from seed.json (which loses float precision).
    // Deferred until source-skeletal-animations.json has its own parser in the reader.

    // Generated-clip-set identity
    {
      auto preimage = canonicalize_generated_clip_set_preimage(pkg.generated_clips);
      auto computed = compute_domain_id_impl(kDomainGeneratedClipSet, preimage);
      auto stored = prov("animations-2d.json");
      if (!stored.empty() && computed != stored)
        add("LV_PROV_GENERATED_CLIPS", "generated-clip-set provenance mismatch");
    }

    // Collision-set identity (from reconstructed collision artifacts, not seed)
    {
      auto preimage = canonicalize_collision_set_preimage(
          pkg.collision_shapes, pkg.collision_windows);
      auto computed = compute_domain_id_impl(kDomainCollisionSet, preimage);
      auto stored = prov("collisions-2d.json");
      if (!stored.empty() && computed != stored)
        add("LV_PROV_COLLISIONS", "collision-set provenance mismatch");
    }

    // ── Optional pixel-level checks (gated by verify_pixel_hashes) ──
    if (options.verify_pixel_hashes) {
      // Channel-set identity
      if (!pkg.channels.empty()) {
        auto preimage = canonicalize_channel_set_preimage(pkg.channels);
        auto computed = compute_domain_id_impl(kDomainChannelSet, preimage);
        auto stored = prov("channels.json");
        if (!stored.empty() && computed != stored)
          add("LV_PROV_CHANNEL_SET", "channel-set provenance mismatch");
      }

      // Base morphology identity
      if (!pkg.base_morphology.empty()) {
        auto preimage = canonicalize_morphology_preimage(pkg.base_morphology);
        auto computed = compute_domain_id_impl(kDomainMorphologySet, preimage);
        auto stored = prov("resolved-base-morphology.json");
        if (!stored.empty() && computed != stored)
          add("LV_PROV_BASE_MORPHOLOGY", "base morphology provenance mismatch");
      }

      // Storm morphology identity
      if (!pkg.storm_morphology.empty()) {
        auto preimage = canonicalize_morphology_preimage(pkg.storm_morphology);
        auto computed = compute_domain_id_impl(kDomainMorphologySet, preimage);
        auto stored = prov("resolved-storm-morphology.json");
        if (!stored.empty() && computed != stored)
          add("LV_PROV_STORM_MORPHOLOGY", "storm morphology provenance mismatch");
      }

      // Transformation morphology sequence identity
      if (!pkg.transformation_morphologies.empty()) {
        std::string trans_preimage;
        for (std::size_t i = 0; i < pkg.transformation_morphologies.size(); ++i) {
          trans_preimage += std::to_string(i) + "\n";
          trans_preimage += canonicalize_morphology_preimage(pkg.transformation_morphologies[i]);
        }
        auto computed = compute_domain_id_impl(kDomainMorphologySet, trans_preimage);
        auto stored = prov("transformation-morphologies.json");
        if (!stored.empty() && computed != stored)
          add("LV_PROV_TRANSFORMATION_MORPHOLOGIES", "transformation morphologies provenance mismatch");
      }

      // Atlas identity (reconstructed from typed atlas metadata)
      if (!pkg.sheet.atlas.placements.empty()) {
        auto preimage = canonicalize_atlas_preimage(
            pkg.sheet.atlas.placements,
            pkg.sheet.atlas.image.width,
            pkg.sheet.atlas.image.height);
        auto computed = compute_domain_id_impl(kDomainSpriteAtlas, preimage);
        auto stored_meta = prov("sheet/atlas.json");
        auto stored_png = prov("sheet/atlas.png");
        if (!stored_meta.empty() && computed != stored_meta)
          add("LV_PROV_ATLAS", "atlas metadata provenance mismatch");
        if (!stored_png.empty() && computed != stored_png)
          add("LV_PROV_ATLAS", "atlas image provenance mismatch");
      }
    }
    if (pkg.manifest.transformation_morphology_count != 10)
      add("LV_VERIFY_TRANSMORPH_COUNT", "expected 10 transformation morphologies");

    // Package identity already verified by reader (parse + canonicalize + recompute).
    // Encoded artifact hashes, path confinement, symlink safety, byte sizes, and
    // file-set equality are all validated by the inventory during reader construction.
    // No duplicate validation needed here.

    // ── Undeclared files check (reader inventory already checked this, but
    //     verify independently if options require it) ──
    if (options.require_no_undeclared_files) {
      std::set<std::string> declared;
      for (auto const& art : pkg.manifest.artifacts) declared.insert(art.path);
      for (auto const& e : std::filesystem::recursive_directory_iterator(package_path)) {
        if (e.is_regular_file()) {
          if (std::filesystem::is_symlink(std::filesystem::symlink_status(e.path())))
            { add("LV_VERIFY_SYMLINK_FILE", "symlink file in package: " + e.path().filename().string()); continue; }
          auto rel = e.path().lexically_relative(package_path).generic_string();
          if (rel != "manifest.json" && !declared.contains(rel))
            add("LV_VERIFY_UNDECLARED", "undeclared file: "+rel);
        } else if (std::filesystem::is_symlink(std::filesystem::symlink_status(e.path()))) {
          auto rel = e.path().lexically_relative(package_path).generic_string();
          add("LV_VERIFY_SYMLINK_ENTRY", "symlink entry in package: " + rel);
        }
      }
    }
  } catch (std::exception const& e) { add("LV_VERIFY_ERROR", e.what()); }
  return result;
}

/* ── Strict typed manifest parser (placed here to access lv_rd_* helpers) ── */
LivingPackageManifestParseResult parse_living_package_manifest(std::string_view bytes, const PackageReadLimits& limits) {
  LivingPackageManifestParseResult result;
  auto add = [&](std::string code, std::string msg) { result.diagnostics.diagnostics.push_back({std::move(code), std::move(msg)}); };

  try {
    if (bytes.size() > limits.max_manifest_bytes)
      { add("LV_PARSE_OVERSIZE", "manifest exceeds byte limit"); return result; }

    gspl::BoundedJsonConfig cfg{};
    cfg.max_object_members = 256;
    cfg.max_array_length = limits.max_artifacts;
    cfg.max_nesting_depth = limits.max_json_nesting;
    cfg.max_input_bytes = bytes.size() + 1;
    gspl::BoundedJsonReader r(bytes, cfg);

    LivingPackageManifest m;
    std::set<std::string> seen_fields;

    if (!r.begin_object("manifest-parse"))
      { add("LV_PARSE_NOT_OBJECT", "manifest is not a JSON object"); return result; }

    while (r.has_more() && !r.has_error()) {
      auto key = r.read_string_result();
      if (!key.ok()) { add("LV_PARSE_KEY", "manifest key parse error"); break; }
      if (!r.require(':', "manifest-parse")) break;

      auto const& k = *key.value;
      if (!seen_fields.insert(k).second)
        { add("LV_PARSE_DUP_KEY", "duplicate manifest key: " + k); r.skip_value(); }
      else if (k == "format")              m.format = lv_rd_str(r, "m.format");
      else if (k == "identityVersion")    m.identity_version = lv_rd_str(r, "m.identityVersion");
      else if (k == "packageIdentity")    m.package_identity = lv_rd_str(r, "m.packageIdentity");
      else if (k == "entityId")           m.entity_id = lv_rd_str(r, "m.entityId");
      else if (k == "canonicalEntityIdentity") m.canonical_entity_identity = lv_rd_str(r, "m.canonicalEntityIdentity");
      else if (k == "seedIdentity")       m.seed_identity = lv_rd_str(r, "m.seedIdentity");
      else if (k == "frameCount")         m.frame_count = lv_rd_u32(r, "m.frameCount");
      else if (k == "clipCount")          m.clip_count = lv_rd_u32(r, "m.clipCount");
      else if (k == "sampleCount")        m.sample_count = lv_rd_u32(r, "m.sampleCount");
      else if (k == "eventCount")         m.event_count = lv_rd_u32(r, "m.eventCount");
      else if (k == "channelCount")       m.channel_count = lv_rd_u32(r, "m.channelCount");
      else if (k == "collisionShapeCount")      m.collision_shape_count = lv_rd_u32(r, "m.collisionShapeCount");
      else if (k == "collisionWindowCount")    m.collision_window_count = lv_rd_u32(r, "m.collisionWindowCount");
      else if (k == "transformationMorphologyCount") m.transformation_morphology_count = lv_rd_u32(r, "m.transformationMorphologyCount");
      else if (k == "artifactCount")     m.artifact_count = lv_rd_u32(r, "m.artifactCount");
      else if (k == "artifacts") {
        if (!r.begin_array("parse.artifacts")) break;
        while (r.has_more() && !r.has_error()) {
          if (m.artifacts.size() >= limits.max_artifacts)
            { add("LV_PARSE_ARTIFACT_LIMIT", "artifact array exceeds limit"); break; }
          if (!r.begin_object("parse.artifact")) break;
          LivingPackageArtifactRecord rec;
          std::set<std::string> art_seen;
          while (r.has_more() && !r.has_error()) {
            auto ak = r.read_string_result(); if (!ak.ok()) break;
            if (!r.require(':', "parse.artifact")) break;
            if (!art_seen.insert(*ak.value).second)
              { add("LV_PARSE_DUP_ART_KEY", "duplicate artifact key: " + *ak.value); r.skip_value(); }
            else if (*ak.value == "path")       rec.path = lv_rd_str(r, "a.path");
            else if (*ak.value == "kind") {
              auto kind_str = lv_rd_str(r, "a.kind");
              auto kind_opt = artifact_kind_from_string(kind_str);
              if (!kind_opt) add("LV_PARSE_UNKNOWN_KIND", "unknown artifact kind: " + kind_str);
              else rec.kind = *kind_opt;
            }
            else if (*ak.value == "schema")     rec.schema = lv_rd_str(r, "a.schema");
            else if (*ak.value == "byteSize")   rec.byte_size = lv_rd_u64(r, "a.byteSize");
            else if (*ak.value == "sha256")     rec.sha256 = lv_rd_str(r, "a.sha256");
            else if (*ak.value == "provenanceIdentity") rec.provenance_identity = lv_rd_str(r, "a.provenanceIdentity");
            else if (*ak.value == "dependencies") {
              if (!r.begin_array("a.deps")) break;
              while (r.has_more() && !r.has_error()) {
                rec.dependencies.push_back(lv_rd_str(r, "a.dep"));
                r.record_array_element("a.deps");
                if (!r.next_array_element("a.deps")) break;
              }
              r.end_array("a.deps");
            } else { add("LV_PARSE_UNKNOWN_ART", "unknown artifact field: " + *ak.value); r.skip_value(); }
            r.record_object_member("parse.artifact");
            if (!r.next_object_member("parse.artifact")) break;
          }
          r.end_object("parse.artifact");
          // Require all artifact fields exactly once
          for (auto const& req : {"path","kind","schema","byteSize","sha256","dependencies","provenanceIdentity"}) {
            if (!art_seen.contains(req)) add("LV_PARSE_MISS_ART", std::string("missing artifact field: ") + req);
          }
          m.artifacts.push_back(std::move(rec));
          r.record_array_element("parse.artifacts");
          if (!r.next_array_element("parse.artifacts")) break;
        }
        r.end_array("parse.artifacts");
      } else { add("LV_PARSE_UNKNOWN_TOP", "unknown top-level field: " + k); r.skip_value(); }
      r.record_object_member("manifest-parse");
      if (!r.next_object_member("manifest-parse")) break;
    }
    r.end_object("manifest-parse");

    if (r.has_error())
      { add("LV_PARSE_ERROR", "manifest JSON parse error"); return result; }

    // Require all top-level fields exactly once
    for (auto const& req : {"format","identityVersion","packageIdentity","entityId","canonicalEntityIdentity","seedIdentity","frameCount","clipCount","sampleCount","eventCount","channelCount","collisionShapeCount","collisionWindowCount","transformationMorphologyCount","artifactCount","artifacts"}) {
      if (!seen_fields.contains(req)) { add("LV_PARSE_MISS_FIELD", std::string("missing top-level field: ") + req); }
    }

    if (!result.diagnostics.ok()) return result;

    // Validate the parsed model before returning success
    auto vres = validate_manifest_model(m, limits);
    for (auto const& d : vres.diagnostics) result.diagnostics.diagnostics.push_back(d);

    if (!result.diagnostics.ok()) return result;
    result.value = std::move(m);
  } catch (std::exception const& e) { add("LV_PARSE_EXCEPTION", e.what()); }
  return result;
}

/* ── Canonical manifest serialization (assumes validated model) ── */
std::string canonicalize_manifest(const LivingPackageManifest& m, bool include_package_identity) {
  std::string out;
  out += "{\"format\":\""; out += lv_escape(m.format); out += "\"";
  out += ",\"identityVersion\":\""; out += lv_escape(m.identity_version); out += "\"";
  if (include_package_identity) { out += ",\"packageIdentity\":\""; out += lv_escape(m.package_identity); out += "\""; }
  out += ",\"entityId\":\""; out += lv_escape(m.entity_id); out += "\"";
  out += ",\"canonicalEntityIdentity\":\""; out += lv_escape(m.canonical_entity_identity); out += "\"";
  out += ",\"seedIdentity\":\""; out += lv_escape(m.seed_identity); out += "\"";
  out += ",\"frameCount\":"; out += std::to_string(m.frame_count);
  out += ",\"clipCount\":"; out += std::to_string(m.clip_count);
  out += ",\"sampleCount\":"; out += std::to_string(m.sample_count);
  out += ",\"eventCount\":"; out += std::to_string(m.event_count);
  out += ",\"channelCount\":"; out += std::to_string(m.channel_count);
  out += ",\"collisionShapeCount\":"; out += std::to_string(m.collision_shape_count);
  out += ",\"collisionWindowCount\":"; out += std::to_string(m.collision_window_count);
  out += ",\"transformationMorphologyCount\":"; out += std::to_string(m.transformation_morphology_count);
  out += ",\"artifactCount\":"; out += std::to_string(m.artifacts.size());
  out += ",\"artifacts\":[";
  for (std::size_t i = 0; i < m.artifacts.size(); ++i) {
    if (i) out += ",";
    auto const& a = m.artifacts[i];
    out += "{\"path\":\""; out += lv_escape(a.path); out += "\"";
    out += ",\"kind\":\""; out += artifact_kind_string(a.kind); out += "\"";
    out += ",\"schema\":\""; out += lv_escape(a.schema); out += "\"";
    out += ",\"byteSize\":"; out += std::to_string(a.byte_size);
    out += ",\"sha256\":\""; out += lv_escape(a.sha256); out += "\"";
    out += ",\"dependencies\":[";
    for (std::size_t j = 0; j < a.dependencies.size(); ++j) {
      if (j) out += ",";
      out += "\""; out += lv_escape(a.dependencies[j]); out += "\"";
    }
    out += "],\"provenanceIdentity\":\""; out += lv_escape(a.provenance_identity); out += "\"}";
  }
  out += "]}";
  return out;
}

} // namespace gspl::sprites
