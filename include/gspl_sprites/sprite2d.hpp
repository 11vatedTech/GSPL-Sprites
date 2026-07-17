#pragma once

#include "gspl_sprites/image.hpp"

namespace gspl::sprites {

struct TrimmedFrame {
  FrameSource frame;
  std::uint32_t source_x{};
  std::uint32_t source_y{};
  std::uint32_t source_width{};
  std::uint32_t source_height{};
};

struct PaletteEntry {
  std::uint32_t rgba{};
  std::uint64_t occurrences{};
};

struct PixelArtPolicy {
  std::uint32_t grid_width{1};
  std::uint32_t grid_height{1};
  std::uint32_t maximum_colors{256};
  bool binary_alpha{true};
};

struct SpriteSheetOptions {
  std::uint32_t maximum_width{4096};
  std::uint32_t maximum_height{4096};
  std::uint32_t padding{1};
  bool trim_frames{true};
  std::uint8_t alpha_threshold{};
};

struct SpriteSheetArtifacts {
  AtlasResult atlas;
  ImageRgba8 alpha;
  ImageRgba8 outline;
  std::string metadata;
};

struct TemporalTransitionMetrics {
  std::string from_frame;
  std::string to_frame;
  std::uint64_t compared_pixels{};
  std::uint64_t changed_pixels{};
  std::uint64_t silhouette_union_pixels{};
  std::uint64_t silhouette_intersection_pixels{};
  std::uint32_t changed_per_million{};
  std::uint32_t silhouette_iou_per_million{};
};

[[nodiscard]] TrimmedFrame trim_transparent(const FrameSource& source,
                                            std::uint8_t alpha_threshold = 0);
[[nodiscard]] FrameSource scale_nearest(const FrameSource& source,
                                        std::uint32_t integer_scale,
                                        const ImageLimits& limits = {});
[[nodiscard]] std::vector<PaletteEntry> extract_palette(const ImageRgba8& image,
                                                        std::uint32_t maximum_entries = 256);
[[nodiscard]] ImageRgba8 remap_palette(const ImageRgba8& image,
                                       std::span<const std::uint32_t> rgba_palette);
[[nodiscard]] ImageRgba8 alpha_mask(const ImageRgba8& image,
                                    std::uint8_t threshold = 0);
[[nodiscard]] ImageRgba8 outline_mask(const ImageRgba8& image,
                                      std::uint8_t threshold = 0);
[[nodiscard]] ValidationResult validate_pixel_art(const ImageRgba8& image,
                                                  const PixelArtPolicy& policy);
[[nodiscard]] std::string canonicalize_atlas_metadata(const AtlasResult& atlas);
[[nodiscard]] SpriteSheetArtifacts compile_sprite_sheet(std::span<const FrameSource> frames,
                                                        const SpriteSheetOptions& options = {});
[[nodiscard]] std::vector<TemporalTransitionMetrics> analyze_temporal_stability(
    std::span<const FrameSource> ordered_frames, std::uint8_t alpha_threshold = 0,
    std::uint64_t maximum_comparison_pixels = 64ULL * 1024ULL * 1024ULL);

} // namespace gspl::sprites
