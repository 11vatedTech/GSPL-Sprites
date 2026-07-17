#pragma once

#include "gspl_sprites/common.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace gspl::sprites {

enum class ColorSpace { srgb, linear_srgb, acescg, data, unknown };
enum class AlphaMode { opaque, straight, premultiplied };

struct ImageLimits {
  std::uint32_t max_width{16384};
  std::uint32_t max_height{16384};
  std::uint64_t max_pixels{64ULL * 1024ULL * 1024ULL};
  std::uint64_t max_decoded_bytes{256ULL * 1024ULL * 1024ULL};
  std::uint64_t max_input_bytes{256ULL * 1024ULL * 1024ULL};
  std::uint64_t max_metadata_bytes{16ULL * 1024ULL * 1024ULL};
  std::uint32_t max_chunk_count{1000};
};

struct ImageRgba8 {
  std::uint32_t width{};
  std::uint32_t height{};
  ColorSpace color_space{ColorSpace::unknown};
  AlphaMode alpha_mode{AlphaMode::straight};
  std::vector<std::uint8_t> pixels;
  [[nodiscard]] bool invariant() const noexcept;
};

[[nodiscard]] ImageRgba8 decode_ppm_p6(std::span<const std::byte> encoded,
                                       const ImageLimits& limits = {});
[[nodiscard]] std::vector<std::byte> encode_ppm_p6(const ImageRgba8& image);
[[nodiscard]] ImageRgba8 decode_png(std::span<const std::byte> encoded,
                                   const ImageLimits& limits = {});
[[nodiscard]] std::vector<std::byte> encode_png(const ImageRgba8& image);

struct FrameSource {
  std::string id;
  ImageRgba8 image;
  std::int32_t pivot_x{};
  std::int32_t pivot_y{};
  std::uint32_t duration_ticks{1};
};

struct AtlasPlacement {
  std::string frame_id;
  std::uint32_t x{};
  std::uint32_t y{};
  std::uint32_t width{};
  std::uint32_t height{};
  std::int32_t pivot_x{};
  std::int32_t pivot_y{};
  std::uint32_t duration_ticks{};
};

struct AtlasResult { ImageRgba8 image; std::vector<AtlasPlacement> placements; };
[[nodiscard]] AtlasResult pack_atlas(std::span<const FrameSource> frames,
                                     std::uint32_t max_width,
                                     std::uint32_t max_height,
                                     std::uint32_t padding = 1);

struct AnimationEvent { std::string id; std::uint32_t tick{}; };
struct AnimationClip {
  std::string id;
  std::vector<std::string> frame_ids;
  std::vector<std::uint32_t> frame_durations;
  std::vector<AnimationEvent> events;
  bool looping{};
};
[[nodiscard]] ValidationResult validate_animation(const AnimationClip& clip,
                                                  std::span<const AtlasPlacement> frames);

} // namespace gspl::sprites
