#include "gspl_sprites/package.hpp"

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/target_contract.hpp"
#include "package_semantics.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <limits>
#include <set>
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
        if (consume(']')) break; expect(",");
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
    if(!consume('"'))fail("expected JSON string");std::string result;
    while(cursor_<source_.size()){
      const unsigned char c=static_cast<unsigned char>(source_[cursor_++]);
      if(c=='"')return result;
      if(c<0x20)fail("control character in JSON string");
      if(c!='\\'){result+=static_cast<char>(c);continue;}
      if(cursor_>=source_.size())fail("truncated JSON escape");const char escaped=source_[cursor_++];
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
      if(!previous.empty()&&artifact.path<=previous)add("SPRITE_PACKAGE_MANIFEST_NONCANONICAL","artifact paths are not strictly sorted");previous=artifact.path;
      if(!lowercase_sha256(artifact.hash)){add("SPRITE_PACKAGE_HASH_INVALID","invalid SHA-256 for: "+artifact.path);continue;}
      auto current=root;bool unsafe_component=false;std::size_t start=0;while(start<artifact.path.size()){const auto end=artifact.path.find('/',start);const auto part=artifact.path.substr(start,end==std::string::npos?artifact.path.size()-start:end-start);current/=part;if(std::filesystem::exists(current)&&std::filesystem::is_symlink(std::filesystem::symlink_status(current))){unsafe_component=true;break;}if(end==std::string::npos)break;start=end+1;}
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
} // namespace gspl::sprites
