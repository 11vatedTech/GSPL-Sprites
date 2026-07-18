#include "gspl_sprites/godot_export.hpp"

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
constexpr std::array<std::string_view, 5> artifacts{
    "assets/entity.glb", "gspl_sprite.tscn", "project.godot",
    "source-evidence.json", "target-report.json"};

std::string bytes_hash(std::span<const std::byte> bytes) {
  return sha256({reinterpret_cast<const char *>(bytes.data()), bytes.size()});
}

bool lowercase_sha256(std::string_view value) {
  return value.size() == 64 &&
         std::ranges::all_of(value, [](unsigned char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

std::string
canonical_source_evidence(const std::optional<TargetSourceEvidence> &evidence) {
  if (!evidence)
    return "{\"sourcePackage\":null}";
  if (!lowercase_sha256(evidence->package_identity()) ||
      !lowercase_sha256(evidence->seed_identity()) ||
      !lowercase_sha256(evidence->authoring_provenance_sha256()) ||
      !lowercase_sha256(evidence->target_compatibility_sha256()))
    throw std::invalid_argument("Godot source package evidence is invalid");
  return "{\"sourcePackage\":{\"authoringProvenanceSha256\":\"" +
         evidence->authoring_provenance_sha256() + "\",\"packageIdentity\":\"" +
         evidence->package_identity() + "\",\"seedIdentity\":\"" +
         evidence->seed_identity() + "\",\"targetCompatibilitySha256\":\"" +
         evidence->target_compatibility_sha256() + "\"}}";
}

bool valid_source_evidence(std::string_view value) {
  if (value == "{\"sourcePackage\":null}")
    return true;
  constexpr std::array<std::string_view, 5> tokens{
      "{\"sourcePackage\":{\"authoringProvenanceSha256\":\"",
      "\",\"packageIdentity\":\"", "\",\"seedIdentity\":\"",
      "\",\"targetCompatibilitySha256\":\"", "\"}}"};
  std::size_t cursor{};
  for (std::size_t index = 0; index < 4; ++index) {
    if (value.substr(cursor, tokens[index].size()) != tokens[index])
      return false;
    cursor += tokens[index].size();
    if (!lowercase_sha256(value.substr(cursor, 64)))
      return false;
    cursor += 64;
  }
  return value.substr(cursor) == tokens.back();
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

std::string escape_project_string(std::string_view value) {
  if (!valid_utf8(value))
    throw std::invalid_argument("Godot project name is not valid UTF-8");
  std::string result;
  for (const unsigned char character : value) {
    if (character < 0x20 || character == 0x7f)
      throw std::invalid_argument(
          "Godot project name contains a control character");
    if (character == '"' || character == '\\')
      result += '\\';
    result += static_cast<char>(character);
  }
  return result;
}

void write_text(const std::filesystem::path &path, std::string_view value) {
  std::ofstream output(path, std::ios::binary);
  output.write(value.data(), static_cast<std::streamsize>(value.size()));
  if (!output)
    throw std::runtime_error("failed writing Godot target artifact: " +
                             path.string());
}

void write_binary(const std::filesystem::path &path,
                  std::span<const std::byte> value) {
  std::ofstream output(path, std::ios::binary);
  output.write(reinterpret_cast<const char *>(value.data()),
               static_cast<std::streamsize>(value.size()));
  if (!output)
    throw std::runtime_error("failed writing Godot target artifact: " +
                             path.string());
}

std::string read_file(const std::filesystem::path &path,
                      std::uint64_t maximum = 512ULL * 1024ULL * 1024ULL) {
  const auto size = std::filesystem::file_size(path);
  if (size > maximum)
    throw std::runtime_error("Godot target artifact exceeds byte limit");
  std::string value(static_cast<std::size_t>(size), '\0');
  std::ifstream input(path, std::ios::binary);
  input.read(value.data(), static_cast<std::streamsize>(value.size()));
  if (!input && !value.empty())
    throw std::runtime_error("failed reading Godot target artifact");
  return value;
}

std::string
manifest(const std::map<std::string, std::string, std::less<>> &hashes) {
  std::ostringstream output;
  output << "{\"adapterId\":\"godot-4.7-3d\",\"artifacts\":[";
  std::size_t index = 0;
  for (const auto &[path, hash] : hashes) {
    if (index++ != 0)
      output << ',';
    output << "{\"path\":\"" << path << "\",\"sha256\":\"" << hash << "\"}";
  }
  output << "],\"format\":\"gspl.godot-target/0.1\"}";
  return output.str();
}

std::vector<TargetRequirement>
projection_requirements(const Projection3dDefinition &projection,
                        std::span<const AnimationClip3d> animations,
                        std::span<const TargetRequirement> additional) {
  std::map<TargetFeature, bool> merged{{TargetFeature::mesh_3d, true},
                                       {TargetFeature::engine_project, true}};
  if (!projection.materials.empty())
    merged[TargetFeature::pbr_materials_3d] = true;
  if (projection.skeleton)
    merged[TargetFeature::skeleton_3d] = true;
  if (!projection.morph_targets.empty())
    merged[TargetFeature::morph_targets_3d] = true;
  if (!animations.empty())
    merged[TargetFeature::animation_3d] = true;
  if (!projection.lods.empty())
    merged[TargetFeature::lod_3d] = true;
  for (const auto &requirement : additional)
    merged[requirement.feature] =
        merged[requirement.feature] || requirement.required;
  std::vector<TargetRequirement> result;
  for (const auto &[feature, required] : merged)
    result.push_back({feature, required});
  return result;
}
} // namespace

TargetSourceEvidence::TargetSourceEvidence(
    std::string package_identity, std::string seed_identity,
    std::string authoring_provenance_sha256,
    std::string target_compatibility_sha256)
    : package_identity_(std::move(package_identity)),
      seed_identity_(std::move(seed_identity)),
      authoring_provenance_sha256_(std::move(authoring_provenance_sha256)),
      target_compatibility_sha256_(std::move(target_compatibility_sha256)) {}

const std::string &TargetSourceEvidence::package_identity() const noexcept {
  return package_identity_;
}
const std::string &TargetSourceEvidence::seed_identity() const noexcept {
  return seed_identity_;
}
const std::string &
TargetSourceEvidence::authoring_provenance_sha256() const noexcept {
  return authoring_provenance_sha256_;
}
const std::string &
TargetSourceEvidence::target_compatibility_sha256() const noexcept {
  return target_compatibility_sha256_;
}

TargetSourceEvidence
target_source_evidence_from_package(const std::filesystem::path &package_root) {
  const auto verification = verify_package(package_root);
  if (!verification.ok())
    throw std::invalid_argument(
        "Godot source package failed verification: " +
        verification.validation.diagnostics.front().code + ": " +
        verification.validation.diagnostics.front().message);
  return {verification.package_identity, verification.seed_identity,
          sha256(read_file(package_root / "authoring-provenance.json",
                           4ULL * 1024ULL * 1024ULL)),
          sha256(read_file(package_root / "target-compatibility.json",
                           4ULL * 1024ULL * 1024ULL))};
}

void export_godot_3d_project(
    const Projection3dDefinition &projection,
    std::span<const AnimationClip3d> animations,
    std::span<const GltfTextureAsset> textures,
    std::span<const TargetRequirement> additional_requirements,
    const std::filesystem::path &output, const GodotProjectOptions &options) {
  if (output.empty() || options.project_name.empty() ||
      options.project_name.size() > 128)
    throw std::invalid_argument("Godot target path or project name is invalid");
  const auto requirements =
      projection_requirements(projection, animations, additional_requirements);
  const auto report = evaluate_target_compatibility(
      builtin_target_adapter("godot-4.7-3d"), requirements);
  if (!report.compatible())
    throw std::invalid_argument(report.validation.diagnostics.front().code +
                                ": " +
                                report.validation.diagnostics.front().message);
  if (std::filesystem::exists(output))
    throw std::runtime_error("Godot target output already exists: " +
                             output.string());
  auto staging = output;
  staging += ".staging";
  if (std::filesystem::exists(staging))
    throw std::runtime_error("Godot target staging path already exists: " +
                             staging.string());
  try {
    std::filesystem::create_directories(staging / "assets");
    const auto glb = export_projection3d_glb(projection, animations, textures,
                                             options.gltf_limits);
    const std::string project =
        "; Generated by GSPL "
        "Sprites.\nconfig_version=5\n\n[application]\n\nconfig/name=\"" +
        escape_project_string(options.project_name) +
        "\"\nrun/main_scene=\"res://"
        "gspl_sprite.tscn\"\n\n[rendering]\n\nrenderer/"
        "rendering_method=\"gl_compatibility\"\n";
    constexpr std::string_view scene =
        "[gd_scene load_steps=2 format=3]\n\n"
        "[ext_resource type=\"PackedScene\" path=\"res://assets/entity.glb\" "
        "id=\"1_entity\"]\n\n"
        "[node name=\"GSPLSprite\" instance=ExtResource(\"1_entity\")]\n";
    const auto report_json = canonicalize_target_compatibility(report);
    const auto source_evidence =
        canonical_source_evidence(options.source_evidence);
    write_binary(staging / "assets" / "entity.glb", glb);
    write_text(staging / "project.godot", project);
    write_text(staging / "gspl_sprite.tscn", scene);
    write_text(staging / "target-report.json", report_json);
    write_text(staging / "source-evidence.json", source_evidence);
    std::map<std::string, std::string, std::less<>> hashes;
    hashes.emplace("assets/entity.glb", bytes_hash(glb));
    hashes.emplace("gspl_sprite.tscn", sha256(scene));
    hashes.emplace("project.godot", sha256(project));
    hashes.emplace("source-evidence.json", sha256(source_evidence));
    hashes.emplace("target-report.json", sha256(report_json));
    write_text(staging / "gspl-target-manifest.json", manifest(hashes));
    const auto verification = verify_godot_3d_project(staging);
    if (!verification.ok())
      throw std::runtime_error(
          "generated Godot target failed verification: " +
          verification.validation.diagnostics.front().message);
    std::filesystem::rename(staging, output);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove_all(staging, ignored);
    throw;
  }
}

GodotProjectVerification
verify_godot_3d_project(const std::filesystem::path &root) {
  GodotProjectVerification result;
  auto add = [&](std::string code, std::string message) {
    result.validation.diagnostics.push_back(
        {std::move(code), std::move(message)});
  };
  try {
    if (root.empty() || !std::filesystem::is_directory(root) ||
        std::filesystem::is_symlink(std::filesystem::symlink_status(root))) {
      add("SPRITE_GODOT_ROOT_INVALID", "Godot target root is absent or unsafe");
      return result;
    }
    std::map<std::string, std::string, std::less<>> hashes;
    for (const auto path : artifacts) {
      const auto file = root / path;
      if (!std::filesystem::is_regular_file(file) ||
          std::filesystem::is_symlink(std::filesystem::symlink_status(file))) {
        add("SPRITE_GODOT_ARTIFACT_INVALID",
            "Godot target artifact is absent or unsafe: " + std::string(path));
        continue;
      }
      hashes.emplace(path, sha256(read_file(file)));
    }
    const auto manifest_path = root / "gspl-target-manifest.json";
    if (!std::filesystem::is_regular_file(manifest_path) ||
        std::filesystem::is_symlink(
            std::filesystem::symlink_status(manifest_path))) {
      add("SPRITE_GODOT_MANIFEST_INVALID",
          "Godot target manifest is absent or unsafe");
      return result;
    }
    const auto expected = manifest(hashes);
    const auto actual = read_file(manifest_path, 1024ULL * 1024ULL);
    if (actual != expected)
      add("SPRITE_GODOT_MANIFEST_MISMATCH",
          "Godot target manifest does not match its artifacts");
    result.project_identity = sha256(actual);
    if (hashes.contains("source-evidence.json") &&
        !valid_source_evidence(
            read_file(root / "source-evidence.json", 1024ULL * 1024ULL)))
      add("SPRITE_GODOT_SOURCE_EVIDENCE_INVALID",
          "Godot source package evidence is malformed");
    std::set<std::string> expected_paths{"gspl-target-manifest.json"};
    expected_paths.insert(artifacts.begin(), artifacts.end());
    std::set<std::string> actual_paths;
    std::uint32_t entries = 0;
    for (const auto &entry :
         std::filesystem::recursive_directory_iterator(root)) {
      if (++entries > 32) {
        add("SPRITE_GODOT_ENTRY_LIMIT",
            "Godot target contains too many directory entries");
        break;
      }
      if (std::filesystem::is_symlink(entry.symlink_status())) {
        add("SPRITE_GODOT_SYMLINK", "Godot target contains a symlink");
        continue;
      }
      if (entry.is_regular_file())
        actual_paths.insert(
            entry.path().lexically_relative(root).generic_string());
    }
    if (actual_paths != expected_paths)
      add("SPRITE_GODOT_FILE_SET_MISMATCH",
          "Godot target contains undeclared files or missing artifacts");
  } catch (const std::exception &error) {
    add("SPRITE_GODOT_TARGET_MALFORMED", error.what());
  }
  return result;
}

} // namespace gspl::sprites
