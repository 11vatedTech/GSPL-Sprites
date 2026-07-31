#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/synthesis.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>

namespace gspl::sprites {

struct PackageLimits {
  std::uint64_t max_manifest_bytes{4ULL * 1024ULL * 1024ULL};
  std::uint64_t max_artifact_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_bytes{2ULL * 1024ULL * 1024ULL * 1024ULL};
  std::uint32_t max_artifacts{4096};
  std::uint32_t max_directory_entries{8192};
  std::uint32_t max_path_bytes{1024};
};

struct PackageVerification {
  ValidationResult validation;
  std::string entity_id;
  std::string seed_identity;
  std::string package_identity;
  std::uint32_t artifact_count{};
  std::uint64_t total_artifact_bytes{};
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

[[nodiscard]] PackageVerification verify_package(const std::filesystem::path& root,
                                                 const PackageLimits& limits = {});

/* ── Living Visual Package ── */

/* ── Semantic artifact kinds ── */
enum class LivingArtifactKind : std::uint8_t {
  living_seed,
  seed_identity,
  frame_metadata,
  frame_image,
  generated_animation,
  frame_samples,
  generated_events,
  pose_hashes,
  frame_hashes,
  channel_metadata,
  channel_image,
  collision_metadata,
  effective_morphology,
  transformation_morphologies,
  sprite_atlas,
  sprite_atlas_metadata,
};

[[nodiscard]] std::string_view artifact_kind_string(LivingArtifactKind kind) noexcept;
[[nodiscard]] std::string_view artifact_schema_for_kind(LivingArtifactKind kind) noexcept;
[[nodiscard]] std::optional<LivingArtifactKind> artifact_kind_from_string(std::string_view s) noexcept;

/* ── Typed manifest artifact record ── */
struct LivingPackageArtifactRecord {
  std::string path;
  LivingArtifactKind kind{LivingArtifactKind::frame_image};
  std::string schema;
  std::uint64_t byte_size{};
  std::string sha256;
  std::vector<std::string> dependencies;
  std::string provenance_identity;
};

/* ── Typed canonical living-package manifest ── */
struct LivingPackageManifest {
  std::string format;
  std::string identity_version;
  std::string package_identity;
  std::string entity_id;
  std::string canonical_entity_identity;
  std::string seed_identity;

  std::uint32_t frame_count{};
  std::uint32_t clip_count{};
  std::uint32_t sample_count{};
  std::uint32_t event_count{};
  std::uint32_t channel_count{};
  std::uint32_t collision_shape_count{};
  std::uint32_t collision_window_count{};
  std::uint32_t transformation_morphology_count{};
  std::uint32_t artifact_count{};

