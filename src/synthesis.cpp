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
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  // Canvas size based on morphology extents
  constexpr std::int32_t canvas_w = 64, canvas_h = 64;
  auto draw_ellipse = [](ImageRgba8& img, int cx, int cy, int rx, int ry, std::uint32_t rgba) {
    std::uint8_t r = (rgba >> 24) & 0xFF, g = (rgba >> 16) & 0xFF, b = (rgba >> 8) & 0xFF, a = rgba & 0xFF;
    for (int y = -ry; y <= ry; ++y) {
      for (int x = -rx; x <= rx; ++x) {
        if (x*x * ry*ry + y*y * rx*rx <= rx*rx * ry*ry) {
          int px = cx + x, py = cy + y;
          if (px >= 0 && px < (int)img.width && py >= 0 && py < (int)img.height) {
            std::size_t idx = (static_cast<std::size_t>(py) * img.width + static_cast<std::size_t>(px)) * 4;
            img.pixels[idx] = r; img.pixels[idx+1] = g; img.pixels[idx+2] = b; img.pixels[idx+3] = a;
          }
        }
      }
    }
  };

  // Build frames: idle (2 frames), attack (2 frames), hit (1 frame)
  std::vector<FrameSource> frames;
  // Each frame is a copy of the static morphology with slight pose variation
  for (int variant = 0; variant < 5; ++variant) {
    ImageRgba8 canvas(canvas_w, canvas_h, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(canvas_w * canvas_h * 4, 0));
    // Draw each morphology part as an ellipse, z-sorted (tail first, then legs, torso, head, ears, eyes, muzzle, aura last)
    // Manual z-sort based on expected depth
    struct ZPart { std::string_view name; std::int64_t z; };
    const ZPart order[] = {
      {"tail", -30}, {"left_front_leg", -20}, {"right_front_leg", -20}, {"torso", 0},
      {"head", 20}, {"left_ear", 25}, {"right_ear", 25}, {"left_eye", 30},
      {"right_eye", 30}, {"muzzle", 30}, {"aura", -50}
    };
    for (const auto& zp : order) {
      auto it = morphology.find(std::string(zp.name));
      if (it == morphology.end()) continue;
      const auto& part = it->second;
      // Determine color
      std::uint32_t color = palette.primary;
      if (part.color == "#56F1FF" || zp.name.find("eye") != std::string::npos || zp.name.find("ear") != std::string::npos)
        color = palette.accent;
      else if (part.color == "#FFFFFF") color = 0xFFFFFF00 | 0xFF;
      else if (!part.color.empty() && part.color != "#242038" && part.color != "#56F1FF") color = hex_color(part.color);
      // Position on canvas (offset to center, scaled)
      const int pcx = canvas_w / 2 + static_cast<int>(part.x * 2);
      const int pcy = canvas_h / 2 - static_cast<int>(part.y * 2);
      const int prx = static_cast<int>(part.size_x);
      const int pry = static_cast<int>(part.size_y);
      draw_ellipse(canvas, pcx, pcy, (std::max)(prx, 2), (std::max)(pry, 2), color);
    }
    // Variants: slight offsets to simulate animation
    if (variant == 1 || variant == 3) {
      // Blink: draw white over eyes
      if (auto e = morphology.find("left_eye"); e != morphology.end())
        draw_ellipse(canvas, canvas_w/2 + static_cast<int>(e->second.x * 2) - 1, canvas_h/2 - static_cast<int>(e->second.y * 2), 3, 1, 0xFFFFFF00 | 0xFF);
      if (auto e = morphology.find("right_eye"); e != morphology.end())
        draw_ellipse(canvas, canvas_w/2 + static_cast<int>(e->second.x * 2) + 1, canvas_h/2 - static_cast<int>(e->second.y * 2), 3, 1, 0xFFFFFF00 | 0xFF);
    }
    const char* names[] = {"idle", "idle", "attack", "attack", "hit"};
    frames.push_back({pf + "." + names[variant] + "." + std::to_string(variant), std::move(canvas), canvas_w/2, canvas_h/2, variant < 4 ? 2u : 1u, {}});
  }

  // Compile sprite sheet
  SpriteSheetOptions opts{256, 256, 2, false, 0};
  auto sheet = compile_sprite_sheet(frames, opts);

  // Animation clips
  std::vector<AnimationClip> anims = {
    {pf + ".idle", {pf + ".idle.0", pf + ".idle.1"}, {2, 2}, {}, true},
    {pf + ".attack", {pf + ".attack.2", pf + ".attack.3"}, {1, 1}, {{"release", 1}}, false},
    {pf + ".hit", {pf + ".hit.4"}, {1}, {}, false},
  };

  // Channel maps (depth)
  auto depth = ImageRgba8{canvas_w, canvas_h, ColorSpace::data, AlphaMode::opaque, std::vector<std::uint8_t>(static_cast<std::size_t>(canvas_w) * canvas_h * 4, 64)};
  std::vector<ChannelMap> channels = {
    {pf + ".idle.0.depth", pf + ".idle.0", ChannelMapKind::depth, depth},
  };

  // Collision shapes from morphology
  std::vector<CollisionShape> shapes;
  auto add_shape = [&](std::string_view part_name, std::string_view bone_id) {
    auto it = morphology.find(std::string(part_name));
    if (it == morphology.end()) return;
    const auto& p = it->second;
    shapes.push_back({std::string(part_name), CollisionKind::axis_aligned_box, std::string(bone_id),
                      p.x, p.y, p.size_x, p.size_y});
  };
  add_shape("torso", "root");
  add_shape("head", "head");

  for (auto& f : frames) f.frame_hash = compute_frame_hash(f.image);
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
} // namespace gspl::sprites