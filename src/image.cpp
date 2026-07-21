#include "gspl_sprites/image.hpp"
#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace gspl::sprites {
namespace {
bool whitespace(std::byte value) {
  const auto c = std::to_integer<unsigned char>(value);
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

class PpmReader final {
public:
  explicit PpmReader(std::span<const std::byte> bytes) : bytes_(bytes) {}
  std::string token() {
    skip_space_and_comments();
    const auto start = cursor_;
    while (cursor_ < bytes_.size() && !whitespace(bytes_[cursor_]) && bytes_[cursor_] != std::byte{'#'}) ++cursor_;
    if (start == cursor_) throw std::runtime_error("PPM header token is missing");
    return {reinterpret_cast<const char*>(bytes_.data() + start), cursor_ - start};
  }
  std::size_t consume_raster_separator() {
    if (cursor_ >= bytes_.size() || !whitespace(bytes_[cursor_])) throw std::runtime_error("PPM raster separator is missing");
    if (bytes_[cursor_] == std::byte{'\r'} && cursor_ + 1 < bytes_.size() && bytes_[cursor_ + 1] == std::byte{'\n'}) cursor_ += 2;
    else ++cursor_;
    return cursor_;
  }
private:
  void skip_space_and_comments() {
    for (;;) {
      while (cursor_ < bytes_.size() && whitespace(bytes_[cursor_])) ++cursor_;
      if (cursor_ >= bytes_.size() || bytes_[cursor_] != std::byte{'#'}) return;
      while (cursor_ < bytes_.size() && bytes_[cursor_] != std::byte{'\n'} && bytes_[cursor_] != std::byte{'\r'}) ++cursor_;
    }
  }
  std::span<const std::byte> bytes_; std::size_t cursor_{};
};

std::uint32_t decimal(std::string_view token, std::string_view field) {
  std::uint32_t value{}; const auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
  if (error != std::errc{} || end != token.data() + token.size()) throw std::runtime_error("invalid PPM " + std::string(field));
  return value;
}
}

bool ImageRgba8::invariant() const noexcept {
  if (width == 0 || height == 0) return false;
  const auto required = static_cast<std::uint64_t>(width) * height * 4ULL;
  return required <= std::numeric_limits<std::size_t>::max() && pixels.size() == static_cast<std::size_t>(required);
}

ImageRgba8 decode_ppm_p6(std::span<const std::byte> encoded, const ImageLimits& limits) {
  if (encoded.size() > limits.max_input_bytes) throw std::runtime_error("image input exceeds byte limit");
  PpmReader reader(encoded);
  if (reader.token() != "P6") throw std::runtime_error("unsupported PPM magic; expected P6");
  const auto width = decimal(reader.token(), "width"); const auto height = decimal(reader.token(), "height");
  if (decimal(reader.token(), "maximum channel") != 255) throw std::runtime_error("only 8-bit PPM channels are supported");
  const auto raster = reader.consume_raster_separator();
  if (width == 0 || height == 0 || width > limits.max_width || height > limits.max_height) throw std::runtime_error("image dimensions exceed limits");
  const auto pixels = static_cast<std::uint64_t>(width) * height;
  if (pixels > limits.max_pixels || pixels > limits.max_decoded_bytes / 4ULL) throw std::runtime_error("decoded image exceeds limits");
  const auto rgb_bytes = pixels * 3ULL;
  if (rgb_bytes > std::numeric_limits<std::size_t>::max() || raster > encoded.size() || encoded.size() - raster != rgb_bytes) throw std::runtime_error("PPM raster length is invalid");
  ImageRgba8 image{width, height, ColorSpace::srgb, AlphaMode::opaque, {}};
  image.pixels.resize(static_cast<std::size_t>(pixels * 4ULL));
  for (std::size_t src = raster, dst = 0; src < encoded.size(); src += 3, dst += 4) {
    image.pixels[dst] = std::to_integer<std::uint8_t>(encoded[src]); image.pixels[dst + 1] = std::to_integer<std::uint8_t>(encoded[src + 1]); image.pixels[dst + 2] = std::to_integer<std::uint8_t>(encoded[src + 2]); image.pixels[dst + 3] = 255;
  }
  return image;
}

std::vector<std::byte> encode_ppm_p6(const ImageRgba8& image) {
  if (!image.invariant()) throw std::invalid_argument("invalid RGBA image");
  if (image.color_space != ColorSpace::srgb) throw std::invalid_argument("PPM encoder requires explicit sRGB input");
  if (image.alpha_mode != AlphaMode::opaque) {
    for (std::size_t index = 3; index < image.pixels.size(); index += 4)
      if (image.pixels[index] != 255) throw std::invalid_argument("PPM cannot represent transparency");
  }
  const auto header = "P6\n" + std::to_string(image.width) + " " + std::to_string(image.height) + "\n255\n";
  std::vector<std::byte> output; output.reserve(header.size() + static_cast<std::size_t>(image.width) * image.height * 3ULL);
  for (const char c : header) output.push_back(static_cast<std::byte>(c));
  for (std::size_t index = 0; index < image.pixels.size(); index += 4) { output.push_back(static_cast<std::byte>(image.pixels[index])); output.push_back(static_cast<std::byte>(image.pixels[index + 1])); output.push_back(static_cast<std::byte>(image.pixels[index + 2])); }
  return output;
}

AtlasResult pack_atlas(std::span<const FrameSource> frames, std::uint32_t max_width, std::uint32_t max_height, std::uint32_t padding) {
  if (frames.empty() || max_width == 0 || max_height == 0 || padding > 64) throw std::invalid_argument("invalid atlas request");
  constexpr std::uint64_t max_atlas_pixels = 64ULL * 1024ULL * 1024ULL;
  const auto atlas_pixels = static_cast<std::uint64_t>(max_width) * max_height;
  if (max_width > 16384 || max_height > 16384 || atlas_pixels > max_atlas_pixels || atlas_pixels > std::numeric_limits<std::size_t>::max() / 4ULL)
    throw std::invalid_argument("atlas dimensions exceed resource limits");
  std::vector<const FrameSource*> ordered; ordered.reserve(frames.size()); std::set<std::string> ids;
  for (const auto& frame : frames) {
    if (frame.id.empty() || !frame.image.invariant() || frame.image.color_space == ColorSpace::unknown || frame.duration_ticks == 0 || !ids.insert(frame.id).second) throw std::invalid_argument("invalid, untagged, or duplicate atlas frame");
    if (frame.image.color_space != frames.front().image.color_space || frame.image.alpha_mode != frames.front().image.alpha_mode) throw std::invalid_argument("atlas frames must share color and alpha semantics");
    ordered.push_back(&frame);
  }
  std::ranges::sort(ordered, [](const auto* left, const auto* right) { if (left->image.height != right->image.height) return left->image.height > right->image.height; if (left->image.width != right->image.width) return left->image.width > right->image.width; return left->id < right->id; });
  AtlasResult result{{max_width, max_height, frames.front().image.color_space, frames.front().image.alpha_mode, std::vector<std::uint8_t>(static_cast<std::size_t>(atlas_pixels * 4ULL), 0)}, {}};
  std::uint32_t x = padding, y = padding, row_height = 0;
  for (const auto* frame : ordered) {
    if (frame->image.width > max_width - std::min(max_width, padding * 2) || frame->image.height > max_height - std::min(max_height, padding * 2)) throw std::runtime_error("frame exceeds atlas dimensions");
    if (x + frame->image.width + padding > max_width) { x = padding; y += row_height + padding; row_height = 0; }
    if (y + frame->image.height + padding > max_height) throw std::runtime_error("frames do not fit atlas");
    for (std::uint32_t row = 0; row < frame->image.height; ++row) {
      const auto source = static_cast<std::size_t>(row) * frame->image.width * 4ULL;
      const auto target = (static_cast<std::size_t>(y + row) * max_width + x) * 4ULL;
      std::ranges::copy_n(frame->image.pixels.begin() + static_cast<std::ptrdiff_t>(source), static_cast<std::size_t>(frame->image.width) * 4ULL, result.image.pixels.begin() + static_cast<std::ptrdiff_t>(target));
    }
    result.placements.push_back({frame->id, x, y, frame->image.width, frame->image.height, frame->pivot_x, frame->pivot_y, frame->duration_ticks});
    x += frame->image.width + padding; row_height = std::max(row_height, frame->image.height);
  }
  std::ranges::sort(result.placements, {}, &AtlasPlacement::frame_id); return result;
}

ValidationResult validate_animation(const AnimationClip& clip, std::span<const AtlasPlacement> frames) {
  ValidationResult result; auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  if (clip.id.empty() || clip.id.size() > 128) add("SPRITE_ANIMATION_ID_INVALID", "animation id must contain 1..128 bytes");
  if (clip.frame_ids.empty() || clip.frame_ids.size() != clip.frame_durations.size() || clip.frame_ids.size() > 4096) add("SPRITE_ANIMATION_FRAMES_INVALID", "frame ids and durations must have equal bounded nonzero length");
  std::set<std::string_view> available; for (const auto& frame : frames) available.insert(frame.frame_id);
  std::uint64_t total = 0; for (std::size_t i = 0; i < clip.frame_ids.size(); ++i) { if (!available.contains(clip.frame_ids[i])) add("SPRITE_ANIMATION_FRAME_MISSING", "animation references absent frame: " + clip.frame_ids[i]); if (clip.frame_durations[i] == 0) add("SPRITE_ANIMATION_DURATION_INVALID", "frame duration must be positive"); total += clip.frame_durations[i]; }
  if (total == 0 || total > 36000) add("SPRITE_ANIMATION_LENGTH_INVALID", "animation duration must be 1..36000 ticks");
  std::set<std::string> event_keys; for (const auto& event : clip.events) { if (event.id.empty() || event.tick >= total) add("SPRITE_ANIMATION_EVENT_INVALID", "animation event is empty or outside clip"); if (!event_keys.insert(event.id + ":" + std::to_string(event.tick)).second) add("SPRITE_ANIMATION_EVENT_DUPLICATE", "duplicate animation event at tick"); }
  return result;
}
ValidationResult validate_frame_distinctness(std::span<const FrameSource> frames, std::span<const AnimationClip> clips) {
  ValidationResult result;
  auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  std::unordered_map<std::string_view, std::string_view> frame_hashes;
  for (const auto& f : frames) frame_hashes[f.id] = f.frame_hash;
  for (const auto& clip : clips) {
    if (clip.frame_ids.empty()) continue;
    std::unordered_set<std::string_view> seen;
    for (const auto& fid : clip.frame_ids) {
      auto it = frame_hashes.find(fid);
      if (it == frame_hashes.end()) {
        add("SPRITE_ANIMATION_FRAME_MISSING", "clip '" + clip.id + "' references missing frame: " + fid);
        continue;
      }
      if (it->second.empty()) {
        add("SPRITE_ANIMATION_FRAME_UNHASHED", "frame '" + fid + "' in clip '" + clip.id + "' has no hash");
        continue;
      }
      if (!seen.insert(it->second).second)
        add("SPRITE_ANIMATION_FRAME_DUPLICATE_CONTENT", "clip '" + clip.id + "' contains frames with duplicate content hash: " + fid);
    }
  }
  return result;
}
std::string compute_frame_hash(const ImageRgba8& image) {
  std::string preimage;
  preimage += "GSPL_FRAME_V1";
  preimage += static_cast<char>(image.width & 0xFF);
  preimage += static_cast<char>((image.width >> 8) & 0xFF);
  preimage += static_cast<char>((image.width >> 16) & 0xFF);
  preimage += static_cast<char>((image.width >> 24) & 0xFF);
  preimage += static_cast<char>(image.height & 0xFF);
  preimage += static_cast<char>((image.height >> 8) & 0xFF);
  preimage += static_cast<char>((image.height >> 16) & 0xFF);
  preimage += static_cast<char>((image.height >> 24) & 0xFF);
  preimage += "RGBA8";
  preimage += "RGBA";
  preimage.append(reinterpret_cast<const char*>(image.pixels.data()), image.pixels.size());
  return sha256(preimage);
}
std::string compute_frame_hash(const FrameSource& frame) {
  return compute_frame_hash(frame.image);
}
PixelAABB compute_pixel_aabb(const ImageRgba8& image) {
  PixelAABB aabb;
  if (image.pixels.empty()) return aabb;
  bool first = true;
  for (std::uint32_t y = 0; y < image.height; ++y) {
    for (std::uint32_t x = 0; x < image.width; ++x) {
      if (image.pixels[(y * image.width + x) * 4 + 3] != 0) {
        if (first) {
          aabb.min_x = aabb.max_x = x;
          aabb.min_y = aabb.max_y = y;
          first = false;
        } else {
          if (x < aabb.min_x) aabb.min_x = x;
          if (x > aabb.max_x) aabb.max_x = x;
          if (y < aabb.min_y) aabb.min_y = y;
          if (y > aabb.max_y) aabb.max_y = y;
        }
      }
    }
  }
  if (!first) aabb.empty = false;
  return aabb;
}
} // namespace gspl::sprites
