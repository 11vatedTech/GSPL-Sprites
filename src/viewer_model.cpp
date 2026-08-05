#include "gspl_sprites/viewer_model.hpp"
#include "gspl_sprites/animation.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace gspl::sprites {
namespace {

[[nodiscard]] ViewerFormGroup classify_clip_group(std::string_view clip_id) {
  if (clip_id.starts_with("base_"))    return ViewerFormGroup::base;
  if (clip_id.starts_with("storm_"))   return ViewerFormGroup::storm;
  if (clip_id.starts_with("transform_")) return ViewerFormGroup::transformation;
  return ViewerFormGroup::other;
}

} // namespace

/* ── Load ── */

LivingPackageViewer::LoadResult
LivingPackageViewer::load(const std::filesystem::path& package_path,
                          const PackageReadLimits& limits) {
  // 1. Verify (fail-closed — reject before reading)
  PackageVerificationOptions options;
  auto verify_result = verify_living_visual_package(package_path, options);
  if (!verify_result.validation.ok()) {
    LoadResult result;
    result.diagnostics = std::move(verify_result.validation);
    return result;
  }

  // 2. Read
  auto read_result = read_living_visual_package(package_path, limits);
  if (!read_result.ok()) {
    LoadResult result;
    result.diagnostics = std::move(read_result.diagnostics);
    return result;
  }

  auto viewer = LivingPackageViewer(std::move(*read_result.value));

  // 3. Build ViewerProvenance
  auto& pkg = viewer.package_;
  viewer.provenance_.entity_id = pkg.entity_id;
  viewer.provenance_.seed_identity = pkg.seed_identity;
  viewer.provenance_.package_identity = pkg.package_identity;
  viewer.provenance_.canonical_entity_identity = pkg.canonical_entity_identity;
  viewer.provenance_.manifest_format = pkg.schema;
  viewer.provenance_.rights = pkg.seed.rights;
  viewer.provenance_.artifact_count =
      static_cast<std::uint32_t>(pkg.manifest.artifacts.size());
  {
    std::uint64_t total = 0;
    for (const auto& a : pkg.manifest.artifacts)
      total += a.byte_size;
    viewer.provenance_.total_artifact_bytes = total;
  }
  viewer.provenance_.frame_count =
      static_cast<std::uint32_t>(pkg.frames.size());
  viewer.provenance_.clip_count =
      static_cast<std::uint32_t>(pkg.generated_clips.size());
  viewer.provenance_.sample_count =
      static_cast<std::uint32_t>(pkg.samples.size());
  viewer.provenance_.event_count =
      static_cast<std::uint32_t>(pkg.events.size());
  viewer.provenance_.channel_count =
      static_cast<std::uint32_t>(pkg.channels.size());
  viewer.provenance_.collision_shape_count =
      static_cast<std::uint32_t>(pkg.collision_shapes.size());
  viewer.provenance_.collision_window_count =
      static_cast<std::uint32_t>(pkg.collision_windows.size());
  viewer.provenance_.transformation_morphology_count =
      static_cast<std::uint32_t>(pkg.transformation_morphologies.size());

  // 4. Build clip info from generated_clips + samples + events
  for (const auto& clip : pkg.generated_clips) {
    ViewerClipInfo info;
    info.clip_id       = clip.id;
    info.group         = classify_clip_group(clip.id);
    info.looping       = clip.looping;
    info.frame_ids     = clip.frame_ids;
    info.frame_count   = static_cast<std::uint32_t>(clip.frame_ids.size());

    // Duration: sum frame_durations or use last event tick as max bound
    std::uint32_t total_duration = 0;
    for (auto d : clip.frame_durations) total_duration += d;
    info.duration_ticks = total_duration;

    // Build cumulative frame start ticks
    std::uint32_t cumulative = 0;
    for (auto d : clip.frame_durations) {
      info.frame_start_ticks.push_back(cumulative);
      cumulative += d;
    }

    // Collect per-clip events from package_.events
    for (const auto& event : pkg.events) {
      if (event.clip_id == clip.id)
        info.events.push_back(event);
    }

    viewer.clips_.push_back(std::move(info));
  }

  // 5. Build channel info
  for (const auto& ch : pkg.channels) {
    ViewerChannelInfo ci;
    ci.id             = ch.id;
    ci.target_frame_id = ch.target_frame_id;
    ci.kind           = ch.kind;
    ci.width          = ch.image.width;
    ci.height         = ch.image.height;
    ci.color_space    = ch.image.color_space;
    ci.alpha_mode     = ch.image.alpha_mode;
    viewer.channel_info_.push_back(std::move(ci));
  }

  // 6. Build atlas placements
  if (!pkg.sheet.atlas.placements.empty()) {
    for (const auto& placement : pkg.sheet.atlas.placements) {
      ViewerAtlasPlacement ap;
      ap.frame_id = placement.frame_id;
      ap.x        = placement.x;
      ap.y        = placement.y;
      ap.width    = placement.width;
      ap.height   = placement.height;
      ap.pivot_x  = placement.pivot_x;
      ap.pivot_y  = placement.pivot_y;
      ap.duration_ticks = placement.duration_ticks;
      viewer.atlas_placements_.push_back(std::move(ap));
    }
  }

  LoadResult result;
  result.value = std::move(viewer);
  return result;
}

