#include "gspl_sprites/projection2d.hpp"

#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
bool id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char c) {
           return std::isalnum(c) != 0 || c == '.' || c == '_' || c == '-';
         });
}
std::string image_identity(const ImageRgba8 &image) {
  std::string bytes;
  bytes.reserve(32 + image.pixels.size());
  bytes.append(std::to_string(image.width)).push_back('|');
  bytes.append(std::to_string(image.height)).push_back('|');
  bytes.append(std::to_string(static_cast<int>(image.color_space))).push_back('|');
  bytes.append(std::to_string(static_cast<int>(image.alpha_mode))).push_back('|');
  bytes.append(reinterpret_cast<const char *>(image.pixels.data()), image.pixels.size());
  return sha256(bytes);
}
bool overlaps(const AtlasPlacement &left, const AtlasPlacement &right) {
  return left.x < static_cast<std::uint64_t>(right.x) + right.width &&
         right.x < static_cast<std::uint64_t>(left.x) + left.width &&
         left.y < static_cast<std::uint64_t>(right.y) + right.height &&
         right.y < static_cast<std::uint64_t>(left.y) + left.height;
}
} // namespace

ValidationResult validate_projection2d(const Projection2dDefinition &projection) {
  ValidationResult result;
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  if (!id(projection.id) || projection.source_frames.empty() ||
      projection.source_frames.size() > 4096 || projection.animations.empty() ||
      projection.animations.size() > 4096 || projection.channel_maps.size() > 24'576 ||
      projection.maximum_ability_active_ticks == 0 ||
      projection.maximum_ability_active_ticks > 3'600'000 ||
      !projection.sheet.atlas.image.invariant() || !projection.sheet.alpha.invariant() ||
      !projection.sheet.outline.invariant())
    add("SPRITE_PROJECTION_2D_INVALID", "2D projection identity, bounds, or compiled images are invalid");
  const auto &atlas = projection.sheet.atlas.image;
  if (projection.sheet.alpha.width != atlas.width || projection.sheet.alpha.height != atlas.height ||
      projection.sheet.outline.width != atlas.width || projection.sheet.outline.height != atlas.height ||
      projection.sheet.atlas.placements.size() != projection.source_frames.size())
    add("SPRITE_PROJECTION_2D_AUXILIARY_INVALID", "2D masks or placement coverage differ from the atlas");
  std::set<std::string> frames;
  for (const auto &frame : projection.source_frames)
    if (!id(frame.id) || !frame.image.invariant() || frame.duration_ticks == 0 ||
        frame.duration_ticks > 3'600'000 || !frames.insert(frame.id).second)
      add("SPRITE_PROJECTION_2D_FRAME_INVALID", "2D source frame is malformed or duplicate");
  std::set<std::string> placements;
  for (std::size_t index = 0; index < projection.sheet.atlas.placements.size(); ++index) {
    const auto &placement = projection.sheet.atlas.placements[index];
    const auto source = std::ranges::find(projection.source_frames, placement.frame_id, &FrameSource::id);
    if (source == projection.source_frames.end() || !placements.insert(placement.frame_id).second ||
        placement.width != source->image.width || placement.height != source->image.height ||
        placement.pivot_x != source->pivot_x || placement.pivot_y != source->pivot_y ||
        placement.duration_ticks != source->duration_ticks || placement.width == 0 || placement.height == 0 ||
        static_cast<std::uint64_t>(placement.x) + placement.width > atlas.width ||
        static_cast<std::uint64_t>(placement.y) + placement.height > atlas.height)
      add("SPRITE_PROJECTION_2D_PLACEMENT_INVALID", "atlas placement is absent, inconsistent, or out of bounds");
    for (std::size_t other = index + 1; other < projection.sheet.atlas.placements.size(); ++other)
      if (overlaps(placement, projection.sheet.atlas.placements[other]))
        add("SPRITE_PROJECTION_2D_PLACEMENT_OVERLAP", "atlas placements overlap");
  }
  try {
    if (projection.sheet.metadata != canonicalize_atlas_metadata(projection.sheet.atlas))
      add("SPRITE_PROJECTION_2D_METADATA_INVALID", "atlas metadata is not canonical or does not match placements");
  } catch (const std::exception &error) {
    add("SPRITE_PROJECTION_2D_METADATA_INVALID", error.what());
  }
  std::set<std::string> animations, animated_frames;
  for (const auto &animation : projection.animations) {
    const auto validation = validate_animation(animation, projection.sheet.atlas.placements);
    result.diagnostics.insert(result.diagnostics.end(), validation.diagnostics.begin(), validation.diagnostics.end());
    if (!animations.insert(animation.id).second)
      add("SPRITE_PROJECTION_2D_ANIMATION_DUPLICATE", "2D animation identity is duplicate");
    animated_frames.insert(animation.frame_ids.begin(), animation.frame_ids.end());
  }
  if (animated_frames != frames)
    add("SPRITE_PROJECTION_2D_ANIMATION_COVERAGE", "every source frame must belong to an animation");
  for (const auto &map : projection.channel_maps) {
    const auto target = std::ranges::find(projection.source_frames, map.target_frame_id, &FrameSource::id);
    if (target == projection.source_frames.end())
      add("SPRITE_PROJECTION_2D_CHANNEL_TARGET", "channel map target frame is absent");
    else {
      const auto validation = validate_channel_map(map, *target);
      result.diagnostics.insert(result.diagnostics.end(), validation.diagnostics.begin(), validation.diagnostics.end());
    }
  }
  try { (void)canonicalize_channel_maps(projection.channel_maps); }
  catch (const std::exception &error) { add("SPRITE_PROJECTION_2D_CHANNEL_SET", error.what()); }
  const auto rig_validation = validate_rig(projection.rig);
  result.diagnostics.insert(result.diagnostics.end(), rig_validation.diagnostics.begin(), rig_validation.diagnostics.end());
  if (rig_validation.ok()) {
    const auto collision_validation = validate_collision_contract(projection.collision_shapes,
        projection.collision_windows, projection.rig, projection.maximum_ability_active_ticks);
    result.diagnostics.insert(result.diagnostics.end(), collision_validation.diagnostics.begin(), collision_validation.diagnostics.end());
  }
  return result;
}

