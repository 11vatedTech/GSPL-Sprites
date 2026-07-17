#pragma once

#include "gspl_sprites/image.hpp"

namespace gspl::sprites {

enum class ChannelMapKind { material_id, tangent_normal, depth, emissive, effects, collision };

struct ChannelMap {
  std::string id;
  std::string target_frame_id;
  ChannelMapKind kind{ChannelMapKind::effects};
  ImageRgba8 image;
};

[[nodiscard]] ValidationResult validate_channel_map(const ChannelMap& map,
                                                    const FrameSource& target);
[[nodiscard]] std::string canonicalize_channel_maps(std::span<const ChannelMap> maps);

} // namespace gspl::sprites
