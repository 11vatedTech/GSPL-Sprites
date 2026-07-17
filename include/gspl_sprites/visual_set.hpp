#pragma once

#include "gspl_sprites/sprite2d.hpp"

#include <filesystem>

namespace gspl::sprites {

struct AuthoredVisualSet {
  std::string schema;
  SpriteSheetOptions sheet;
  std::vector<FrameSource> frames;
};

struct VisualSetLimits {
  ImageLimits image;
  std::uint64_t max_manifest_bytes{1ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_encoded_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_decoded_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint32_t max_frames{4096};
};

[[nodiscard]] AuthoredVisualSet load_authored_visual_set(const std::filesystem::path& manifest_path,
                                                        const VisualSetLimits& limits = {});

} // namespace gspl::sprites