std::string canonicalize_projection2d(const Projection2dDefinition &projection) {
  const auto validation = validate_projection2d(projection);
  if (!validation.ok()) throw std::invalid_argument(validation.diagnostics.front().message);
  auto frames = projection.source_frames; auto animations = projection.animations;
  auto channels = projection.channel_maps;
  std::ranges::sort(frames, {}, &FrameSource::id); std::ranges::sort(animations, {}, &AnimationClip::id);
  std::ranges::sort(channels, {}, &ChannelMap::id);
  std::ostringstream output;
  output << "schema=gspl.projection-2d/0.1\nid=" << projection.id
         << "\natlas=" << image_identity(projection.sheet.atlas.image)
         << "\nalpha=" << image_identity(projection.sheet.alpha)
         << "\noutline=" << image_identity(projection.sheet.outline)
         << "\nmetadata=" << sha256(projection.sheet.metadata)
         << "\nrig=" << sha256(canonicalize_rig(projection.rig))
         << "\nchannels=" << sha256(canonicalize_channel_maps(projection.channel_maps)) << '\n';
  for (const auto &frame : frames)
    output << "frame=" << frame.id << '|' << image_identity(frame.image) << '|'
           << frame.pivot_x << '|' << frame.pivot_y << '|' << frame.duration_ticks << '\n';
  for (const auto &animation : animations) {
    output << "animation=" << animation.id << '|' << (animation.looping ? "true" : "false") << '\n';
    for (std::size_t i = 0; i < animation.frame_ids.size(); ++i)
      output << "animation_frame=" << animation.id << '|' << i << '|'
             << animation.frame_ids[i] << '|' << animation.frame_durations[i] << '\n';
    for (const auto &event : animation.events)
      output << "animation_event=" << animation.id << '|' << event.tick << '|' << event.id << '\n';
  }
  for (const auto &channel : channels)
    output << "channel=" << channel.id << '|' << channel.target_frame_id << '|'
           << static_cast<int>(channel.kind) << '|'
           << image_identity(channel.image) << '\n';
  output << "collisions=" << sha256(canonicalize_collisions(projection.collision_shapes,
      projection.collision_windows));
  return output.str();
}

std::string projection2d_identity(const Projection2dDefinition &projection) {
  return sha256(canonicalize_projection2d(projection));
}

} // namespace gspl::sprites