LivingPackageViewer::LivingPackageViewer(LoadedLivingVisualPackage package)
    : package_(std::move(package)) {}

/* ── Clip / Frame queries ── */

const ViewerClipInfo* LivingPackageViewer::clip(std::string_view clip_id) const noexcept {
  for (const auto& c : clips_)
    if (c.clip_id == clip_id) return &c;
  return nullptr;
}

const FrameSource* LivingPackageViewer::frame_by_id(std::string_view frame_id) const noexcept {
  for (const auto& f : package_.frames)
    if (f.id == frame_id) return &f;
  return nullptr;
}

const ChannelMap* LivingPackageViewer::channel(std::string_view id) const noexcept {
  for (const auto& ch : package_.channels)
    if (ch.id == id) return &ch;
  return nullptr;
}

const ImageRgba8* LivingPackageViewer::atlas_image() const noexcept {
  if (package_.sheet.atlas.image.pixels.empty()) return nullptr;
  return &package_.sheet.atlas.image;
}

/* ── Deterministic tick → frame mapping ── */

ViewerFrame LivingPackageViewer::frame_for_tick(
    std::string_view clip_id, std::uint32_t tick) const noexcept {
  ViewerFrame result;
  const auto* ci = clip(clip_id);
  if (!ci || ci->frame_ids.empty()) return result;

  // Wrap tick into clip duration (looping behavior)
  std::uint32_t duration = ci->duration_ticks;
  std::uint32_t wrapped_tick = tick;
  if (duration > 0) wrapped_tick = tick % duration;

  // Binary search frame_start_ticks to find the frame containing wrapped_tick
  auto it = std::upper_bound(ci->frame_start_ticks.begin(),
                             ci->frame_start_ticks.end(), wrapped_tick);
  if (it == ci->frame_start_ticks.begin()) {
    result.frame_index = 0;
  } else {
    result.frame_index = static_cast<std::uint32_t>(
        std::distance(ci->frame_start_ticks.begin(), it) - 1);
  }

  // Clamp to valid range
  if (result.frame_index >= ci->frame_ids.size())
    result.frame_index = static_cast<std::uint32_t>(ci->frame_ids.size() - 1);

  result.frame_id  = ci->frame_ids[result.frame_index];
  result.frame     = frame_by_id(result.frame_id);

  // Map back to source_tick via samples
  for (const auto& s : package_.samples) {
    if (s.clip_id == clip_id && s.frame_id == result.frame_id) {
      result.source_tick = s.source_tick;
      break;
    }
  }

  return result;
}

/* ── Playback ── */

bool LivingPackageViewer::select_clip(std::string_view clip_id) noexcept {
  for (std::size_t i = 0; i < clips_.size(); ++i) {
    if (clips_[i].clip_id == clip_id) {
      current_clip_index_ = i;
      current_tick_ = 0;
      state_ = ViewerPlaybackState::paused;
      return true;
    }
  }
  return false;
}

bool LivingPackageViewer::step_frame(int delta) noexcept {
  if (clips_.empty()) return false;
  const auto& ci = clips_[current_clip_index_];
  if (ci.frame_ids.empty()) return false;

  int idx = -1;
  for (std::size_t i = 0; i < ci.frame_ids.size(); ++i) {
    if (ci.frame_ids[i] == current_frame_id()) {
      idx = static_cast<int>(i);
      break;
    }
  }

  int new_idx = idx + delta;
  if (looping_) {
    int n = static_cast<int>(ci.frame_ids.size());
    new_idx = ((new_idx % n) + n) % n;
  } else {
    new_idx = std::max(0, std::min(new_idx, static_cast<int>(ci.frame_ids.size()) - 1));
  }

  current_tick_ = ci.frame_start_ticks[static_cast<std::size_t>(new_idx)];
  return true;
}

