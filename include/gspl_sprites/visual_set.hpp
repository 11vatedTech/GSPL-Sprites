#pragma once

#include "gspl_sprites/sprite2d.hpp"

#include <filesystem>

namespace gspl::sprites {

enum class Direction2d { none, north, north_east, east, south_east, south, south_west, west, north_west };

struct VisualFrameSemantic {
  std::string animation;
  Direction2d direction{Direction2d::none};
  std::string layer;
  std::uint32_t ordinal{};
  std::string frame_id;
};

struct AuthoredVisualSet {
  std::string schema;
  SpriteSheetOptions sheet;
  std::vector<FrameSource> frames;
  std::vector<VisualFrameSemantic> semantics;
  std::vector<std::string> layer_order;
  bool pixel_art{};
  PixelArtPolicy pixel_policy;
  std::uint32_t temporal_max_changed_per_million{1'000'000};
  std::uint32_t temporal_min_silhouette_iou_per_million{};
  std::vector<TemporalTransitionMetrics> temporal_metrics;
  std::string canonical_metadata;
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
