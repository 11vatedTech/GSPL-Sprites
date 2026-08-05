#pragma once

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/sprite2d.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites {

/* LIVING PACKAGE VIEWER (headless model)
 *
 * The first real visual instrument for GSPL artifacts. It consumes ONLY a
 * verified Living Visual Package (verify_living_visual_package then
 * read_living_visual_package) and displays what the package actually
 * contains: generated frames, clips, timing, events, channels, atlas
 * placements, morphologies, collisions, and provenance. It contains no
 * Voltfox-specific runtime logic and rebuilds nothing.
 *
 * Pure C++23: no SDL, no Qt, no rendering. The display layer (SDL preview)
 * is a thin consumer. All playback scheduling derives from package
 * animation data; no hardcoded durations. Fail-closed: a package rejected
 * by the verifier is never exposed as trusted.
 */

enum class ViewerPlaybackState { stopped, playing, paused };

/* Generic clip group derived from the required-clip table semantics
 * (kRequiredClips): base_* / storm_* / transform_*. Not Voltfox-specific. */
enum class ViewerFormGroup { base, transformation, storm, other };

struct ViewerClipInfo {
  std::string clip_id;
  ViewerFormGroup group{ViewerFormGroup::other};
  std::uint32_t duration_ticks{};
  std::uint32_t frame_count{};
  bool looping{};
  std::vector<std::string> frame_ids;            // exact ordered frame ids
  std::vector<std::uint32_t> frame_start_ticks;  // cumulative timing (package data)
  std::vector<GeneratedAnimationEvent> events;   // per-clip mapped events
};

struct ViewerChannelInfo {
  std::string id;
  std::string target_frame_id;
  ChannelMapKind kind{ChannelMapKind::effects};
  std::uint32_t width{};
  std::uint32_t height{};
  ColorSpace color_space{ColorSpace::unknown};
  AlphaMode alpha_mode{AlphaMode::straight};
};

struct ViewerAtlasPlacement {
  std::string frame_id;
  std::uint32_t x{};
  std::uint32_t y{};
  std::uint32_t width{};
  std::uint32_t height{};
  std::int32_t pivot_x{};
  std::int32_t pivot_y{};
  std::uint32_t duration_ticks{};
};

struct ViewerFrame {
  const FrameSource* frame{nullptr};
  std::string frame_id;
  std::uint32_t frame_index{};
  std::uint32_t source_tick{};
};

struct ViewerProvenance {
  std::string entity_id;
  std::string seed_identity;
  std::string package_identity;
  std::string canonical_entity_identity;
  std::string manifest_format;
  std::string identity_version;
  RightsClass rights{RightsClass::unknown};
  std::uint32_t artifact_count{};
  std::uint64_t total_artifact_bytes{};
  std::uint32_t frame_count{};
  std::uint32_t clip_count{};
  std::uint32_t sample_count{};
  std::uint32_t event_count{};
  std::uint32_t channel_count{};
  std::uint32_t collision_shape_count{};
  std::uint32_t collision_window_count{};
  std::uint32_t transformation_morphology_count{};
};

class LivingPackageViewer {
public:
  struct LoadResult;

  /* Verify first (fail-closed), then read. Any verifier or reader failure
   * is surfaced as diagnostics; no partial/unverified content is exposed. */
  [[nodiscard]] static LoadResult
  load(const std::filesystem::path& package_path,
       const PackageReadLimits& limits = {});

  /* identity / provenance */
  [[nodiscard]] const ViewerProvenance& provenance() const noexcept { return provenance_; }
  [[nodiscard]] std::string_view entity_id() const noexcept { return provenance_.entity_id; }
  [[nodiscard]] std::string_view seed_identity() const noexcept { return provenance_.seed_identity; }
  [[nodiscard]] std::string_view package_identity() const noexcept { return provenance_.package_identity; }

  /* clips / frames */
  [[nodiscard]] std::span<const ViewerClipInfo> clips() const noexcept { return clips_; }
  [[nodiscard]] const ViewerClipInfo* clip(std::string_view clip_id) const noexcept;
  [[nodiscard]] std::size_t clip_count() const noexcept { return clips_.size(); }
  [[nodiscard]] std::size_t frame_count() const noexcept { return package_.frames.size(); }
  [[nodiscard]] std::span<const FrameSource> frames() const noexcept { return package_.frames; }
  [[nodiscard]] const FrameSource* frame_by_id(std::string_view frame_id) const noexcept;

  /* Deterministic package-data tick -> frame mapping for a clip.
   * Uses the retained sample table; wraps by clip duration when looping. */
  [[nodiscard]] ViewerFrame frame_for_tick(std::string_view clip_id,
                                           std::uint32_t tick) const noexcept;

  /* channels */
  [[nodiscard]] std::span<const ViewerChannelInfo> channel_info() const noexcept { return channel_info_; }
  [[nodiscard]] std::span<const ChannelMap> channels() const noexcept { return package_.channels; }
  [[nodiscard]] const ChannelMap* channel(std::string_view id) const noexcept;

  /* atlas */
  [[nodiscard]] std::span<const ViewerAtlasPlacement> atlas_placements() const noexcept { return atlas_placements_; }
  [[nodiscard]] const ImageRgba8* atlas_image() const noexcept;

  /* morphology inspector */
  [[nodiscard]] const EffectiveMorphology& base_morphology() const noexcept { return package_.base_morphology; }
  [[nodiscard]] const EffectiveMorphology& storm_morphology() const noexcept { return package_.storm_morphology; }
  [[nodiscard]] std::span<const EffectiveMorphology> transformation_morphologies() const noexcept {
    return package_.transformation_morphologies;
  }
  [[nodiscard]] std::size_t transformation_morphology_count() const noexcept {
    return package_.transformation_morphologies.size();
  }

  /* collision / gameplay overlays */
  [[nodiscard]] std::span<const CollisionShape> collision_shapes() const noexcept { return package_.collision_shapes; }
  [[nodiscard]] std::span<const CollisionWindow> collision_windows() const noexcept { return package_.collision_windows; }
  [[nodiscard]] std::vector<CollisionWindow>
  active_collision_windows(std::string_view clip_id, std::uint32_t tick) const;

  /* playback (package-data driven; deterministic) */
  [[nodiscard]] bool select_clip(std::string_view clip_id) noexcept;
  void play() noexcept { state_ = ViewerPlaybackState::playing; }
  void pause() noexcept { state_ = ViewerPlaybackState::paused; }
  void stop() noexcept { state_ = ViewerPlaybackState::stopped; current_tick_ = 0; }
  void toggle_play() noexcept {
    state_ = state_ == ViewerPlaybackState::playing ? ViewerPlaybackState::paused
                                                    : ViewerPlaybackState::playing;
  }
  [[nodiscard]] bool step_frame(int delta) noexcept;
  void seek_tick(std::uint32_t tick) noexcept { current_tick_ = tick; }
  void set_speed(double speed) noexcept { speed_ = speed > 0.0 ? speed : 1.0; }
  void set_looping(bool looping) noexcept { looping_ = looping; }
  [[nodiscard]] bool looping() const noexcept { return looping_; }
  [[nodiscard]] double speed() const noexcept { return speed_; }

  [[nodiscard]] std::string_view current_clip_id() const noexcept;
  [[nodiscard]] std::uint32_t current_tick() const noexcept { return current_tick_; }
  [[nodiscard]] ViewerPlaybackState playback_state() const noexcept { return state_; }
  [[nodiscard]] ViewerFrame current_frame() const noexcept;
  [[nodiscard]] std::string_view current_frame_id() const noexcept;
  [[nodiscard]] const FrameSource* current_frame_source() const noexcept;

  /* frame comparison (pure pixel ops; temporal metrics reuse the existing
     analyze_temporal_stability utility, never reimplemented) */
  [[nodiscard]] static ImageRgba8 blend_frames(const ImageRgba8& a,
                                               const ImageRgba8& b,
                                               double alpha);
  [[nodiscard]] static ImageRgba8 difference_mask(const ImageRgba8& a,
                                                  const ImageRgba8& b);
  [[nodiscard]] std::vector<TemporalTransitionMetrics>
  temporal_metrics(std::string_view clip_id) const;

private:
  explicit LivingPackageViewer(LoadedLivingVisualPackage package);

  LoadedLivingVisualPackage package_;
  std::vector<ViewerClipInfo> clips_;
  std::vector<ViewerChannelInfo> channel_info_;
  std::vector<ViewerAtlasPlacement> atlas_placements_;
  ViewerProvenance provenance_;
  std::size_t current_clip_index_{0};
  std::uint32_t current_tick_{0};
  ViewerPlaybackState state_{ViewerPlaybackState::stopped};
  double speed_{1.0};
  bool looping_{false};
};

struct LivingPackageViewer::LoadResult {
  std::optional<LivingPackageViewer> value;
  ValidationResult diagnostics;
  [[nodiscard]] bool ok() const noexcept { return value.has_value() && diagnostics.ok(); }
};

} // namespace gspl::sprites