std::string_view LivingPackageViewer::current_clip_id() const noexcept {
  if (clips_.empty()) return {};
  return clips_[current_clip_index_].clip_id;
}

ViewerFrame LivingPackageViewer::current_frame() const noexcept {
  return frame_for_tick(current_clip_id(), current_tick_);
}

std::string_view LivingPackageViewer::current_frame_id() const noexcept {
  auto f = current_frame();
  return f.frame_id.empty() ? std::string_view{} : f.frame->id;
}

const FrameSource* LivingPackageViewer::current_frame_source() const noexcept {
  return current_frame().frame;
}

/* ── Collision overlays ── */

std::vector<CollisionWindow>
LivingPackageViewer::active_collision_windows(
    std::string_view clip_id, std::uint32_t tick) const {
  (void)clip_id;
  std::vector<CollisionWindow> active;
  for (const auto& cw : package_.collision_windows) {
    if (tick >= cw.start_tick && tick < cw.end_tick)
      active.push_back(cw);
  }
  return active;
}

/* ── Frame comparison (pixel ops) ── */

ImageRgba8 LivingPackageViewer::blend_frames(const ImageRgba8& a,
                                             const ImageRgba8& b, double alpha) {
  if (a.width != b.width || a.height != b.height || a.pixels.size() != b.pixels.size())
    return a; // mismatch: return "a" unchanged, caller validates

  ImageRgba8 result;
  result.width  = a.width;
  result.height = a.height;
  result.pixels.resize(a.pixels.size());

  double a_weight = 1.0 - alpha;
  for (std::size_t i = 0; i < a.pixels.size(); i += 4) {
    double a_r = static_cast<double>(a.pixels[i]);
    double a_g = static_cast<double>(a.pixels[i + 1]);
    double a_b = static_cast<double>(a.pixels[i + 2]);
    double a_a = static_cast<double>(a.pixels[i + 3]);
    double b_r = static_cast<double>(b.pixels[i]);
    double b_g = static_cast<double>(b.pixels[i + 1]);
    double b_b = static_cast<double>(b.pixels[i + 2]);
    double b_a = static_cast<double>(b.pixels[i + 3]);

    result.pixels[i]     = static_cast<std::uint8_t>(a_weight * a_r + alpha * b_r);
    result.pixels[i + 1] = static_cast<std::uint8_t>(a_weight * a_g + alpha * b_g);
    result.pixels[i + 2] = static_cast<std::uint8_t>(a_weight * a_b + alpha * b_b);
    result.pixels[i + 3] = static_cast<std::uint8_t>(a_weight * a_a + alpha * b_a);
  }

  return result;
}

ImageRgba8 LivingPackageViewer::difference_mask(const ImageRgba8& a,
                                                const ImageRgba8& b) {
  std::uint32_t w = std::min(a.width, b.width);
  std::uint32_t h = std::min(a.height, b.height);

  ImageRgba8 result;
  result.width  = w;
  result.height = h;
  result.pixels.resize(static_cast<std::size_t>(w) * h * 4);

  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      std::size_t ai = (static_cast<std::size_t>(y) * a.width + x) * 4;
      std::size_t bi = (static_cast<std::size_t>(y) * b.width + x) * 4;
      std::size_t ri = (static_cast<std::size_t>(y) * w + x) * 4;

      bool diff = (a.pixels[ai] != b.pixels[bi]) ||
                  (a.pixels[ai + 1] != b.pixels[bi + 1]) ||
                  (a.pixels[ai + 2] != b.pixels[bi + 2]) ||
                  (a.pixels[ai + 3] != b.pixels[bi + 3]);

      std::uint8_t val = diff ? 255 : 0;
      result.pixels[ri]     = val;
      result.pixels[ri + 1] = val;
      result.pixels[ri + 2] = val;
      result.pixels[ri + 3] = 255;
    }
  }

  return result;
}

/* ── Temporal metrics (reuses analyze_temporal_stability) ── */

std::vector<TemporalTransitionMetrics>
LivingPackageViewer::temporal_metrics(std::string_view clip_id) const {
  const auto* ci = clip(clip_id);
  if (!ci || ci->frame_ids.empty()) return {};

  std::vector<FrameSource> ordered_frames;
  for (const auto& fid : ci->frame_ids) {
    const auto* fs = frame_by_id(fid);
    if (fs) ordered_frames.push_back(*fs);
  }

  if (ordered_frames.empty()) return {};
  return analyze_temporal_stability(ordered_frames);
}

} // namespace gspl::sprites
