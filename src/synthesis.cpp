#include "gspl_sprites/synthesis.hpp"

#include "gspl_sprites/animation.hpp"
#include "gspl_sprites/animation3d.hpp"
#include "gspl_sprites/channel_map.hpp"
#include "gspl_sprites/projection25d.hpp"
#include "gspl_sprites/projection3d.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"

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
                                                 const SynthesisPalette& [[maybe_unused]] palette,
                                                 const RigDefinition& [[maybe_unused]] rig) {
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

Projection3dDefinition synthesize_projection3d(std::string_view entity_id, std::string_view form_id,
                                               const SynthesisPalette& [[maybe_unused]] palette,
                                               const RigDefinition& [[maybe_unused]] rig) {
  std::string pf = std::string(entity_id) + "." + std::string(form_id);
  Projection3dDefinition proj;
  proj.id = pf + ".3d";
  proj.materials = {
    {pf + ".body", palette.primary, 0, 900000},
    {pf + ".accent", palette.accent, 0, 900000},
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
                                                        const RigDefinition& [[maybe_unused]] rig) {
  std::string eid = std::string(entity_id);
  return {eid + ".manifest.2d",
    {{"base", "base", eid + ".base.2d"}, {"storm", "storm", eid + ".storm.2d"}},
    {{"ascend", "ascend"}, {"descend", "descend"}}};
}

TransformationManifestationProgram make_manifestation25d(std::string_view entity_id,
                                                         const RigDefinition& [[maybe_unused]] rig) {
  std::string eid = std::string(entity_id);
  return {eid + ".manifest.25d",
    {{"base", "base", eid + ".base.25d"}, {"storm", "storm", eid + ".storm.25d"}},
    {{"ascend", "ascend"}, {"descend", "descend"}}};
}

TransformationManifestationProgram make_manifestation3d(std::string_view entity_id,
                                                        const RigDefinition& [[maybe_unused]] rig) {
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

} // namespace gspl::sprites