  std::vector<LivingPackageArtifactRecord> artifacts;
};

/* ── Canonical manifest serialization ── */
[[nodiscard]] std::string canonicalize_manifest(
    const LivingPackageManifest& manifest,
    bool include_package_identity);

/* ── Artifact schemas ── */
inline constexpr std::string_view kSchemaLivingSeed               = "gspl.living-seed/0.1";
inline constexpr std::string_view kSchemaSourceSkeletalAnimations = "gspl.source-skeletal-animations/0.1";
inline constexpr std::string_view kSchemaGeneratedAnimation2d     = "gspl.generated-animation-2d/0.1";
inline constexpr std::string_view kSchemaGeneratedAnimationEvents = "gspl.generated-animation-events/0.1";
inline constexpr std::string_view kSchemaFrameSamples             = "gspl.frame-samples/0.1";
inline constexpr std::string_view kSchemaPoseHashes               = "gspl.pose-hashes/0.1";
inline constexpr std::string_view kSchemaFrameHashes              = "gspl.frame-hashes/0.1";
inline constexpr std::string_view kSchemaChannelMaps              = "gspl.channel-maps/0.1";
inline constexpr std::string_view kSchemaCollisions2d             = "gspl.collisions-2d/0.1";
inline constexpr std::string_view kSchemaFrames2d                   = "gspl.frames-2d/0.1";
inline constexpr std::string_view kSchemaEffectiveMorphology      = "gspl.effective-morphology/0.1";
inline constexpr std::string_view kSchemaTransformationMorphologies = "gspl.transformation-morphologies/0.1";
inline constexpr std::string_view kSchemaSpriteSheet              = "gspl.sprite-sheet/0.1";
inline constexpr std::string_view kSchemaLivingVisualPackage      = "gspl.living-visual-package/0.1";
inline constexpr std::string_view kIdentityPreimageVersion  = "gspl.living-visual-package.identity/0.1";

struct PackageReadLimits {
  std::uint64_t max_manifest_bytes{4ULL * 1024ULL * 1024ULL};
  std::uint64_t max_artifact_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_bytes{2ULL * 1024ULL * 1024ULL * 1024ULL};
  std::uint32_t max_artifacts{4096};
  std::uint32_t max_directory_entries{8192};
  std::uint32_t max_path_bytes{1024};
  std::uint32_t max_frames{256};
  std::uint32_t max_channels{512};
  std::uint32_t max_morphology_parts{128};
  std::uint32_t max_json_tokens{262144};
  std::uint32_t max_json_nesting{32};
  std::uint32_t max_image_width{4096};
  std::uint32_t max_image_height{4096};
  std::uint64_t max_decoded_pixel_bytes{256ULL * 1024ULL * 1024ULL};
};

/* ── Fully reconstructed loaded package ── */
struct LoadedLivingVisualPackage {
  std::string schema;
  std::string entity_id;
  std::string canonical_entity_identity;
  std::string seed_identity;
  std::string package_identity;

  LivingPackageManifest manifest;
  SpriteSeed seed;

  std::vector<FrameSource> frames;
  std::vector<AnimationClip> generated_clips;
  std::vector<GeneratedFrameSample> samples;
  std::vector<GeneratedAnimationEvent> events;
  std::vector<ChannelMap> channels;

  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;

  EffectiveMorphology base_morphology;
  EffectiveMorphology storm_morphology;
  std::vector<EffectiveMorphology> transformation_morphologies;

  SpriteSheetArtifacts sheet;
};

struct LivingVisualPackageReadResult {
  std::optional<LoadedLivingVisualPackage> value;
  ValidationResult diagnostics;
  [[nodiscard]] bool ok() const { return value.has_value() && diagnostics.ok(); }
};

struct PackageVerificationOptions {
  bool verify_pixel_hashes{true};
  bool verify_channel_dimensions{true};
  bool verify_morphologies{true};
  bool strict_collision_refs{true};
  bool require_no_symlinks{true};
  bool require_no_undeclared_files{true};
};

struct LivingVisualPackageVerificationResult {
  ValidationResult validation;
  std::string package_identity;
  std::string seed_identity;
  std::uint32_t frame_count{};
  std::uint32_t clip_count{};
  std::uint32_t sample_count{};
  std::uint32_t event_count{};
  std::uint32_t channel_count{};
  std::uint32_t collision_shape_count{};
  std::uint32_t collision_window_count{};
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

/* ── Safe path encoding for artifact IDs ── */
[[nodiscard]] std::string encode_package_component(std::string_view semantic_id);
[[nodiscard]] std::string decode_package_component(std::string_view encoded);

/* ── Package builder ── */
void build_living_visual_package(const LivingVisualPackageInput& input,
                                 const std::filesystem::path& output);

/* ── Process-independent reader ── */
[[nodiscard]] LivingVisualPackageReadResult read_living_visual_package(
    const std::filesystem::path& package_path,
    const PackageReadLimits& limits = {});

/* ── Independent verifier ── */
[[nodiscard]] LivingVisualPackageVerificationResult verify_living_visual_package(
    const std::filesystem::path& package_path,
    const PackageVerificationOptions& options = {});

} // namespace gspl::sprites
