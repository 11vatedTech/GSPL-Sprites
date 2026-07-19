#pragma once

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/sprite2d.hpp"

#include <cstdint>

namespace gspl::sprites {

struct Projection2dDefinition {
  std::string id;
  std::vector<FrameSource> source_frames;
  SpriteSheetArtifacts sheet;
  std::vector<AnimationClip> animations;
  std::vector<ChannelMap> channel_maps;
  RigDefinition rig;
  std::vector<CollisionShape> collision_shapes;
  std::vector<CollisionWindow> collision_windows;
  std::uint32_t maximum_ability_active_ticks{1};
};

[[nodiscard]] ValidationResult
validate_projection2d(const Projection2dDefinition &projection);
[[nodiscard]] std::string
canonicalize_projection2d(const Projection2dDefinition &projection);
[[nodiscard]] std::string
projection2d_identity(const Projection2dDefinition &projection);

} // namespace gspl::sprites
