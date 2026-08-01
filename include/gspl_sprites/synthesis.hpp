#pragma once

#include <cstdint>

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/projection2d.hpp"
#include "gspl_sprites/projection25d.hpp"
#include "gspl_sprites/projection3d.hpp"
#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/animation3d.hpp"
#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"
#include <string>
#include <vector>

namespace gspl::sprites {

struct SynthesisPalette {
  std::uint32_t primary;
  std::uint32_t secondary;
  std::uint32_t accent;
  std::uint32_t outline;
  std::uint32_t background;
};

[[nodiscard]] SynthesisPalette make_palette(std::string_view primary_hex, std::string_view accent_hex);
[[nodiscard]] RigDefinition make_biped_rig(std::string_view id);

[[nodiscard]] Projection2dDefinition synthesize_projection2d(std::string_view entity_id, std::string_view form_id,
                                                             const SynthesisPalette& palette,
                                                             const RigDefinition& rig);

[[nodiscard]] Projection25dDefinition synthesize_projection25d(std::string_view entity_id, std::string_view form_id,
                                                                const SynthesisPalette& palette,
                                                                const RigDefinition& rig);

[[nodiscard]] Projection3dDefinition synthesize_projection3d(std::string_view entity_id, std::string_view form_id,
                                                               const SynthesisPalette& palette,
                                                               const RigDefinition& rig);

[[nodiscard]] Projection3dDefinition synthesize_projection3d_voltfox(std::string_view entity_id, std::string_view form_id,
                                                                        const SynthesisPalette& palette,
                                                                        const std::map<std::string, MorphologyPart, std::less<>>& morphology);

[[nodiscard]] Projection25dDefinition synthesize_projection25d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology);

[[nodiscard]] Projection2dDefinition synthesize_projection2d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const RigDefinition& rig,
    std::span<const SkeletalClip> clips = {});

[[nodiscard]] Projection2dDefinition synthesize_morphology_projection2d(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const RigDefinition& rig,
    std::span<const SkeletalClip> clips = {});

[[nodiscard]] std::vector<AnimationClip3d> synthesize_animation3d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const std::vector<SkeletalClip>& clips,
    std::span<const AnimationIntent> animation_intents);

[[nodiscard]] TransformationManifestationProgram make_manifestation2d(std::string_view entity_id,
                                                                       const RigDefinition& rig);
[[nodiscard]] TransformationManifestationProgram make_manifestation25d(std::string_view entity_id,
                                                                        const RigDefinition& rig);
[[nodiscard]] TransformationManifestationProgram make_manifestation3d(std::string_view entity_id,
                                                                       const RigDefinition& rig);

struct SynthesisResult {
  Projection2dDefinition proj2d_base;
  Projection2dDefinition proj2d_transformed;
  Projection25dDefinition proj25d_base;
  Projection25dDefinition proj25d_transformed;
  Projection3dDefinition proj3d_base;
  Projection3dDefinition proj3d_transformed;
  std::vector<AnimationClip3d> animations3d;
  TransformationManifestationProgram manifest2d;
  TransformationManifestationProgram manifest25d;
  TransformationManifestationProgram manifest3d;
};

[[nodiscard]] SynthesisResult synthesize_unified_entity(std::string_view entity_id,
                                                        const SynthesisPalette& base_palette,
                                                        const SynthesisPalette& transformed_palette);

[[nodiscard]] SynthesisResult synthesize_unified_entity(const SpriteIr& ir);

[[nodiscard]] ValidationResult enforce_resource_limits(const SpriteSeed& seed,
                                                         const SynthesisResult& result,
                                                         const ResourceLimits& limits = {});

/* ── Required clip table for living animation ── */
struct RequiredLivingClip {
  std::string_view semantic_role;
  std::string_view exact_id;
  std::uint32_t output_frame_count;
  bool is_looping;
};

inline constexpr RequiredLivingClip kRequiredClips[] = {
  {"base_idle",        "base_idle",         4, true},
  {"base_locomotion",  "base_locomotion",   6, true},
  {"base_attack",      "base_attack",       6, false},
  {"base_hit",         "base_hit",          3, false},
  {"transformation",   "transform_ascend", 10, false},
  {"storm_idle",       "storm_idle",        4, true},
  {"storm_locomotion", "storm_locomotion",  6, true},
  {"storm_attack",     "storm_attack",       6, false},
  {"storm_hit",        "storm_hit",          3, false},
};

/* ── Frame sample schedule for event mapping ── */
struct GeneratedFrameSample {
  std::string clip_id;
  std::string frame_id;
  std::uint32_t frame_index;
  std::uint32_t source_tick;
  std::string pose_hash;
  std::string frame_hash;
};

struct GeneratedAnimationEvent {
  std::string clip_id;
  std::string event_id;
  std::uint32_t authored_tick{};
  std::uint32_t frame_index{};
  std::string frame_id;
  std::uint32_t mapped_source_tick{};
};

/* ── Living Animation 2D: entity-level synthesis producing exactly
     48 frames (19 base + 10 transform + 19 storm) ── */
using EffectiveMorphology = std::map<std::string, MorphologyPart, std::less<>>;

struct LivingAnimation2d {
  std::vector<FrameSource> base_frames;
  std::vector<FrameSource> transformation_frames;
  std::vector<FrameSource> storm_frames;
  std::vector<FrameSource> all_frames;
  std::vector<AnimationClip> clips;
  std::vector<GeneratedFrameSample> samples;
  std::vector<GeneratedAnimationEvent> generated_events;
  std::vector<ChannelMap> channel_maps;
  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;
  SpriteSheetArtifacts sheet;
  EffectiveMorphology base_morphology;
  EffectiveMorphology storm_morphology;
  std::vector<EffectiveMorphology> transformation_morphologies;
};

struct LivingAnimation2dBuildResult {
  std::optional<LivingAnimation2d> value;
  ValidationResult diagnostics;
  [[nodiscard]] bool ok() const { return value.has_value() && diagnostics.ok(); }
};

[[nodiscard]] std::string canonicalize_pose(const EvaluatedPose& pose);

[[nodiscard]] LivingAnimation2dBuildResult synthesize_living_animation2d(const SpriteSeed& seed);

/* ── Living Visual Package Input ── */
struct LivingVisualPackageInput {
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

[[nodiscard]] std::uint32_t with_alpha(std::uint32_t rgba, std::uint8_t alpha);

/* ── Source-over blending for RGBA (0xRRGGBBAA packed format) ── */
void blend_source_over(std::uint8_t* dest, std::uint32_t src_rgba);

} // namespace gspl::sprites