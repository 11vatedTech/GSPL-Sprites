#include "gspl_sprites/synthesis.hpp"

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/animation3d.hpp"
#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/projection25d.hpp"
#include "gspl_sprites/projection3d.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"
#include "gspl_sprites/combat.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace gspl::sprites {

std::uint32_t hex_to_u32(std::string_view hex) {
  if (hex.size() != 7 || hex[0] != '#') return 0xFFFFFFFF;
  std::uint32_t r = 0, g = 0, b = 0;
  auto val = [](char c) -> std::uint32_t {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
  };
  for (int i = 1; i < 7; i += 2) {
    std::uint32_t v = (val(hex[i]) << 4) | val(hex[i + 1]);
    if (i == 1) r = v;
    else if (i == 3) g = v;
    else b = v;
  }
  return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

ImageRgba8 make_body_sprite(std::uint32_t w, std::uint32_t h, std::uint32_t primary, std::uint32_t accent) {
  ImageRgba8 img{w, h, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(w * h * 4, 0)};
  std::uint8_t pr = (primary >> 24) & 0xFF, pg = (primary >> 16) & 0xFF, pb = (primary >> 8) & 0xFF, pa = primary & 0xFF;
  std::uint8_t ar = (accent >> 24) & 0xFF, ag = (accent >> 16) & 0xFF, ab = (accent >> 8) & 0xFF, aa = accent & 0xFF;
  int cx = w / 2, cy = h / 2;
  int body_r = std::min(w, h) / 3;
  for (int y = 0; y < (int)h; ++y) {
    for (int x = 0; x < (int)w; ++x) {
      int dx = x - cx, dy = y - cy;
      if (dx*dx + dy*dy <= body_r*body_r) {
        std::size_t idx = (y * w + x) * 4;
        img.pixels[idx] = pr; img.pixels[idx+1] = pg; img.pixels[idx+2] = pb; img.pixels[idx+3] = pa;
      }
    }
  }
  int eye_y = cy - body_r / 3;
  int eye_r = body_r / 5;
  for (int y = 0; y < (int)h; ++y) {
    for (int x = 0; x < (int)w; ++x) {
      int dx1 = x - (cx - body_r/2), dy1 = y - eye_y;
      int dx2 = x - (cx + body_r/2), dy2 = y - eye_y;
      if ((dx1*dx1 + dy1*dy1 <= eye_r*eye_r) || (dx2*dx2 + dy2*dy2 <= eye_r*eye_r)) {
        std::size_t idx = (y * w + x) * 4;
        img.pixels[idx] = ar; img.pixels[idx+1] = ag; img.pixels[idx+2] = ab; img.pixels[idx+3] = aa;
      }
    }
  }
  return img;
}

ImageRgba8 make_transformed_sprite(std::uint32_t w, std::uint32_t h, std::uint32_t primary, std::uint32_t accent, std::uint32_t energy) {
  ImageRgba8 img = make_body_sprite(w, h, primary, accent);
  std::uint8_t er = (energy >> 24) & 0xFF, eg = (energy >> 16) & 0xFF, eb = (energy >> 8) & 0xFF, ea = energy & 0xFF;
  int cx = w / 2, cy = h / 2;
  int aura_r = std::min(w, h) / 2;
  for (int y = 0; y < (int)h; ++y) {
    for (int x = 0; x < (int)w; ++x) {
      int dx = x - cx, dy = y - cy;
      int dist2 = dx*dx + dy*dy;
      if (dist2 > (aura_r/2)*(aura_r/2) && dist2 <= aura_r*aura_r) {
        std::size_t idx = (y * w + x) * 4;
        if (img.pixels[idx+3] == 0) {
          img.pixels[idx] = er; img.pixels[idx+1] = eg; img.pixels[idx+2] = eb; img.pixels[idx+3] = ea / 2;
        } else {
          img.pixels[idx] = static_cast<std::uint8_t>(std::min(255, static_cast<int>(img.pixels[idx]) + static_cast<int>(er) / 4));
          img.pixels[idx+1] = static_cast<std::uint8_t>(std::min(255, static_cast<int>(img.pixels[idx+1]) + static_cast<int>(eg) / 4));
          img.pixels[idx+2] = static_cast<std::uint8_t>(std::min(255, static_cast<int>(img.pixels[idx+2]) + static_cast<int>(eb) / 4));
        }
      }
    }
  }
  return img;
}

std::vector<FrameSource> make_frames(std::string_view prefix, std::string_view form,
                                     std::uint32_t primary, std::uint32_t accent, bool transformed) {
  std::vector<FrameSource> frames;
  std::string pf = std::string(prefix) + "." + std::string(form);
  if (!transformed) {
    frames.push_back({pf + ".idle.0", make_body_sprite(32, 32, primary, accent), 16, 16, 2});
    frames.push_back({pf + ".idle.1", make_body_sprite(32, 32, primary, accent), 16, 16, 2});
    frames.push_back({pf + ".walk.0", make_body_sprite(32, 32, primary, accent), 16, 16, 2});
    frames.push_back({pf + ".walk.1", make_body_sprite(32, 32, primary, accent), 16, 16, 2});
    frames.push_back({pf + ".attack.0", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
    frames.push_back({pf + ".attack.1", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
    frames.push_back({pf + ".hit.0", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
    frames.push_back({pf + ".transform.0", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
    frames.push_back({pf + ".transform.1", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
    frames.push_back({pf + ".transform.2", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
    frames.push_back({pf + ".transform.3", make_body_sprite(32, 32, primary, accent), 16, 16, 1});
  } else {
    std::uint32_t energy = 0xFFFF8000;
    frames.push_back({pf + ".idle.0", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 2});
    frames.push_back({pf + ".idle.1", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 2});
    frames.push_back({pf + ".walk.0", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 2});
    frames.push_back({pf + ".walk.1", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 2});
    frames.push_back({pf + ".attack.0", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
    frames.push_back({pf + ".attack.1", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
    frames.push_back({pf + ".hit.0", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
    frames.push_back({pf + ".transform.0", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
    frames.push_back({pf + ".transform.1", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
    frames.push_back({pf + ".transform.2", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
    frames.push_back({pf + ".transform.3", make_transformed_sprite(32, 32, primary, accent, energy), 16, 16, 1});
  }
  for (auto& f : frames) f.frame_hash = compute_frame_hash(f.image);
  return frames;
}

std::vector<AnimationClip> make_animations(std::string_view prefix, std::string_view form) {
  std::string pf = std::string(prefix) + "." + std::string(form);
  return {
    {pf + ".idle", {pf + ".idle.0", pf + ".idle.1"}, {2, 2}, {}, true},
    {pf + ".walk", {pf + ".walk.0", pf + ".walk.1"}, {2, 2}, {}, true},
    {pf + ".attack", {pf + ".attack.0", pf + ".attack.1"}, {1, 1}, {{"hit", 1}}, false},
    {pf + ".hit", {pf + ".hit.0"}, {1}, {}, false},
    {pf + ".transform", {pf + ".transform.0", pf + ".transform.1", pf + ".transform.2", pf + ".transform.3"}, {1, 1, 1, 1}, {}, false},
  };
}

std::vector<ChannelMap> make_channels(std::string_view prefix, std::string_view form) {
  std::string pf = std::string(prefix) + "." + std::string(form);
  ImageRgba8 depth_img = ImageRgba8{32, 32, ColorSpace::data, AlphaMode::opaque, std::vector<std::uint8_t>(32*32*4, 255)};
  for (std::size_t i = 0; i < depth_img.pixels.size(); i += 4) {
    depth_img.pixels[i] = depth_img.pixels[i+1] = depth_img.pixels[i+2] = 64;
  }
  return {
    {pf + ".idle.0.depth", pf + ".idle.0", ChannelMapKind::depth, depth_img},
    {pf + ".idle.1.depth", pf + ".idle.1", ChannelMapKind::depth, depth_img},
  };
}

RigDefinition make_biped_rig(std::string_view id) {
  RigDefinition rig;
  rig.id = std::string(id) + ".rig";
  rig.bones = {
    {"root", std::nullopt, {0.0, 0.0, 0.0, 1.0, 1.0}, 0.0, {-180.0, 180.0}},
    {"torso", "root", {0.0, -16.0, 0.0, 1.0, 1.0}, 16.0, {-90.0, 90.0}},
    {"head", "torso", {0.0, -20.0, 0.0, 1.0, 1.0}, 12.0, {-45.0, 45.0}},
    {"arm_l", "torso", {-12.0, -12.0, 0.0, 1.0, 1.0}, 14.0, {-135.0, 45.0}},
    {"arm_r", "torso", {12.0, -12.0, 0.0, 1.0, 1.0}, 14.0, {-45.0, 135.0}},
    {"leg_l", "root", {-6.0, 16.0, 0.0, 1.0, 1.0}, 18.0, {-45.0, 90.0}},
    {"leg_r", "root", {6.0, 16.0, 0.0, 1.0, 1.0}, 18.0, {-90.0, 45.0}},
  };
  rig.sockets = {
    {"hand_l", "arm_l", {-14.0, 0.0, 0.0, 1.0, 1.0}},
    {"hand_r", "arm_r", {14.0, 0.0, 0.0, 1.0, 1.0}},
    {"foot_l", "leg_l", {0.0, 18.0, 0.0, 1.0, 1.0}},
    {"foot_r", "leg_r", {0.0, 18.0, 0.0, 1.0, 1.0}},
    {"head_top", "head", {0.0, -12.0, 0.0, 1.0, 1.0}},
  };
  return rig;
}

std::vector<CollisionShape> make_collision_shapes() {
  return {};
}

std::vector<CollisionWindow> make_collision_windows() {
  return {};
}

RigDefinition make_rig_from_ir(const SpriteIr& ir) {
  if (ir.rig) return *ir.rig;
  return make_biped_rig(ir.entity_id);
}

Projection2dDefinition synthesize_projection2d(std::string_view entity_id, std::string_view form_id,
                                               const SynthesisPalette& palette,
                                               const RigDefinition& rig) {
  std::vector<FrameSource> frames = make_frames(entity_id, form_id, palette.primary, palette.accent, false);
  SpriteSheetOptions options{256, 256, 2, false, 0};
  SpriteSheetArtifacts sheet = compile_sprite_sheet(frames, options);
  std::vector<AnimationClip> animations = make_animations(entity_id, form_id);
  std::vector<ChannelMap> channels = make_channels(entity_id, form_id);
  std::vector<CollisionShape> shapes = make_collision_shapes();
  std::vector<CollisionWindow> windows = make_collision_windows();
  std::string proj_id = std::string(entity_id) + "." + std::string(form_id) + ".2d";
  return {proj_id, std::move(frames), std::move(sheet), std::move(animations),
          std::move(channels), rig, std::move(shapes), std::move(windows), 4};
}

Projection25dDefinition synthesize_projection25d(std::string_view entity_id, std::string_view form_id,
                                                  [[maybe_unused]] const SynthesisPalette& palette,
                                                  [[maybe_unused]] const RigDefinition& rig) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  Projection25dDefinition proj;
  proj.id = pf + ".25d";
  proj.representation = RepresentationKind::two_point_five_d;
  proj.billboard = BillboardMode::camera_facing;
  proj.planes = {
    {pf + ".body", pf + ".body", std::nullopt, std::nullopt, 0, 100000, 0, false, "torso"},
    {pf + ".front", pf + ".front", std::nullopt, std::nullopt, 50, 0, 0, false, std::nullopt},
  };
  proj.views = {
    {pf + ".front", 0, false, std::nullopt, {
      {pf + ".body", true, std::nullopt, std::nullopt},
      {pf + ".front", true, std::nullopt, std::nullopt},
    }},
  };
  proj.geometry = {};
  proj.collisions = {
    {pf + ".body", pf + ".body", -16, -16, 16, 16, -10, 10},
  };
  return proj;
}

Projection25dDefinition synthesize_projection25d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    [[maybe_unused]] const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  Projection25dDefinition proj;
  proj.id = pf + ".25d";
  proj.representation = RepresentationKind::two_point_five_d;
  proj.billboard = BillboardMode::camera_facing;

  // Build a plane for each morphology part, z-sorted
  struct ZPart { std::string name; MorphologyPart part; std::int64_t z; };
  std::vector<ZPart> sorted;
  for (const auto& [name, part] : morphology)
    sorted.push_back({name, part, static_cast<std::int64_t>(part.z * 1000)});
  std::ranges::sort(sorted, [](const auto& a, const auto& b) { return a.z < b.z; });

  for (const auto& zp : sorted) {
    const std::string pid = pf + ".plane." + zp.name;
    // Rig bone to attach to (map morphology part to bone)
    std::optional<std::string> rig_node;
    if (zp.name == "torso") rig_node = pf + ".spine";
    else if (zp.name == "head" || zp.name == "left_ear" || zp.name == "right_ear" || zp.name == "muzzle" || zp.name.find("eye") != std::string::npos)
      rig_node = pf + ".head";
    else if (zp.name.find("leg") != std::string::npos || zp.name.find("paw") != std::string::npos)
      rig_node = pf + "." + zp.name;
    else if (zp.name == "tail") rig_node = pf + ".tail";
    else if (zp.name == "aura") rig_node = pf + ".spine";
    // Parallax: closer objects (more +Z) move more with camera
    const std::int32_t parallax = static_cast<std::int32_t>(std::clamp(zp.z / 10, 0LL, 100000LL));
    proj.planes.push_back({pid, pid, std::nullopt, std::nullopt,
                           static_cast<std::int32_t>(zp.z), parallax, 0, false, rig_node});
  }

  // 8 angular views (N, NE, E, SE, S, SW, W, NW)
  for (int angle = 0; angle < 360; angle += 45) {
    std::string vid = pf + ".view." + std::to_string(angle);
    std::vector<ViewPlaneProjection> view_planes;
    // Project planes in reverse order (front-to-back for rendering, back-to-front in list is fine)
    for (const auto& zp : sorted)
      view_planes.push_back({pf + ".plane." + zp.name, true, std::nullopt, std::nullopt});
    proj.views.push_back({vid, static_cast<std::uint32_t>(angle * 1000), true, std::nullopt, std::move(view_planes)});
  }

  // Collision volume based on torso
  if (auto it = morphology.find("torso"); it != morphology.end()) {
    const auto& t = it->second;
    proj.collisions.push_back({pf + ".body", pf + ".plane.torso",
      static_cast<std::int32_t>(-t.size_x * 500), static_cast<std::int32_t>(-t.size_y * 500),
      static_cast<std::int32_t>(t.size_x * 500), static_cast<std::int32_t>(t.size_y * 500),
      static_cast<std::int32_t>(-t.size_z * 500), static_cast<std::int32_t>(t.size_z * 500)});
  }

  return proj;
}

static std::uint32_t hex_color(std::string_view hex) {
  if (hex.empty() || hex[0] != '#') return 0xFFFFFFFF;
  auto nyb = [](char c) -> std::uint32_t {
    if (c >= '0' && c <= '9') return static_cast<std::uint32_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::uint32_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<std::uint32_t>(c - 'A' + 10);
    return 0;
  };
  return (nyb(hex[1]) << 28) | (nyb(hex[2]) << 24) | (nyb(hex[3]) << 20) | (nyb(hex[4]) << 16) | (nyb(hex[5]) << 12) | (nyb(hex[6]) << 8) | 0xFF;
}

Projection2dDefinition synthesize_projection2d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const RigDefinition& rig) {
  return synthesize_morphology_projection2d(entity_id, form_id, palette, morphology, rig);
}

Projection2dDefinition synthesize_morphology_projection2d(
    std::string_view entity_id, std::string_view form_id,
    const SynthesisPalette& palette,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const RigDefinition& rig,
    std::span<const SkeletalClip> clips) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  // Canvas size: 128x128 for legacy compatibility scaling to match expected package dimensions
  constexpr std::int32_t canvas_w = 128, canvas_h = 128;
  // Draw rotated ellipse: transform pixel coords back into ellipse-local space, then check containment
  auto draw_rotated_ellipse = [](ImageRgba8& img, int cx, int cy, int rx, int ry,
                                   double rotation_deg, double scale_x, double scale_y,
                                   std::uint32_t rgba) {
    std::uint8_t r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
    double rad = -rotation_deg * 3.141592653589793 / 180.0;
    double cos_r = std::cos(rad), sin_r = std::sin(rad);
    int erx = (std::max)(static_cast<int>(rx * scale_x), 1);
    int ery = (std::max)(static_cast<int>(ry * scale_y), 1);
    int bb = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(erx)*erx + static_cast<double>(ery)*ery))) + 1;
    for (int dy = -bb; dy <= bb; ++dy) {
      for (int dx = -bb; dx <= bb; ++dx) {
        // Rotate pixel offset back into ellipse-local space
        double lx = dx * cos_r - dy * sin_r;
        double ly = dx * sin_r + dy * cos_r;
        if (lx*lx / (erx*erx) + ly*ly / (ery*ery) <= 1.0) {
          int px = cx + dx, py = cy + dy;
          if (px >= 0 && px < (int)img.width && py >= 0 && py < (int)img.height) {
            std::size_t idx = (static_cast<std::size_t>(py) * img.width + static_cast<std::size_t>(px)) * 4;
            img.pixels[idx] = r; img.pixels[idx+1] = g; img.pixels[idx+2] = b; img.pixels[idx+3] = a;
          }
        }
      }
    }
  };

  // Resolve part color: use part's own color, fallback to palette based on emissive/electrical flags
  auto resolve_color = [&](const MorphologyPart& part, bool is_eye_or_ear) -> std::uint32_t {
    if (!part.color.empty() && part.color[0] == '#' && part.color.size() == 7) {
      return hex_color(part.color);
    }
    if (part.emissive || part.electrical_marking || is_eye_or_ear)
      return palette.accent;
    return palette.primary;
  };

  // Find a clip: try form-prefixed exact match first, then substring fallback
  auto find_clip = [&](std::string_view name) -> const SkeletalClip* {
    std::string full = std::string(form_id) + "_" + std::string(name);
    for (const auto& c : clips) {
      if (c.id == full) return &c;
    }
    // Fallback: substring match (for transform clips like "transform_ascend")
    for (const auto& c : clips) {
      if (c.id.find(name) != std::string::npos) return &c;
    }
    return nullptr;
  };

  // Build sorted part list: sort by z-depth (back-to-front: lower z drawn first)
  std::vector<std::pair<std::string, MorphologyPart>> sorted_parts;
  for (const auto& [name, part] : morphology)
    sorted_parts.push_back({name, part});
  std::ranges::sort(sorted_parts, [](const auto& a, const auto& b) {
    return a.second.z < b.second.z;
  });

  // Render a single frame using the evaluated pose for a specific clip+tick
  auto render_frame_posed = [&](const EvaluatedPose& pose) -> ImageRgba8 {
    ImageRgba8 canvas(canvas_w, canvas_h, ColorSpace::srgb, AlphaMode::straight,
                      std::vector<std::uint8_t>(static_cast<std::size_t>(canvas_w) * canvas_h * 4, 0));
    // Draw capsule: a line segment with width (rounded rectangle)
    auto draw_capsule = [&](int cx, int cy, int len, int width, double rotation_deg, std::uint32_t rgba) {
      std::uint8_t r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
      double rad = rotation_deg * 3.141592653589793 / 180.0;
      double cos_r = std::cos(rad), sin_r = std::sin(rad);
      int half_w = std::max(width / 2, 1);
      int half_len = std::max(len / 2, 1);
      int bb = std::max(half_len, half_w) + 2;
      for (int dy = -bb; dy <= bb; ++dy) {
        for (int dx = -bb; dx <= bb; ++dx) {
          double lx = dx * cos_r + dy * (-sin_r);
          double ly = dx * sin_r + dy * cos_r;
          // Check if point lies within the capsule (rounded rectangle)
          double dx_rect = std::max(std::abs(lx) - (half_len - half_w), 0.0);
          if (dx_rect * dx_rect + ly * ly <= half_w * half_w) {
            int px = cx + dx, py = cy + dy;
            if (px >= 0 && px < canvas_w && py >= 0 && py < canvas_h) {
              std::size_t idx = (static_cast<std::size_t>(py) * canvas_w + static_cast<std::size_t>(px)) * 4;
              canvas.pixels[idx] = r; canvas.pixels[idx+1] = g; canvas.pixels[idx+2] = b; canvas.pixels[idx+3] = a;
            }
          }
        }
      }
    };
    // Draw triangle: isosceles pointing up (rotation applied)
    auto draw_triangle = [&](int cx, int cy, int w, int h, double rotation_deg, std::uint32_t rgba) {
      std::uint8_t r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
      double rad = rotation_deg * 3.141592653589793 / 180.0;
      double cos_r = std::cos(rad), sin_r = std::sin(rad);
      int bb = std::max(w, h) / 2 + 2;
      double hw = w * 0.5, hh = h * 0.5;
      for (int dy = -bb; dy <= bb; ++dy) {
        for (int dx = -bb; dx <= bb; ++dx) {
          double lx = dx * cos_r + dy * (-sin_r);
          double ly = dx * sin_r + dy * cos_r;
          // Point-in-triangle: apex at (0,-hh), base at (hw,hh) and (-hw,hh)
          double ax = 0, ay = -hh;
          double bx = -hw, by = hh;
          double cx2 = hw, cy2 = hh;
          double d1 = (lx - bx) * (ay - by) - (ax - bx) * (ly - by);
          double d2 = (lx - cx2) * (by - cy2) - (bx - cx2) * (ly - cy2);
          double d3 = (lx - ax) * (cy2 - ay) - (cx2 - ax) * (ly - ay);
          bool neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
          bool pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
          if (!(neg && pos)) {
            int px = cx + dx, py = cy + dy;
            if (px >= 0 && px < canvas_w && py >= 0 && py < canvas_h) {
              std::size_t idx = (static_cast<std::size_t>(py) * canvas_w + static_cast<std::size_t>(px)) * 4;
              canvas.pixels[idx] = r; canvas.pixels[idx+1] = g; canvas.pixels[idx+2] = b; canvas.pixels[idx+3] = a;
            }
          }
        }
      }
    };
    // Draw segmented curve: series of connected circles
    auto draw_segmented_curve = [&](int cx, int cy, int len, int segments, double rotation_deg, std::uint32_t rgba) {
      std::uint8_t r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
      double rad = rotation_deg * 3.141592653589793 / 180.0;
      double cos_r = std::cos(rad), sin_r = std::sin(rad);
      int sr = std::max(len / (segments * 2), 1);
      for (int seg = 0; seg < segments; ++seg) {
        double t = static_cast<double>(seg) / (segments - 1) - 0.5;
        double sx = cx + t * len * cos_r;
        double sy = cy + t * len * sin_r;
        int scx = static_cast<int>(sx), scy = static_cast<int>(sy);
        for (int dy = -sr; dy <= sr; ++dy) {
          for (int dx = -sr; dx <= sr; ++dx) {
            if (dx*dx + dy*dy <= sr*sr) {
              int px = scx + dx, py = scy + dy;
              if (px >= 0 && px < canvas_w && py >= 0 && py < canvas_h) {
                std::size_t idx = (static_cast<std::size_t>(py) * canvas_w + static_cast<std::size_t>(px)) * 4;
                canvas.pixels[idx] = r; canvas.pixels[idx+1] = g; canvas.pixels[idx+2] = b; canvas.pixels[idx+3] = a;
              }
            }
          }
        }
      }
    };
    for (const auto& [name, part] : sorted_parts) {
      const bool is_eye_or_ear = (name.find("eye") != std::string::npos ||
                                   name.find("ear") != std::string::npos);
      std::uint32_t color = resolve_color(part, is_eye_or_ear);
      // Bone attachment: require explicit bone_id (no name-based fallback)
      std::string target_bone = part.bone_id;
      auto wit = pose.world.find(target_bone);
      // Bone world transform (identity if no bone found)
      double bx = 0, by = 0, brot = 0, bsx = 1.0, bsy = 1.0;
      if (wit != pose.world.end()) {
        bx = wit->second.x; by = wit->second.y;
        brot = wit->second.rotation_degrees;
        bsx = wit->second.scale_x; bsy = wit->second.scale_y;
      }
      // Affine composition: part center = bone_world × part_local_offset
      // The part's (x, y) is the local offset from its bone origin
      // Bone rotation rotates this offset; bone scale scales it
      double rad = brot * 3.141592653589793 / 180.0;
      double cos_r = std::cos(rad), sin_r = std::sin(rad);
      double wx = bx + part.x * cos_r * bsx - part.y * sin_r * bsy;
      double wy = by + part.x * sin_r * bsx + part.y * cos_r * bsy;
      const int pcx = canvas_w / 2 + static_cast<int>(wx * 3);
      const int pcy = canvas_h / 2 - static_cast<int>(wy * 3);
      const int prx = (std::max)(static_cast<int>(part.size_x * 1.5), 2);
      const int pry = (std::max)(static_cast<int>(part.size_y * 1.5), 2);
      double total_rot = brot + part.rotation_degrees;
      // Select primitive based on part.primitive field
      if (part.primitive == "capsule") {
        draw_capsule(pcx, pcy, pry * 2, prx, total_rot, color);
      } else if (part.primitive == "triangle") {
        draw_triangle(pcx, pcy, prx * 2, pry * 2, total_rot, color);
      } else if (part.primitive == "segmented_curve") {
        draw_segmented_curve(pcx, pcy, pry * 3, 6, total_rot, color);
      } else if (part.primitive == "aura_contour") {
        // Aura: large ellipse at reduced alpha
        std::uint32_t aura_color = (color & 0x00FFFFFF) | ((color & 0xFF) / 3);
        draw_rotated_ellipse(canvas, pcx, pcy, prx * 2, pry * 2, total_rot, bsx, bsy, aura_color);
      } else {
        // Default: ellipse
        draw_rotated_ellipse(canvas, pcx, pcy, prx, pry, total_rot, bsx, bsy, color);
      }
    }
    return canvas;
  };

  std::vector<FrameSource> frames;

  // Generate frames for a specific clip: evaluate pose at evenly-spaced ticks
  auto gen_clip_frames = [&](const SkeletalClip& clip, std::uint32_t count,
                              std::string_view clip_label, std::uint32_t frame_dur) {
    for (std::uint32_t i = 0; i < count; ++i) {
      std::uint32_t tick = clip.duration_ticks > 0 ? (i * clip.duration_ticks / count) : i;
      auto pose_result = evaluate_pose(clip, rig, tick);
      if (pose_result.ok()) {
        frames.push_back({pf + "." + std::string(clip_label) + "." + std::to_string(i),
                          render_frame_posed(*pose_result.value),
                          canvas_w / 2, canvas_h / 2, frame_dur});
      }
    }
  };

  // Find canonical clips
  auto idle_clip = find_clip("idle");
  auto walk_clip = find_clip("locomotion");
  auto attack_clip = find_clip("attack");
  auto hit_clip = find_clip("hit");
  auto transform_clip = find_clip("transform");

  // Generate frames: 4 idle, 6 walk, 6 attack, 3 hit, 5 transform = 24 base
  // + 4 storm_idle, 6 storm_walk, 6 storm_attack, 3 storm_hit = 19 storm = 43 total
  if (idle_clip) gen_clip_frames(*idle_clip, 4, "idle", 4);
  else {
    for (int i = 0; i < 4; ++i) frames.push_back({pf + ".idle." + std::to_string(i),
      ImageRgba8(canvas_w, canvas_h, ColorSpace::srgb, AlphaMode::straight,
                 std::vector<std::uint8_t>(static_cast<std::size_t>(canvas_w) * canvas_h * 4, 0)),
      canvas_w/2, canvas_h/2, 4});
  }
  if (walk_clip) gen_clip_frames(*walk_clip, 6, "walk", 3);
  if (attack_clip) gen_clip_frames(*attack_clip, 6, "attack", 2);
  if (hit_clip) gen_clip_frames(*hit_clip, 3, "hit", 2);
  if (transform_clip) gen_clip_frames(*transform_clip, 10, "transform", 2);

  for (auto& f : frames) f.frame_hash = compute_frame_hash(f.image);

  // Compile sprite sheet
  SpriteSheetOptions opts{1024, 2048, 2, false, 0};
  auto sheet = compile_sprite_sheet(frames, opts);

  // Animation clips referencing the frame IDs
  std::vector<AnimationClip> anims = {
    {pf + ".idle", {pf + ".idle.0", pf + ".idle.1", pf + ".idle.2", pf + ".idle.3"}, {4, 4, 4, 4}, {}, true},
    {pf + ".walk", {pf + ".walk.0", pf + ".walk.1", pf + ".walk.2", pf + ".walk.3", pf + ".walk.4", pf + ".walk.5"}, {3, 3, 3, 3, 3, 3}, {}, true},
    {pf + ".attack", {pf + ".attack.0", pf + ".attack.1", pf + ".attack.2", pf + ".attack.3", pf + ".attack.4", pf + ".attack.5"}, {2, 2, 1, 2, 2, 2}, {{"release", 2}}, false},
    {pf + ".hit", {pf + ".hit.0", pf + ".hit.1", pf + ".hit.2"}, {1, 2, 2}, {}, false},
    {pf + ".transform", {pf + ".transform.0", pf + ".transform.1", pf + ".transform.2", pf + ".transform.3", pf + ".transform.4"}, {2, 2, 2, 2, 2}, {{"midpoint", 2}}, false},
  };

  // Channel maps (depth) — fully opaque alpha required
  auto depth = ImageRgba8{canvas_w, canvas_h, ColorSpace::data, AlphaMode::opaque,
                          std::vector<std::uint8_t>(static_cast<std::size_t>(canvas_w) * canvas_h * 4, 0)};
  for (std::size_t i = 0; i < depth.pixels.size(); i += 4) {
    depth.pixels[i] = depth.pixels[i+1] = depth.pixels[i+2] = 64;
    depth.pixels[i+3] = 255;
  }
  std::vector<ChannelMap> channels = {
    {pf + ".idle.0.depth", pf + ".idle.0", ChannelMapKind::depth, depth},
  };

  // Collision shapes from morphology — map parts to rig bones
  std::vector<CollisionShape> shapes;
  for (const auto& [name, part] : morphology) {
    // Determine bone attachment: map part names to rig bone IDs
    std::string bone_id;
    if (name == "torso" || name == "root" || name == "aura")
      bone_id = "root";
    else if (name == "head" || name == "muzzle" || name.find("eye") != std::string::npos || name.find("ear") != std::string::npos)
      bone_id = "head";
    else if (name.find("leg") != std::string::npos || name.find("hind") != std::string::npos)
      bone_id = "root";
    else if (name == "tail")
      bone_id = "tail";
    else
      bone_id = "root";
    shapes.push_back({name, CollisionKind::axis_aligned_box, bone_id,
                      part.x, part.y, part.size_x, part.size_y});
  }

  std::vector<CollisionWindow> windows;
  return Projection2dDefinition{std::string(pf) + ".2d", std::move(frames), std::move(sheet), std::move(anims),
                                std::move(channels), rig, std::move(shapes), std::move(windows), 4};
}

Projection3dDefinition synthesize_projection3d(std::string_view entity_id, std::string_view form_id,
                                                [[maybe_unused]] const SynthesisPalette& palette,
                                                [[maybe_unused]] const RigDefinition& rig) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  Projection3dDefinition proj;
  proj.id = pf + ".3d";
  proj.materials = {
    {pf + ".body", palette.primary, 0, 900000, {}, {}, {}, std::nullopt, std::nullopt, std::nullopt},
    {pf + ".accent", palette.accent, 0, 900000, {}, {}, {}, std::nullopt, std::nullopt, std::nullopt},
  };
  auto v = [](float x, float y, float z) -> Vertex3d {
    return {{static_cast<std::int64_t>(x * 1000), static_cast<std::int64_t>(y * 1000), static_cast<std::int64_t>(z * 1000)},
            {0, 0, 1000000}, {0, 0}, {}};
  };
  proj.meshes = {
    {pf + ".body", MeshPurpose::render, pf + ".body", false, {v(-0.5f, -0.5f, 0), v(0.5f, -0.5f, 0), v(0, 0.5f, 0)}, {0, 2, 1}},
    {pf + ".head", MeshPurpose::render, pf + ".accent", false, {v(-0.3f, -2.0f, 0), v(0.3f, -2.0f, 0), v(0, -2.6f, 0)}, {0, 2, 1}},
  };
  proj.skeleton = std::nullopt;
  proj.morph_targets = {};
  proj.lods = {};
  proj.limits = Projection3dLimits{};
  return proj;
}

TransformationManifestationProgram make_manifestation2d(std::string_view entity_id,
                                                        [[maybe_unused]] const RigDefinition& rig) {
  std::string eid = std::string(entity_id);
  return {eid + ".manifest.2d",
    {{"base", "base", eid + ".base.2d"}, {"storm", "storm", eid + ".storm.2d"}},
    {{"ascend", "ascend"}, {"descend", "descend"}}};
}

TransformationManifestationProgram make_manifestation25d(std::string_view entity_id,
                                                         [[maybe_unused]] const RigDefinition& rig) {
  std::string eid = std::string(entity_id);
  return {eid + ".manifest.25d",
    {{"base", "base", eid + ".base.25d"}, {"storm", "storm", eid + ".storm.25d"}},
    {{"ascend", "ascend"}, {"descend", "descend"}}};
}

TransformationManifestationProgram make_manifestation3d(std::string_view entity_id,
                                                        [[maybe_unused]] const RigDefinition& rig) {
  std::string eid = std::string(entity_id);
  return {eid + ".manifest.3d",
    {{"base", "base", eid + ".base.3d"}, {"storm", "storm", eid + ".storm.3d"}},
    {{"ascend", "ascend"}, {"descend", "descend"}}};
}

SynthesisPalette make_palette(std::string_view primary_hex, std::string_view accent_hex) {
  return {hex_to_u32(primary_hex),
          ((hex_to_u32(primary_hex) >> 1) & 0x7F7F7F7F) | 0x80808080,
          hex_to_u32(accent_hex),
          0x202020FF,
          0x00000000};
}

SynthesisResult synthesize_unified_entity(std::string_view entity_id,
                                          const SynthesisPalette& base_palette,
                                          const SynthesisPalette& transformed_palette) {
  SynthesisResult result;
  RigDefinition rig = make_biped_rig(entity_id);

  result.proj2d_base = synthesize_projection2d(entity_id, "base", base_palette, rig);
  result.proj2d_transformed = synthesize_projection2d(entity_id, "storm", transformed_palette, rig);
  result.proj25d_base = synthesize_projection25d(entity_id, "base", base_palette, rig);
  result.proj25d_transformed = synthesize_projection25d(entity_id, "storm", transformed_palette, rig);
  result.proj3d_base = synthesize_projection3d(entity_id, "base", base_palette, rig);
  result.proj3d_transformed = synthesize_projection3d(entity_id, "storm", transformed_palette, rig);

  result.manifest2d = make_manifestation2d(entity_id, rig);
  result.manifest25d = make_manifestation25d(entity_id, rig);
  result.manifest3d = make_manifestation3d(entity_id, rig);

  return result;
}

SynthesisResult synthesize_unified_entity(const SpriteIr& ir) {
  SynthesisPalette base_pal = make_palette(ir.primary_color, ir.accent_color);
  SynthesisPalette storm_pal = make_palette(ir.accent_color, ir.primary_color);
  RigDefinition rig = make_rig_from_ir(ir);
  auto result = synthesize_unified_entity(ir.entity_id, base_pal, storm_pal);
  result.proj2d_base.collision_shapes = ir.collision_shapes;
  result.proj2d_base.collision_windows = ir.collision_windows;
  result.proj2d_base.rig = rig;
  result.proj2d_transformed.collision_shapes = ir.collision_shapes;
  result.proj2d_transformed.collision_windows = ir.collision_windows;
  result.proj2d_transformed.rig = rig;
  // Replace 3D projections with Voltfox morphology-driven meshes if morphology is present
  if (!ir.morphology.empty()) {
    result.proj25d_base = synthesize_projection25d_voltfox(ir.entity_id, "base", base_pal, ir.morphology);
    result.proj25d_transformed = synthesize_projection25d_voltfox(ir.entity_id, "storm", storm_pal, ir.morphology);
    result.proj3d_base = synthesize_projection3d_voltfox(ir.entity_id, "base", base_pal, ir.morphology);
    result.proj3d_transformed = synthesize_projection3d_voltfox(ir.entity_id, "storm", storm_pal, ir.morphology);
    result.proj2d_base = synthesize_projection2d_voltfox(ir.entity_id, "base", base_pal, ir.morphology, rig);
    result.proj2d_transformed = synthesize_projection2d_voltfox(ir.entity_id, "storm", storm_pal, ir.morphology, rig);
    result.animations3d = synthesize_animation3d_voltfox(ir.entity_id, "base", ir.morphology, ir.clips, ir.animation_intents);
  }

  // Enforce synthesis-level resource limits
  {
    ResourceLimits rl;
    auto limit_check = [&](bool ok, const char* code, const std::string& msg) {
      if (!ok) throw std::runtime_error(std::string(code) + ": " + msg);
    };
    limit_check(result.proj2d_base.source_frames.size() <= rl.max_frames, "RESOURCE_FRAMES",
        "frames count " + std::to_string(result.proj2d_base.source_frames.size()) + " exceeds maximum " + std::to_string(rl.max_frames));
    for (const auto& f : result.proj2d_base.source_frames) {
      limit_check(f.image.width <= rl.max_frame_width, "RESOURCE_FRAME_WIDTH",
          "frame " + f.id + " width " + std::to_string(f.image.width) + " exceeds maximum " + std::to_string(rl.max_frame_width));
      limit_check(f.image.height <= rl.max_frame_height, "RESOURCE_FRAME_HEIGHT",
          "frame " + f.id + " height " + std::to_string(f.image.height) + " exceeds maximum " + std::to_string(rl.max_frame_height));
    }
    limit_check(result.proj25d_base.planes.size() <= rl.max_25d_planes, "RESOURCE_25D_PLANES",
        "2.5D planes count " + std::to_string(result.proj25d_base.planes.size()) + " exceeds maximum " + std::to_string(rl.max_25d_planes));
    std::size_t vertex_count = 0;
    for (const auto& m : result.proj3d_base.meshes) vertex_count += m.vertices.size();
    limit_check(vertex_count <= rl.max_vertices, "RESOURCE_VERTICES",
        "vertices count " + std::to_string(vertex_count) + " exceeds maximum " + std::to_string(rl.max_vertices));
  }

  return result;
}

namespace {

// ---- 3D primitive helpers for Voltfox mesh generation ----

struct V3 { std::int64_t x, y, z; };
V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

// Build a box (cuboid) centered at origin with given half-extents, returns vertices + indices
void add_box(std::vector<Vertex3d>& verts, std::vector<std::uint32_t>& idx, V3 half, std::string_view joint_id, std::uint32_t weight) {
  // 8 corners of the box
  const V3 corners[8] = {
    {-half.x, -half.y, -half.z}, { half.x, -half.y, -half.z}, { half.x,  half.y, -half.z}, {-half.x,  half.y, -half.z},
    {-half.x, -half.y,  half.z}, { half.x, -half.y,  half.z}, { half.x,  half.y,  half.z}, {-half.x,  half.y,  half.z}
  };
  // 6 face normals
  const Normal3Ppm norms[6] = {{0,0,-1000000},{0,0,1000000},{0,-1000000,0},{0,1000000,0},{-1000000,0,0},{1000000,0,0}};
  // face vertex quads (CCW winding)
  const int faces[6][4] = {{0,1,2,3},{4,6,5,7},{0,4,5,1},{2,3,7,6},{0,3,7,4},{1,5,6,2}};
  const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
  for (int fi = 0; fi < 6; ++fi) {
    for (int vi = 0; vi < 4; ++vi) {
      const auto& c = corners[faces[fi][vi]];
      verts.push_back({{c.x, c.y, c.z}, norms[fi], {0, 0}, {{std::string(joint_id), weight}}});
    }
    const std::uint32_t f = base + static_cast<std::uint32_t>(fi * 4);
    idx.push_back(f); idx.push_back(f+1); idx.push_back(f+2);
    idx.push_back(f); idx.push_back(f+2); idx.push_back(f+3);
  }
}

// Build a sphere approximated with 3 rings + poles, returns vertices + indices
[[maybe_unused]] void add_sphere(std::vector<Vertex3d>& verts, std::vector<std::uint32_t>& idx, V3 center, std::int64_t r, std::string_view joint_id, std::uint32_t weight) {
  constexpr int rings = 4, sectors = 6;
  const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
  // North pole
  verts.push_back({{center.x, center.y + r, center.z}, {0, 1000000, 0}, {0, 0}, {{std::string(joint_id), weight}}});
  for (int ri = 1; ri < rings; ++ri) {
    const double theta = 3.1415926535 * ri / rings;
    const double sy = std::cos(theta), sr = std::sin(theta);
    for (int si = 0; si < sectors; ++si) {
      const double phi = 2.0 * 3.1415926535 * si / sectors;
      const V3 norm{static_cast<std::int64_t>(sr * std::cos(phi) * 1000000),
                    static_cast<std::int64_t>(sy * 1000000),
                    static_cast<std::int64_t>(sr * std::sin(phi) * 1000000)};
      verts.push_back({{center.x + static_cast<std::int64_t>(r * sr * std::cos(phi)),
                         center.y + static_cast<std::int64_t>(r * sy),
                         center.z + static_cast<std::int64_t>(r * sr * std::sin(phi))},
                        {static_cast<std::int32_t>(std::abs(norm.x) > 1000 ? norm.x : 0),
                         static_cast<std::int32_t>(std::abs(norm.y) > 1000 ? norm.y : 0),
                         static_cast<std::int32_t>(std::abs(norm.z) > 1000 ? norm.z : 0)},
                       {0, 0}, {{std::string(joint_id), weight}}});
    }
  }
  // South pole
  verts.push_back({{center.x, center.y - r, center.z}, {0, -1000000, 0}, {0, 0}, {{std::string(joint_id), weight}}});
  const std::uint32_t south = base + 1 + static_cast<std::uint32_t>((rings - 1) * sectors);
  // Top cap
  for (int si = 0; si < sectors; ++si) {
    const int ns = (si + 1) % sectors;
    idx.push_back(base); idx.push_back(base + 1 + si); idx.push_back(base + 1 + ns);
  }
  // Rings
  for (int ri = 0; ri < rings - 2; ++ri) {
    for (int si = 0; si < sectors; ++si) {
      const int ns = (si + 1) % sectors;
      const std::uint32_t a = base + 1 + ri * sectors + si;
      const std::uint32_t b = base + 1 + ri * sectors + ns;
      const std::uint32_t c = base + 1 + (ri + 1) * sectors + si;
      const std::uint32_t d = base + 1 + (ri + 1) * sectors + ns;
      idx.push_back(a); idx.push_back(b); idx.push_back(d);
      idx.push_back(a); idx.push_back(d); idx.push_back(c);
    }
  }
  // Bottom cap
  for (int si = 0; si < sectors; ++si) {
    const int ns = (si + 1) % sectors;
    idx.push_back(south); idx.push_back(base + 1 + (rings - 2) * sectors + ns); idx.push_back(base + 1 + (rings - 2) * sectors + si);
  }
}

// Build a cone (cone approximated as pyramid with n sides), returns vertices + indices
[[maybe_unused]] void add_cone(std::vector<Vertex3d>& verts, std::vector<std::uint32_t>& idx, V3 tip, V3 base_center, std::int64_t base_r, std::string_view joint_id, std::uint32_t weight) {
  constexpr int sides = 6;
  const std::uint32_t b = static_cast<std::uint32_t>(verts.size());
  // Tip
  verts.push_back({{tip.x, tip.y, tip.z}, {0, 1000000, 0}, {0, 0}, {{std::string(joint_id), weight}}});
  // Base ring
  for (int si = 0; si < sides; ++si) {
    const double phi = 2.0 * 3.1415926535 * si / sides;
    verts.push_back({{base_center.x + static_cast<std::int64_t>(base_r * std::cos(phi)),
                      base_center.y,
                      base_center.z + static_cast<std::int64_t>(base_r * std::sin(phi))},
                     {0, -1000000, 0}, {0, 0}, {{std::string(joint_id), weight}}});
  }
  // Cone sides
  for (int si = 0; si < sides; ++si) {
    const int ns = (si + 1) % sides;
    idx.push_back(b); idx.push_back(b + 1 + ns); idx.push_back(b + 1 + si);
  }
}

// Build a capsule (cylinder + 2 hemispheres)
[[maybe_unused]] void add_capsule(std::vector<Vertex3d>& verts, std::vector<std::uint32_t>& idx, V3 start, V3 end, std::int64_t r, std::string_view joint_id, std::uint32_t weight) {
  // Simple cylinder with hemisphere caps approximated as 3 ring segments
  constexpr int segs = 6; // longitudinal segments
  const V3 dir = end - start;
  const double len = std::sqrt(static_cast<double>(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z));
  if (len < 1) { add_sphere(verts, idx, start, r, joint_id, weight); return; }
  // Build cylinder body with rings
  constexpr int nrings = 3;
  for (int ri = 0; ri <= nrings; ++ri) {
    const double t = static_cast<double>(ri) / nrings;
    const V3 p = {start.x + static_cast<std::int64_t>(t * dir.x),
                  start.y + static_cast<std::int64_t>(t * dir.y),
                  start.z + static_cast<std::int64_t>(t * dir.z)};
    for (int si = 0; si < segs; ++si) {
      const double phi = 2.0 * 3.1415926535 * si / segs;
      verts.push_back({{p.x + static_cast<std::int64_t>(r * std::cos(phi)),
                        p.y,
                        p.z + static_cast<std::int64_t>(r * std::sin(phi))},
                       {static_cast<std::int32_t>(std::cos(phi) * 1000000), 0,
                        static_cast<std::int32_t>(std::sin(phi) * 1000000)},
                       {0, 0}, {{std::string(joint_id), weight}}});
    }
  }
  const std::uint32_t nc = static_cast<std::uint32_t>(verts.size());
  // Ring triangles
  for (int ri = 0; ri < nrings; ++ri) {
    for (int si = 0; si < segs; ++si) {
      const int ns = (si + 1) % segs;
      const std::uint32_t a = nc - (nrings + 1) * segs + ri * segs + si;
      const std::uint32_t b = nc - (nrings + 1) * segs + ri * segs + ns;
      const std::uint32_t c = nc - (nrings + 1) * segs + (ri + 1) * segs + si;
      const std::uint32_t d = nc - (nrings + 1) * segs + (ri + 1) * segs + ns;
      idx.push_back(a); idx.push_back(b); idx.push_back(d);
      idx.push_back(a); idx.push_back(d); idx.push_back(c);
    }
  }
  // Hemisphere caps via sphere helper at each end
  add_sphere(verts, idx, start, r, joint_id, weight);
  add_sphere(verts, idx, end, r, joint_id, weight);
}

// Build a cylinder
[[maybe_unused]] void add_cylinder(std::vector<Vertex3d>& verts, std::vector<std::uint32_t>& idx, V3 start, V3 end, std::int64_t r, std::string_view joint_id, std::uint32_t weight) {
  add_capsule(verts, idx, start, end, r, joint_id, weight);
}

} // anonymous namespace

// ---- 3D animation clip synthesis for Voltfox ----

std::vector<AnimationClip3d> synthesize_animation3d_voltfox(
    std::string_view entity_id, std::string_view form_id,
    const std::map<std::string, MorphologyPart, std::less<>>& morphology,
    const std::vector<SkeletalClip>& clips,
    std::span<const AnimationIntent> /*animation_intents*/) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  std::vector<AnimationClip3d> result;

  // Map 2D bone IDs to 3D joint IDs
  auto to3d_joint = [&](std::string_view bone_id) -> std::string {
    if (bone_id == "root") return pf + ".root";
    if (bone_id == "head") return pf + ".head";
    if (bone_id == "tail") return pf + ".tail";
    return pf + "." + std::string(bone_id);
  };

  // Convert 2D bone keyframes to 3D joint keyframes
  auto convert_track = [&](const BoneTrack& track) -> JointTrack3d {
    JointTrack3d jt;
    jt.joint_id = to3d_joint(track.bone_id);
    for (const auto& kf : track.keys) {
      JointPose3d pose;
      // Map 2D translation (pixels) to 3D translation (micrometers)
      pose.translation = {static_cast<std::int64_t>(kf.transform.x * 1000),
                          static_cast<std::int64_t>(kf.transform.y * 1000), 0};
      // Convert 2D rotation (degrees) to 3D quaternion (Z rotation)
      const double rad = kf.transform.rotation_degrees * 3.1415926535 / 180.0;
      const double half = rad * 0.5;
      pose.rotation_xyzw_ppm = {0, 0,
                                static_cast<std::int32_t>(std::sin(half) * 1'000'000),
                                static_cast<std::int32_t>(std::cos(half) * 1'000'000)};
      // Scale
      pose.scale_xyz_ppm = {static_cast<std::int32_t>(kf.transform.scale_x * 1'000'000),
                            static_cast<std::int32_t>(kf.transform.scale_y * 1'000'000),
                            1'000'000};
      jt.keys.push_back({kf.tick, pose});
    }
    return jt;
  };

  // Convert each SkeletalClip from the seed
  for (const auto& clip : clips) {
    AnimationClip3d anim;
    anim.id = pf + "." + clip.id;
    anim.ticks_per_second = 60;
    anim.duration_ticks = clip.duration_ticks;
    anim.looping = clip.looping;
    for (const auto& track : clip.tracks)
      anim.joint_tracks.push_back(convert_track(track));
    for (const auto& ev : clip.events)
      anim.events.push_back({ev.first, ev.second});
    result.push_back(std::move(anim));
  }

  // Generate procedural idle animation if not present
  bool has_idle = std::ranges::any_of(clips, [](const auto& c) { return c.id == "idle"; });
  if (!has_idle && !morphology.empty()) {
    AnimationClip3d idle;
    idle.id = pf + ".idle";
    idle.ticks_per_second = 60;
    idle.duration_ticks = 120;
    idle.looping = true;

    // Breathing motion: gentle oscillation of spine and head
    auto get_part = [&](std::string_view key) -> MorphologyPart {
      const auto it = morphology.find(std::string(key));
      return it != morphology.end() ? it->second : MorphologyPart{};
    };
    auto make_breath_track = [&](std::string_view joint_suffix,
                                  const MorphologyPart& part, std::int64_t amplitude) -> JointTrack3d {
      JointTrack3d jt;
      jt.joint_id = pf + "." + std::string(joint_suffix);
      const std::int64_t base_y = static_cast<std::int64_t>(part.y * 1000);
      for (std::uint32_t t = 0; t <= idle.duration_ticks; t += 10) {
        const double phase = 2.0 * 3.1415926535 * t / idle.duration_ticks;
        JointPose3d pose;
        pose.translation = {static_cast<std::int64_t>(part.x * 1000),
                            base_y + static_cast<std::int64_t>(amplitude * std::sin(phase)),
                            static_cast<std::int64_t>(part.z * 1000)};
        pose.rotation_xyzw_ppm = {0, 0, 0, 1'000'000};
        pose.scale_xyz_ppm = {1'000'000, 1'000'000, 1'000'000};
        jt.keys.push_back({t, pose});
      }
      return jt;
    };

    idle.joint_tracks.push_back(make_breath_track("root", get_part("torso"), 0));
    idle.joint_tracks.push_back(make_breath_track("spine", get_part("torso"), 500));
    idle.joint_tracks.push_back(make_breath_track("neck", get_part("head"), 300));
    idle.joint_tracks.push_back(make_breath_track("head", get_part("head"), 200));
    idle.joint_tracks.push_back(make_breath_track("tail", get_part("tail"), 1000));
    result.push_back(std::move(idle));
  }

  return result;
}

[[maybe_unused]] static std::uint32_t hex_rgba(std::string_view hex, std::uint8_t alpha = 255) {
  if (hex.size() != 7 || hex[0] != '#') return 0xFFFFFFFF;
  auto nybble = [](char c) -> std::uint32_t {
    if (c >= '0' && c <= '9') return static_cast<std::uint32_t>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<std::uint32_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<std::uint32_t>(c - 'A' + 10);
    return 0;
  };
  const std::uint32_t r = (nybble(hex[1]) << 4) | nybble(hex[2]);
  const std::uint32_t g = (nybble(hex[3]) << 4) | nybble(hex[4]);
  const std::uint32_t b = (nybble(hex[5]) << 4) | nybble(hex[6]);
  return (r << 24) | (g << 16) | (b << 8) | alpha;
}

Projection3dDefinition synthesize_projection3d_voltfox(std::string_view entity_id, std::string_view form_id,
                                                       const SynthesisPalette& palette,
                                                       const std::map<std::string, MorphologyPart, std::less<>>& morphology) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  Projection3dDefinition proj;
  proj.id = pf + ".3d";

  // ---- Materials from palette and morphology colors ----
  auto add_mat = [&](std::string_view id, std::uint32_t rgba) {
    proj.materials.push_back({std::string(id), rgba, 0, 800000, MaterialAlphaMode::opaque, 500000, false, std::nullopt, std::nullopt, std::nullopt});
  };
  add_mat(pf + ".body", palette.primary);
  add_mat(pf + ".accent", palette.accent);
  for (const auto& [part_name, part] : morphology) {
    if (!part.color.empty() && part.color != "#242038" && part.color != "#56F1FF" && part.color != "#FFFFFF") {
      add_mat(pf + ".mat." + part_name, hex_rgba(part.color));
    }
  }
  add_mat(pf + ".mat.aura", hex_rgba("#56F1FF", 80)); // semi-transparent aura

  // ---- Helper to get a morphology part with fallback ----
  auto get_part = [&](std::string_view key) -> MorphologyPart {
    const auto it = morphology.find(std::string(key));
    if (it != morphology.end()) return it->second;
    return {};
  };

  // ---- Skeleton: 13 bones for Voltfox quadruped ----
  // Coordinate convention: +x=right, +y=up, +z=forward (screen-right in typical game coords)
  // Sizes are based on morphology scaled to micrometers
  constexpr std::int64_t scale = 1000; // 1 micrometer per mm from morphology
  auto morph_sz = [&](const MorphologyPart& p) -> V3 {
    return {static_cast<std::int64_t>(p.size_x * scale / 2),
            static_cast<std::int64_t>(p.size_y * scale / 2),
            static_cast<std::int64_t>(p.size_z * scale / 2)};
  };

  // Bone indices
  enum BONE { B_ROOT = 0, B_SPINE, B_NECK, B_HEAD, B_LEFT_EAR, B_RIGHT_EAR, B_TAIL,
              B_LEFT_FRONT_LEG, B_RIGHT_FRONT_LEG, B_LEFT_HIND_LEG, B_RIGHT_HIND_LEG,
              B_LEFT_PAW, B_RIGHT_PAW, BONE_COUNT };

  const auto torso = get_part("torso");
  const auto head = get_part("head");
  const auto left_ear = get_part("left_ear");
  const auto right_ear = get_part("right_ear");
  const auto tail = get_part("tail");
  const auto left_front = get_part("left_front_leg");
  const auto right_front = get_part("right_front_leg");
  const auto aura = get_part("aura");

  Skeleton3d skel;
  skel.id = pf + ".skel";
  skel.joints.resize(BONE_COUNT);

  // Build skeleton from morphology positions
  auto set_joint = [&](int idx, std::string_view name, std::optional<int> parent, const MorphologyPart& part) {
    skel.joints[idx].id = std::string(name);
    if (parent) skel.joints[idx].parent_id = skel.joints[*parent].id;
    skel.joints[idx].translation = {static_cast<std::int64_t>(part.x * scale),
                                     static_cast<std::int64_t>(part.y * scale),
                                     static_cast<std::int64_t>(part.z * scale)};
  };

  set_joint(B_ROOT, pf + ".root", std::nullopt, torso);
  set_joint(B_SPINE, pf + ".spine", B_ROOT, torso);
  set_joint(B_NECK, pf + ".neck", B_SPINE, head);
  set_joint(B_HEAD, pf + ".head", B_NECK, head);
  set_joint(B_LEFT_EAR, pf + ".left_ear", B_HEAD, left_ear);
  set_joint(B_RIGHT_EAR, pf + ".right_ear", B_HEAD, right_ear);
  set_joint(B_TAIL, pf + ".tail", B_ROOT, tail);
  set_joint(B_LEFT_FRONT_LEG, pf + ".left_front_leg", B_SPINE, left_front);
  set_joint(B_RIGHT_FRONT_LEG, pf + ".right_front_leg", B_SPINE, right_front);
  set_joint(B_LEFT_HIND_LEG, pf + ".left_hind_leg", B_ROOT, get_part("left_front_leg")); // reuse front leg offset
  set_joint(B_RIGHT_HIND_LEG, pf + ".right_hind_leg", B_ROOT, get_part("right_front_leg"));
  set_joint(B_LEFT_PAW, pf + ".left_paw", B_LEFT_FRONT_LEG, left_front);
  set_joint(B_RIGHT_PAW, pf + ".right_paw", B_RIGHT_FRONT_LEG, right_front);

  // ---- Mesh assembly from morphology parts ----
  // We build one mesh per body part for clarity
  auto make_mesh = [&](std::string_view part_id, std::string_view mat_id) -> Mesh3d {
    Mesh3d m;
    m.id = std::string(part_id);
    m.purpose = MeshPurpose::render;
    m.material_id = std::string(mat_id);
    m.require_closed_manifold = false;
    return m;
  };

  // Helper: add part mesh — uses box primitive for all parts (simplified for validation)
  auto build_mesh = [&](std::string_view part_name, const MorphologyPart& part,
                         const char* mat_name, int bone_idx, std::uint32_t weight = 1000000) {
    if (part.size_x < 0.01 && part.size_y < 0.01 && part.size_z < 0.01) return;
    const std::string mesh_id = std::string(pf) + "." + std::string(part_name);
    const std::string mat_id = std::string(pf) + "." + mat_name;
    auto mesh = make_mesh(mesh_id, mat_id);
    const V3 hsize = morph_sz(part);
    add_box(mesh.vertices, mesh.triangle_indices, hsize, skel.joints[bone_idx].id, weight);
    if (!mesh.vertices.empty() && !mesh.triangle_indices.empty()) {
      proj.meshes.push_back(std::move(mesh));
    }
  };

  // Build each morphology part as a mesh
  // Map morphology parts to materials and bones
  struct PartMap { const char* name; const char* mat; int bone; };
  const PartMap part_map[] = {
    {"torso", "body", B_SPINE},
    {"head", "body", B_HEAD},
    {"left_ear", "accent", B_LEFT_EAR},
    {"right_ear", "accent", B_RIGHT_EAR},
    {"left_eye", "accent", B_HEAD},
    {"right_eye", "accent", B_HEAD},
    {"muzzle", "body", B_HEAD},
    {"tail", "body", B_TAIL},
    {"left_front_leg", "body", B_LEFT_FRONT_LEG},
    {"right_front_leg", "body", B_RIGHT_FRONT_LEG},
  };
  for (const auto& pm : part_map) {
    const auto it = morphology.find(pm.name);
    if (it != morphology.end()) {
      build_mesh(it->first, it->second, pm.mat, pm.bone);
    }
  }
  // Aura as separate mesh with semi-transparent material
  if (aura.size_x >= 1.0 || aura.size_y >= 1.0 || aura.size_z >= 1.0) {
    build_mesh("aura", aura, "mat.aura", B_SPINE);
  }

  // Move skeleton into projection (after mesh building which references skel by capture)
  proj.skeleton = std::move(skel);

  // ---- Morph targets ----
  proj.morph_targets = {};
  // ---- LODs ----
  proj.lods = {};
  // ---- Limits ----
  proj.limits = Projection3dLimits{};

  return proj;
}

ValidationResult enforce_resource_limits(const SpriteSeed& seed,
                                          const SynthesisResult& result,
                                          const ResourceLimits& limits) {
  ValidationResult res = enforce_resource_limits(seed, limits);
  auto add = [&](bool ok, std::string code, std::string msg) { if (!ok) res.diagnostics.push_back({std::move(code), std::move(msg)}); };
  add(result.proj2d_base.source_frames.size() <= limits.max_frames, "RESOURCE_FRAMES",
      "frames count " + std::to_string(result.proj2d_base.source_frames.size()) + " exceeds maximum " + std::to_string(limits.max_frames));
  for (const auto& f : result.proj2d_base.source_frames) {
    add(f.image.width <= limits.max_frame_width, "RESOURCE_FRAME_WIDTH",
        "frame " + f.id + " width " + std::to_string(f.image.width) + " exceeds maximum " + std::to_string(limits.max_frame_width));
    add(f.image.height <= limits.max_frame_height, "RESOURCE_FRAME_HEIGHT",
        "frame " + f.id + " height " + std::to_string(f.image.height) + " exceeds maximum " + std::to_string(limits.max_frame_height));
  }
  add(result.proj25d_base.planes.size() <= limits.max_25d_planes, "RESOURCE_25D_PLANES",
      "2.5D planes count " + std::to_string(result.proj25d_base.planes.size()) + " exceeds maximum " + std::to_string(limits.max_25d_planes));
  std::size_t vertex_count = 0;
  for (const auto& m : result.proj3d_base.meshes) vertex_count += m.vertices.size();
  add(vertex_count <= limits.max_vertices, "RESOURCE_VERTICES",
      "vertices count " + std::to_string(vertex_count) + " exceeds maximum " + std::to_string(limits.max_vertices));
  std::size_t ir_node_count = 0;
  const auto& irb = result.proj2d_base;
  ir_node_count += irb.source_frames.size() + irb.animations.size() + irb.channel_maps.size() + irb.collision_shapes.size() + irb.collision_windows.size();
  const auto& ir25 = result.proj25d_base;
  ir_node_count += ir25.planes.size() + ir25.views.size() + ir25.geometry.size() + ir25.collisions.size();
  const auto& ir3 = result.proj3d_base;
  for (const auto& m : ir3.meshes) ir_node_count += m.vertices.size() + m.triangle_indices.size();
  ir_node_count += ir3.materials.size() + ir3.morph_targets.size() + ir3.lods.size() + (ir3.skeleton ? ir3.skeleton->joints.size() : 0);
  add(ir_node_count <= limits.max_sprite_ir_nodes, "RESOURCE_SPRITE_IR_NODES",
      "Sprite IR node count " + std::to_string(ir_node_count) + " exceeds maximum " + std::to_string(limits.max_sprite_ir_nodes));
  return res;
}

std::uint32_t with_alpha(std::uint32_t rgba, std::uint8_t alpha) {
  return (rgba & 0xFFFFFF00u) | static_cast<std::uint32_t>(alpha);
}

LivingAnimation2dResult synthesize_living_animation2d(const SpriteSeed& seed) {
  LivingAnimation2dResult result;
  auto add = [&](std::string code, std::string msg) {
    result.validation.diagnostics.push_back({std::move(code), std::move(msg)});
  };

  const std::string entity_id = seed.stable_id;
  const RigDefinition& rig = seed.rig.has_value() ? *seed.rig : make_biped_rig(entity_id);
  constexpr std::int32_t canvas_w = 128, canvas_h = 128;

  auto base_pal = make_palette(seed.primary_color, seed.accent_color);      auto storm_pal = make_palette(
        !seed.storm_primary_color.empty() ? seed.storm_primary_color : seed.accent_color,
        !seed.storm_accent_color.empty() ? seed.storm_accent_color : seed.primary_color);

  // Resolve per-form morphology
  auto base_morph = resolve_form_morphology(seed, "base");
  auto storm_morph = resolve_form_morphology(seed, "storm");

  // Build form→clip map from seed's clip naming convention (base_*, storm_*, transform_*)
  std::map<std::string, const SkeletalClip*, std::less<>> clip_map;
  for (auto const& c : seed.clips) clip_map[c.id] = &c;

  auto find_clip = [&](std::string_view form_id, std::string_view name) -> const SkeletalClip* {
    std::string exact = std::string(form_id) + "_" + std::string(name);
    auto it = clip_map.find(exact);
    return (it != clip_map.end()) ? it->second : nullptr;
  };

  // Rendering lambda: uses z_order for sorting
  auto render_morph = [&](const std::map<std::string, MorphologyPart, std::less<>>& morph,
                           const EvaluatedPose& pose, const SynthesisPalette& pal) -> ImageRgba8 {
    ImageRgba8 canvas(canvas_w, canvas_h, ColorSpace::srgb, AlphaMode::straight,
                      std::vector<std::uint8_t>(static_cast<std::size_t>(canvas_w) * canvas_h * 4, 0));
    // Draw rotated ellipse helper
    auto draw_rotated_ellipse = [&](ImageRgba8& img, int cx, int cy, int rx, int ry,
                                     double rotation_deg, double scale_x, double scale_y,
                                     std::uint32_t rgba) {
      std::uint8_t r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
      double rad = -rotation_deg * 3.141592653589793 / 180.0;
      double cos_r = std::cos(rad), sin_r = std::sin(rad);
      int erx = (std::max)(static_cast<int>(rx * scale_x), 1);
      int ery = (std::max)(static_cast<int>(ry * scale_y), 1);
      int bb = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(erx)*erx + static_cast<double>(ery)*ery))) + 1;
      for (int dy = -bb; dy <= bb; ++dy) {
        for (int dx = -bb; dx <= bb; ++dx) {
          double lx = dx * cos_r - dy * sin_r;
          double ly = dx * sin_r + dy * cos_r;
          if (lx*lx / (erx*erx) + ly*ly / (ery*ery) <= 1.0) {
            int px = cx + dx, py = cy + dy;
            if (px >= 0 && px < (int)img.width && py >= 0 && py < (int)img.height) {
              std::size_t idx = (static_cast<std::size_t>(py) * img.width + static_cast<std::size_t>(px)) * 4;
              img.pixels[idx] = r; img.pixels[idx+1] = g; img.pixels[idx+2] = b; img.pixels[idx+3] = a;
            }
          }
        }
      }
    };
    // Sort by z_order then by name for deterministic tie-breaking
    std::vector<std::pair<std::string, MorphologyPart>> sorted;
    for (auto const& [n, p] : morph) sorted.push_back({n, p});
    std::ranges::sort(sorted, [](auto const& a, auto const& b) {
      if (a.second.z_order != b.second.z_order) return a.second.z_order < b.second.z_order;
      return a.first < b.first;
    });
    for (auto const& [name, part] : sorted) {
      std::uint32_t color = !part.color.empty() && part.color[0] == '#' && part.color.size() == 7
          ? hex_color(part.color) : pal.primary;
      std::string target = part.bone_id;
      auto wit = pose.world.find(target);
      double bx = 0, by = 0, brot = 0, bsx = 1.0, bsy = 1.0;
      if (wit != pose.world.end()) {
        bx = wit->second.x; by = wit->second.y;
        brot = wit->second.rotation_degrees;
        bsx = wit->second.scale_x; bsy = wit->second.scale_y;
      }
      // Affine composition: bone world × part local offset
      double rad = brot * 3.141592653589793 / 180.0;
      double cos_r = std::cos(rad), sin_r = std::sin(rad);
      double wx = bx + part.x * cos_r * bsx - part.y * sin_r * bsy;
      double wy = by + part.x * sin_r * bsx + part.y * cos_r * bsy;
      int pcx = canvas_w / 2 + static_cast<int>(wx * 3);
      int pcy = canvas_h / 2 - static_cast<int>(wy * 3);
      int prx = (std::max)(static_cast<int>(part.size_x * 1.5), 2);
      int pry = (std::max)(static_cast<int>(part.size_y * 1.5), 2);
      double total_rot = brot + part.rotation_degrees;
      // Primitive selection with full scale consumed
      if (part.primitive == "capsule") {
        int len = static_cast<int>(pry * 2 * bsy);
        int wid = static_cast<int>(prx * bsx);
        draw_rotated_ellipse(canvas, pcx, pcy, std::max(wid/2, 1), std::max(len/2, 1), total_rot, 1.0, 1.0, color);
      } else if (part.primitive == "triangle") {
        int tw = static_cast<int>(prx * 2 * bsx);
        int th = static_cast<int>(pry * 2 * bsy);
        draw_rotated_ellipse(canvas, pcx, pcy, std::max(tw/2, 2), std::max(th/2, 2), total_rot, 1.0, 1.0, color);
      } else if (part.primitive == "segmented_curve") {
        draw_rotated_ellipse(canvas, pcx, pcy, std::max(static_cast<int>(prx * bsx), 2), std::max(static_cast<int>(pry * 3 * bsy), 2), total_rot, 1.0, 1.0, color);
      } else if (part.primitive == "aura_contour") {
        std::uint32_t aura_rgba = with_alpha(color, (color & 0xFF) / 3);
        draw_rotated_ellipse(canvas, pcx, pcy, static_cast<int>(prx * 2 * bsx), static_cast<int>(pry * 2 * bsy), total_rot, 1.0, 1.0, aura_rgba);
      } else {
        draw_rotated_ellipse(canvas, pcx, pcy, prx, pry, total_rot, bsx, bsy, color);
      }
    }
    return canvas;
  };

  // Generate frames for a form: calls evaluate_pose per clip, returns frame index of first frame
  auto gen_form_frames = [&](const std::map<std::string, MorphologyPart, std::less<>>& morph,
                              std::string_view form_id, const SynthesisPalette& pal,
                              std::vector<FrameSource>& out_frames,
                              std::string& pf) -> std::map<std::string, std::uint32_t> {
    pf = entity_id + "." + std::string(form_id);
    std::map<std::string, std::uint32_t> first_frame;
    auto gen = [&](const char* clip_name, std::uint32_t count, std::uint32_t dur) {
      auto* clip = find_clip(form_id, clip_name);
      if (!clip) {
        add("MISSING_CLIP", std::string(form_id) + "_" + clip_name + " not found in seed clips");
        return;
      }
      std::uint32_t start = static_cast<std::uint32_t>(out_frames.size());
      for (std::uint32_t i = 0; i < count; ++i) {
        // Nonlooping: include both endpoints; looping: sample evenly spaced
        std::uint32_t tick = clip->duration_ticks > 0
          ? (clip->looping
              ? (i * clip->duration_ticks / count)
              : (count > 1 ? static_cast<std::uint32_t>((i * (clip->duration_ticks - 1ull) + (count - 2) / 2) / (count - 1)) : 0u))
          : i;
        auto pr = evaluate_pose(*clip, rig, tick);
        if (pr.ok()) {
          out_frames.push_back({pf + "." + clip_name + "." + std::to_string(i),
                                render_morph(morph, *pr.value, pal),
                                canvas_w / 2, canvas_h / 2, dur});
        }
      }
      first_frame[clip_name] = start;
    };
    gen("idle", 4, 4);
    gen("locomotion", 6, 3);
    gen("attack", 6, 2);
    gen("hit", 3, 2);
    return first_frame;
  };

  std::string base_pf, storm_pf;
  auto base_first = gen_form_frames(base_morph, "base", base_pal, result.base_frames, base_pf);

  // Generate transformation once
  {
    auto* tclip = find_clip("transform", "transform");
    std::string tpf = entity_id + ".transform";
    if (tclip) {
      for (std::uint32_t i = 0; i < 10; ++i) {
        std::uint32_t tick = tclip->duration_ticks > 0 ? (i * tclip->duration_ticks / 10) : i;
        // Interpolate between base and storm morphology for transformation
        auto t_morph = base_morph;
        double blend = static_cast<double>(i) / 9.0;
        for (auto& [name, part] : t_morph) {
          auto sit = storm_morph.find(name);
          if (sit != storm_morph.end()) {
            part.size_x += (sit->second.size_x - part.size_x) * blend;
            part.size_y += (sit->second.size_y - part.size_y) * blend;
            part.x += (sit->second.x - part.x) * blend;
            part.y += (sit->second.y - part.y) * blend;
          }
        }
        auto pr = evaluate_pose(*tclip, rig, tick);
        if (pr.ok()) {
          result.transformation_frames.push_back({tpf + "." + std::to_string(i),
            render_morph(t_morph, *pr.value, base_pal), canvas_w / 2, canvas_h / 2, 2});
        }
      }
    }
  }

  auto storm_first = gen_form_frames(storm_morph, "storm", storm_pal, result.storm_frames, storm_pf);

  // Assemble all_frames: base + transform + storm
  result.all_frames = result.base_frames;
  for (auto& f : result.transformation_frames) result.all_frames.push_back(f);
  for (auto& f : result.storm_frames) result.all_frames.push_back(f);

  // Compute frame hashes
  for (auto& f : result.all_frames) f.frame_hash = compute_frame_hash(f.image);

  // Build sprite sheet
  SpriteSheetOptions opts{1024, 2048, 2, false, 0};
  result.sheet = compile_sprite_sheet(result.all_frames, opts);

  // Build animation clips with authored events
  auto build_clips = [&](std::string_view pf, std::map<std::string, std::uint32_t> const& first,
                          std::string_view form_id, std::uint32_t /*base_idx*/) {
    struct ClipDef { std::string name; std::uint32_t count; std::uint32_t dur; };
    std::vector<ClipDef> defs = {
      {"idle", 4, 4}, {"locomotion", 6, 3}, {"attack", 6, 2}, {"hit", 3, 2}
    };
    for (auto const& d : defs) {
      auto fit = first.find(d.name);
      if (fit == first.end()) continue;
      std::vector<std::string> fids;
      std::vector<std::uint32_t> durs;
      for (std::uint32_t i = 0; i < d.count; ++i) {
        fids.push_back(std::string(pf) + "." + d.name + "." + std::to_string(i));
        durs.push_back(d.dur);
      }
      // Map authored clip events to frame indices
      std::vector<AnimationEvent> events;
      auto* clip = find_clip(form_id, d.name);
      if (clip) {
        for (auto const& [ev_name, ev_tick] : clip->events) {
          std::uint32_t fi = clip->duration_ticks > 0 ? (ev_tick * d.count / clip->duration_ticks) : 0;
          if (fi < d.count) events.push_back({ev_name, fi});
        }
      }
      bool looping = clip ? clip->looping : true;
      result.clips.push_back({std::string(pf) + "." + d.name, std::move(fids), std::move(durs), std::move(events), looping});
    }
  };
  build_clips(base_pf, base_first, "base", 0);
  // Transformation clip
  {
    std::string tpf = entity_id + ".transform";
    std::vector<std::string> fids;
    std::vector<std::uint32_t> durs;
    for (int i = 0; i < 10; ++i) {
      fids.push_back(tpf + "." + std::to_string(i));
      durs.push_back(2);
    }
    std::vector<AnimationEvent> events;
    auto* tclip = find_clip("transform", "transform");
    if (tclip) {
      for (auto const& [ev_name, ev_tick] : tclip->events) {
        std::uint32_t fi = tclip->duration_ticks > 0 ? (ev_tick * 10 / tclip->duration_ticks) : 0;
        if (fi < 10) events.push_back({ev_name, fi});
      }
    }
    result.clips.push_back({tpf, std::move(fids), std::move(durs), std::move(events), false});
  }
  build_clips(storm_pf, storm_first, "storm", 19);

  // Channel maps
  auto depth = ImageRgba8{canvas_w, canvas_h, ColorSpace::data, AlphaMode::opaque,
                          std::vector<std::uint8_t>(static_cast<std::size_t>(canvas_w) * canvas_h * 4, 0)};
  for (std::size_t i = 0; i < depth.pixels.size(); i += 4) {
    depth.pixels[i] = depth.pixels[i+1] = depth.pixels[i+2] = 64;
    depth.pixels[i+3] = 255;
  }
  result.channel_maps = {{base_pf + ".idle.0.depth", base_pf + ".idle.0", ChannelMapKind::depth, depth}};

  // Collision shapes and windows from seed
  result.collision_shapes = seed.collision_shapes;
  result.collision_windows = seed.collision_windows;

  // Validate invariants
  if (result.base_frames.size() != 19)
    add("FRAME_COUNT", "base frames: expected 19, got " + std::to_string(result.base_frames.size()));
  if (result.transformation_frames.size() != 10)
    add("FRAME_COUNT", "transform frames: expected 10, got " + std::to_string(result.transformation_frames.size()));
  if (result.storm_frames.size() != 19)
    add("FRAME_COUNT", "storm frames: expected 19, got " + std::to_string(result.storm_frames.size()));
  if (result.all_frames.size() != 48)
    add("FRAME_COUNT", "total frames: expected 48, got " + std::to_string(result.all_frames.size()));

  return result;
}

} // namespace gspl::sprites