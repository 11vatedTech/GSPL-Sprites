#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/rights.hpp"
#include "gspl_sprites/viewer_model.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

using namespace gspl::sprites;

namespace {

void print_diagnostics(const ValidationResult& vr) {
  for (const auto& d : vr.diagnostics)
    std::cerr << "  " << d.code << ": " << d.message << '\n';
}

int headless_main(const LivingPackageViewer& viewer) {
  const auto& prov = viewer.provenance();
  std::cout << "PACKAGE entity=" << prov.entity_id
            << " identity=" << prov.package_identity
            << " artifact_count=" << prov.artifact_count
            << " total_bytes=" << prov.total_artifact_bytes
            << " frames=" << prov.frame_count
            << " clips=" << prov.clip_count
            << " channels=" << prov.channel_count
            << '\n';

  std::cout << "HEADLESS_BEGIN\n";
  for (const auto& ci : viewer.clips()) {
    std::cout << "CLIP " << ci.clip_id
              << " frames=" << ci.frame_count
              << " duration=" << ci.duration_ticks
              << " looping=" << ci.looping
              << '\n';
  }

  // Show first clip playback (first 10 frames)
  if (!viewer.clips().empty()) {
    const auto& ci = viewer.clips()[0];
    for (std::uint32_t t = 0; t < 10; ++t) {
      auto f = viewer.frame_for_tick(ci.clip_id, t);
      std::cout << "TICK " << t << " frame=" << f.frame_id
                << " index=" << f.frame_index
                << " source_tick=" << f.source_tick
                << '\n';
    }
  }
  std::cout << "HEADLESS_END\n";
  return 0;
}

#ifdef GSPL_SPRITES_HAS_SDL2

#include <SDL.h>
#include <cstdio>

// ── 8x8 bitmap font (ASCII 32..126), public-domain classic font8x8 ──
static const std::uint64_t kFont8x8[95] = {
  0x0000000000000000ULL, 0x183C3C1818001800ULL, 0x3636000000000000ULL,
  0x36367F367F363600ULL, 0x0C3E031E301F0C00ULL, 0x006333180C666300ULL,
  0x1C361C6E3B336E00ULL, 0x0606030000000000ULL, 0x180C0606060C1800ULL,
  0x060C1818180C0600ULL, 0x00663CFF3C660000ULL, 0x000C0C3F0C0C0000ULL,
  0x0000000000000C06ULL, 0x0000003F00000000ULL, 0x0000000000000C00ULL,
  0x6030180C06030100ULL, 0x3E63737B6F673E00ULL, 0x0C0E0C0C0C0C3F00ULL,
  0x3E63603018037F00ULL, 0x3E63601C60633E00ULL, 0x30383C367F303000ULL,
  0x7F031F3060633E00ULL, 0x1C06031F33331E00ULL, 0x7F6330180C060600ULL,
  0x3E63633E63633E00ULL, 0x3E63637E60301E00ULL, 0x000C0C00000C0C00ULL,
  0x000C0C00000C0C06ULL, 0x180C0603060C1800ULL, 0x00003F003F000000ULL,
  0x060C1830180C0600ULL, 0x3E63603018001800ULL, 0x3E637B7B7B031E00ULL,
  0x0C1E33333F333300ULL, 0x3F66663E66663F00ULL, 0x3C66030303663C00ULL,
  0x1F36666666361F00ULL, 0x7F46161E16467F00ULL, 0x7F46161E16060F00ULL,
  0x3C66030373667C00ULL, 0x3333333F33333300ULL, 0x1E0C0C0C0C0C1E00ULL,
  0x7830303033331E00ULL, 0x6766361E36666700ULL, 0x0F06060646667F00ULL,
  0x63777F7F6B636300ULL, 0x63676F7B73636300ULL, 0x1C36636363361C00ULL,
  0x3F66663E06060F00ULL, 0x1E3333333B1E3800ULL, 0x3F66663E36666700ULL,
  0x1E33070E38331E00ULL, 0x3F2D0C0C0C0C1E00ULL, 0x3333333333333F00ULL,
  0x33333333331E0C00ULL, 0x6363636B7F776300ULL, 0x6363361C36636300ULL,
  0x3333331E0C0C1E00ULL, 0x7F6331184C667F00ULL, 0x1E06060606061E00ULL,
  0x03060C1830604000ULL, 0x1E18181818181E00ULL, 0x081C366300000000ULL,
  0x00000000000000FFULL, 0x0C0C180000000000ULL, 0x00001E303E336E00ULL,
  0x0706063E66663B00ULL, 0x00001E3303331E00ULL, 0x3830303E33336E00ULL,
  0x00001E333F031E00ULL, 0x1C36060F06060F00ULL, 0x00006E33333E301FULL,
  0x0706363E66666700ULL, 0x0C000E0C0C0C1E00ULL, 0x300030303033331EULL,
  0x070666361E366700ULL, 0x0E0C0C0C0C0C1E00ULL, 0x0000337F7F6B6300ULL,
  0x00003B6666666700ULL, 0x00001E3333331E00ULL, 0x00003B66663E060FULL,
  0x00006E33333E3078ULL, 0x00003B6606060F00ULL, 0x00003E031E301F00ULL,
  0x080C3E0C0C2C1800ULL, 0x0000333333336E00ULL, 0x00003333331E0C00ULL,
  0x0000636B7F7F3600ULL, 0x000063361C366300ULL, 0x00003333333E301FULL,
  0x00003F190C263F00ULL, 0x380C0C070C0C3800ULL, 0x1818181818181818ULL,
  0x070C0C380C0C0700ULL, 0x6E3B000000000000ULL,
};

static void draw_char(SDL_Renderer* ren, int x, int y, unsigned char ch,
                      std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  if (ch < 32 || ch > 126) return;
  const std::uint64_t gdata = kFont8x8[ch - 32];
  if (!gdata) return;
  SDL_SetRenderDrawColor(ren, r, g, b, 255);
  for (int row = 0; row < 8; ++row) {
    std::uint8_t bits = static_cast<std::uint8_t>(gdata >> (56 - row * 8));
    for (int col = 0; col < 8; ++col) {
      if (bits & (0x80 >> col))
        SDL_RenderDrawPoint(ren, x + col, y + row);
    }
  }
}

static void draw_text(SDL_Renderer* ren, int x, int y, const char* text,
                      std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  while (*text) {
    draw_char(ren, x, y, static_cast<unsigned char>(*text), r, g, b);
    x += 10;
    ++text;
  }
}

static std::string color_space_name(ColorSpace cs) {
  switch (cs) {
    case ColorSpace::srgb: return "srgb";
    case ColorSpace::linear_srgb: return "linear_srgb";
    case ColorSpace::acescg: return "acescg";
    case ColorSpace::data: return "data";
    default: return "unknown";
  }
}

static std::string alpha_mode_name(AlphaMode am) {
  switch (am) {
    case AlphaMode::opaque: return "opaque";
    case AlphaMode::straight: return "straight";
    case AlphaMode::premultiplied: return "premultiplied";
    default: return "unknown";
  }
}

static std::string channel_kind_name(ChannelMapKind k) {
  switch (k) {
    case ChannelMapKind::material_id: return "material_id";
    case ChannelMapKind::tangent_normal: return "tangent_normal";
    case ChannelMapKind::depth: return "depth";
    case ChannelMapKind::emissive: return "emissive";
    case ChannelMapKind::collision: return "collision";
    default: return "effects";
  }
}

enum class DisplayMode { frame, channel, atlas };

struct ViewerDisplayState {
  DisplayMode mode{DisplayMode::frame};
  std::size_t channel_index{0};
  bool debug{false};
  bool morphology_panel{false};
  bool onion{false};
  bool difference{false};
  bool nearest{true};
  int zoom_mode{0};  // 0 = fit, 1 = 100%, 2 = 200%, 3 = 400%
  bool smoke{false};
  int fps{0};
  int frame_count{0};
  std::uint32_t last_fps_time{0};
  double tick_accum{0.0};
};

// ── texture helpers ────────────────────────────────────────────────────
// ImageRgba8 stores R,G,B,A bytes per pixel. On little-endian, SDL
// ABGR8888's in-memory byte order is exactly R,G,B,A.
static SDL_Texture* make_texture(SDL_Renderer* ren, const ImageRgba8& img) {
  if (img.pixels.empty() || img.width == 0 || img.height == 0) return nullptr;
  SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888,
                                       SDL_TEXTUREACCESS_STATIC,
                                       static_cast<int>(img.width),
                                       static_cast<int>(img.height));
  if (!tex) return nullptr;
  if (SDL_UpdateTexture(tex, nullptr, img.pixels.data(),
                        static_cast<int>(img.width) * 4) != 0) {
    SDL_DestroyTexture(tex);
    return nullptr;
  }
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  return tex;
}

// Fit an image of (img_w x img_h) into viewport keeping aspect ratio.
static SDL_Rect fit_rect(int img_w, int img_h, int view_w, int view_h,
                         int margin) {
  SDL_Rect r{};
  if (img_w <= 0 || img_h <= 0 || view_w <= 0 || view_h <= 0) return r;
  double scale = std::min(static_cast<double>(view_w - 2 * margin) / img_w,
                          static_cast<double>(view_h - 2 * margin) / img_h);
  if (scale <= 0.0) scale = 1.0;
  r.w = static_cast<int>(img_w * scale);
  r.h = static_cast<int>(img_h * scale);
  r.x = (view_w - r.w) / 2;
  r.y = (view_h - r.h) / 2;
  return r;
}

// Zoom-aware destination rect (fit or 100/200/400% centered).
static SDL_Rect display_rect(int img_w, int img_h, int view_w, int view_h,
                             int margin, int zoom_mode) {
  if (zoom_mode == 0) return fit_rect(img_w, img_h, view_w, view_h, margin);
  double scale = static_cast<double>(zoom_mode);
  SDL_Rect r{};
  r.w = static_cast<int>(img_w * scale);
  r.h = static_cast<int>(img_h * scale);
  r.x = (view_w - r.w) / 2;
  r.y = (view_h - r.h) / 2;
  return r;
}

static void draw_checkerboard(SDL_Renderer* ren, const SDL_Rect& vp,
                              int cell) {
  for (int y = vp.y; y < vp.y + vp.h; y += cell) {
    for (int x = vp.x; x < vp.x + vp.w; x += cell) {
      bool on = ((x - vp.x) / cell + (y - vp.y) / cell) % 2 == 0;
      SDL_SetRenderDrawColor(ren, on ? 0x28 : 0x22, on ? 0x28 : 0x22,
                             on ? 0x30 : 0x28, 255);
      SDL_Rect c = {x, y, cell, cell};
      SDL_RenderFillRect(ren, &c);
    }
  }
}

static void draw_panel(SDL_Renderer* ren, int x, int y, int w, int h) {
  SDL_SetRenderDrawColor(ren, 10, 10, 16, 225);
  SDL_Rect bg = {x, y, w, h};
  SDL_RenderFillRect(ren, &bg);
  SDL_SetRenderDrawColor(ren, 86, 241, 255, 255);
  SDL_RenderDrawRect(ren, &bg);
}

// ── panel renderers ────────────────────────────────────────────────────
static void draw_provenance_panel(SDL_Renderer* ren, const LivingPackageViewer& viewer) {
  const auto& prov = viewer.provenance();
  char line[192];
  int y = 10;
  draw_panel(ren, 8, 8, 480, 128);
  std::snprintf(line, sizeof(line), "Entity:  %s", prov.entity_id.c_str());
  draw_text(ren, 16, y, line, 86, 241, 255); y += 12;
  std::snprintf(line, sizeof(line), "Package: %s", prov.package_identity.c_str());
  draw_text(ren, 16, y, line, 86, 241, 255); y += 12;
  std::snprintf(line, sizeof(line), "Seed:    %s", prov.seed_identity.c_str());
  draw_text(ren, 16, y, line, 86, 241, 255); y += 12;
  std::snprintf(line, sizeof(line), "Canonical: %s", prov.canonical_entity_identity.c_str());
  draw_text(ren, 16, y, line, 86, 241, 255); y += 12;
  std::snprintf(line, sizeof(line), "Rights:  %s   Schema: %s",
                std::string(rights_class_to_string(prov.rights)).c_str(),
                prov.manifest_format.c_str());
  draw_text(ren, 16, y, line, 86, 241, 255); y += 12;
  std::snprintf(line, sizeof(line),
                "Artifacts: %u  bytes: %llu  frames: %u  clips: %u  ch: %u",
                prov.artifact_count,
                static_cast<unsigned long long>(prov.total_artifact_bytes),
                prov.frame_count, prov.clip_count, prov.channel_count);
  draw_text(ren, 16, y, line, 86, 241, 255); y += 12;
  std::snprintf(line, sizeof(line),
                "Samples: %u  events: %u  collisions: %u  xforms: %u",
                prov.sample_count, prov.event_count,
                prov.collision_shape_count, prov.transformation_morphology_count);
  draw_text(ren, 16, y, line, 86, 241, 255);
}

static void draw_morphology_panel(SDL_Renderer* ren,
                                  const LivingPackageViewer& viewer) {
  const ViewerClipInfo* clip = viewer.clip(viewer.current_clip_id());
  if (!clip) return;

  const EffectiveMorphology* morph = nullptr;
  std::string title = "Morphology: ";
  switch (clip->group) {
    case ViewerFormGroup::base:
      morph = &viewer.base_morphology();
      title += "base";
      break;
    case ViewerFormGroup::storm:
      morph = &viewer.storm_morphology();
      title += "storm";
      break;
    case ViewerFormGroup::transformation: {
      auto spans = viewer.transformation_morphologies();
      if (!spans.empty()) {
        morph = &spans[0];
        title += "transformation";
      } else {
        title += "transformation (none)";
      }
      break;
    }
    default:
      title += "other";
      break;
  }

  char line[192];
  draw_panel(ren, 8, 144, 640, 320);
  draw_text(ren, 16, 152, title.c_str(), 255, 200, 100);

  if (!morph) return;
  int y = 166;
  int count = 0;
  for (const auto& [id, part] : *morph) {
    if (y > 420) break;
    std::snprintf(line, sizeof(line), "%s parent=%s role=%s prim=%s",
                  id.c_str(), part.parent.c_str(),
                  part.semantic_role.c_str(), part.primitive.c_str());
    draw_text(ren, 16, y, line, 86, 241, 255); y += 11;
    std::snprintf(line, sizeof(line),
                  "  pos=(%.0f,%.0f,%.0f) size=(%.0f,%.0f,%.0f) rot=%.0f z=%d%s%s",
                  part.x, part.y, part.z, part.size_x, part.size_y, part.size_z,
                  part.rotation_degrees, part.z_order,
                  part.emissive ? " EMISSIVE" : "",
                  part.electrical_marking ? " ELEC" : "");
    draw_text(ren, 16, y, line, 150, 150, 150); y += 11;
    ++count;
    if (count >= 12) break;
  }
}

static void draw_hud(SDL_Renderer* ren, const LivingPackageViewer& viewer,
                     const ViewerDisplayState& ds, int view_w) {
  char line[192];
  std::string mode_name;
  switch (ds.mode) {
    case DisplayMode::frame: mode_name = "FRAME"; break;
    case DisplayMode::channel: mode_name = "CHANNEL"; break;
    case DisplayMode::atlas: mode_name = "ATLAS"; break;
  }
  const char* state_name = "STOPPED";
  if (viewer.playback_state() == ViewerPlaybackState::playing) state_name = "PLAYING";
  else if (viewer.playback_state() == ViewerPlaybackState::paused) state_name = "PAUSED";

  draw_panel(ren, view_w - 260, 8, 252, 96);
  std::snprintf(line, sizeof(line), "Clip:   %s",
                std::string(viewer.current_clip_id()).c_str());
  draw_text(ren, view_w - 252, 16, line, 86, 241, 255);
  std::snprintf(line, sizeof(line), "Frame:  %s  tick=%u",
                std::string(viewer.current_frame_id()).c_str(),
                viewer.current_tick());
  draw_text(ren, view_w - 252, 28, line, 86, 241, 255);
  std::snprintf(line, sizeof(line), "Mode:   %s  %s  x%.1f",
                mode_name.c_str(), state_name, viewer.speed());
  draw_text(ren, view_w - 252, 40, line, 86, 241, 255);
  std::snprintf(line, sizeof(line), "FPS:    %d  zoom=%d  scale=%s",
                ds.fps, ds.zoom_mode, ds.nearest ? "nearest" : "smooth");
  draw_text(ren, view_w - 252, 52, line, 86, 241, 255);
  draw_text(ren, view_w - 252, 66,
            "SP=play R=restart <-/->=step ^=faster v=slower", 120, 120, 120);
  draw_text(ren, view_w - 252, 78,
            "[/]=clip 1=fr 2=ch 3=atlas C=ch++ D=prov M=morph", 120, 120, 120);
}

// ── overlay renderers ──────────────────────────────────────────────────
static void draw_collision_overlay(SDL_Renderer* ren,
                                   const LivingPackageViewer& viewer,
                                   const SDL_Rect& frame_rect,
                                   const ImageRgba8& img) {
  if (viewer.collision_shapes().empty() || viewer.collision_windows().empty())
    return;
  auto clip_id = viewer.current_clip_id();
  auto active = viewer.active_collision_windows(clip_id, viewer.current_tick());
  if (active.empty()) return;

  // Display scale: sprite coordinates ~ +/-100 units map to half the frame.
  double k = static_cast<double>(std::min(img.width, img.height)) / 200.0;
  SDL_SetRenderDrawColor(ren, 255, 80, 80, 255);
  for (const auto& win : active) {
    for (const auto& shape : viewer.collision_shapes()) {
      if (shape.id != win.shape_id) continue;
      int cx = frame_rect.x + static_cast<int>((img.width / 2.0 + shape.offset_x * k));
      int cy = frame_rect.y + static_cast<int>((img.height / 2.0 + shape.offset_y * k));
      int hw = static_cast<int>(shape.extent_x * k);
      int hh = static_cast<int>(shape.extent_y * k);
      SDL_Rect r = {cx - hw, cy - hh, hw * 2, hh * 2};
      SDL_RenderDrawRect(ren, &r);
      if (win.deals_damage) {
        SDL_SetRenderDrawColor(ren, 255, 60, 60, 90);
        SDL_RenderFillRect(ren, &r);
        SDL_SetRenderDrawColor(ren, 255, 80, 80, 255);
      }
      break;
    }
  }
}

static void draw_event_markers(SDL_Renderer* ren,
                               const LivingPackageViewer& viewer,
                               const SDL_Rect& frame_rect) {
  const ViewerClipInfo* clip = viewer.clip(viewer.current_clip_id());
  if (!clip) return;
  char line[128];
  int my = frame_rect.y;
  for (const auto& ev : clip->events) {
    if (ev.mapped_source_tick == viewer.current_tick()) {
      std::snprintf(line, sizeof(line), "EVENT %s tick=%u",
                    ev.event_id.c_str(), ev.authored_tick);
      SDL_SetRenderDrawColor(ren, 255, 220, 60, 255);
      for (int i = 0; i < 40; ++i)
        SDL_RenderDrawPoint(ren, frame_rect.x + 8 + i, frame_rect.y - 2);
      draw_text(ren, frame_rect.x + 8, frame_rect.y - 12, line, 255, 220, 60);
      (void)my;
    }
  }
}

// ── mode renderers ─────────────────────────────────────────────────────
static void render_frame_mode(SDL_Renderer* ren, int w, int h,
                              const LivingPackageViewer& viewer,
                              ViewerDisplayState& ds) {
  const ViewerFrame vf = viewer.current_frame();
  if (!vf.frame || vf.frame->image.pixels.empty()) return;
  const ImageRgba8& img = vf.frame->image;

  SDL_Rect vp = {0, 0, w, h};
  draw_checkerboard(ren, vp, 8);
  SDL_Rect target = display_rect(static_cast<int>(img.width),
                                 static_cast<int>(img.height), w, h, 40,
                                 ds.zoom_mode);

  if (ds.onion || ds.difference) {
    std::uint32_t prev_tick = viewer.current_tick() > 0
                                  ? viewer.current_tick() - 1
                                  : 0;
    ViewerFrame pv = viewer.frame_for_tick(viewer.current_clip_id(), prev_tick);
    if (pv.frame && !pv.frame->image.pixels.empty()) {
      if (ds.difference) {
        ImageRgba8 diff = LivingPackageViewer::difference_mask(pv.frame->image, img);
        if (SDL_Texture* dtex = make_texture(ren, diff)) {
          SDL_RenderCopy(ren, dtex, nullptr, &target);
          SDL_DestroyTexture(dtex);
        }
      } else {
        ImageRgba8 blend = LivingPackageViewer::blend_frames(pv.frame->image, img, 0.5);
        if (SDL_Texture* btex = make_texture(ren, blend)) {
          SDL_RenderCopy(ren, btex, nullptr, &target);
          SDL_DestroyTexture(btex);
        }
      }
    } else {
      if (SDL_Texture* tex = make_texture(ren, img)) {
        SDL_RenderCopy(ren, tex, nullptr, &target);
        SDL_DestroyTexture(tex);
      }
    }
  } else {
    if (SDL_Texture* tex = make_texture(ren, img)) {
      SDL_RenderCopy(ren, tex, nullptr, &target);
      SDL_DestroyTexture(tex);
    }
  }

  // pivot crosshair + collision/event overlays (package data only)
  int px = target.x + static_cast<int>(img.width * target.w / static_cast<double>(img.width)) + vf.frame->pivot_x;
  int py = target.y + static_cast<int>(img.height * target.h / static_cast<double>(img.height)) + vf.frame->pivot_y;
  SDL_SetRenderDrawColor(ren, 60, 220, 120, 255);
  SDL_RenderDrawLine(ren, px - 6, py, px + 6, py);
  SDL_RenderDrawLine(ren, px, py - 6, px, py + 6);

  draw_collision_overlay(ren, viewer, target, img);
  draw_event_markers(ren, viewer, target);
}

static void render_channel_mode(SDL_Renderer* ren, int w, int h,
                                const LivingPackageViewer& viewer,
                                ViewerDisplayState& ds) {
  auto info = viewer.channel_info();
  SDL_Rect vp = {0, 0, w, h};
  draw_checkerboard(ren, vp, 8);
  if (info.empty()) {
    draw_text(ren, w / 2 - 60, h / 2, "no channels", 255, 200, 100);
    return;
  }
  std::size_t ci = ds.channel_index % info.size();
  const auto& cinfo = info[ci];
  const ChannelMap* cm = viewer.channel(cinfo.id);
  if (!cm || cm->image.pixels.empty()) {
    draw_text(ren, w / 2 - 80, h / 2, "channel image missing", 255, 200, 100);
    return;
  }
  SDL_Rect target = display_rect(static_cast<int>(cm->image.width),
                                 static_cast<int>(cm->image.height), w, h, 40,
                                 ds.zoom_mode);
  if (SDL_Texture* tex = make_texture(ren, cm->image)) {
    SDL_RenderCopy(ren, tex, nullptr, &target);
    SDL_DestroyTexture(tex);
  }

  char line[192];
  draw_panel(ren, 8, 8, 470, 60);
  std::snprintf(line, sizeof(line), "Channel: %s  kind=%s",
                cinfo.id.c_str(), channel_kind_name(cinfo.kind).c_str());
  draw_text(ren, 16, 16, line, 86, 241, 255);
  std::snprintf(line, sizeof(line), "Target:  %s  %ux%u  %s  %s",
                cinfo.target_frame_id.c_str(), cinfo.width, cinfo.height,
                color_space_name(cinfo.color_space).c_str(),
                alpha_mode_name(cinfo.alpha_mode).c_str());
  draw_text(ren, 16, 28, line, 86, 241, 255);
  std::snprintf(line, sizeof(line), "Channels: %zu/%zu   (C = next)",
                ci + 1, info.size());
  draw_text(ren, 16, 40, line, 150, 150, 150);
}

static void render_atlas_mode(SDL_Renderer* ren, int w, int h,
                              const LivingPackageViewer& viewer,
                              ViewerDisplayState& ds) {
  const ImageRgba8* aimg = viewer.atlas_image();
  SDL_Rect vp = {0, 0, w, h};
  draw_checkerboard(ren, vp, 8);
  if (!aimg || aimg->pixels.empty()) {
    draw_text(ren, w / 2 - 80, h / 2, "no atlas in package", 255, 200, 100);
    return;
  }
  SDL_Rect target = fit_rect(static_cast<int>(aimg->width),
                             static_cast<int>(aimg->height), w, h, 40);
  if (SDL_Texture* tex = make_texture(ren, *aimg)) {
    SDL_RenderCopy(ren, tex, nullptr, &target);
    SDL_DestroyTexture(tex);
  }

  // placements
  double sx = static_cast<double>(target.w) / aimg->width;
  double sy = static_cast<double>(target.h) / aimg->height;
  SDL_SetRenderDrawColor(ren, 255, 200, 100, 255);
  char idbuf[64];
  for (const auto& pl : viewer.atlas_placements()) {
    SDL_Rect pr = {target.x + static_cast<int>(pl.x * sx),
                   target.y + static_cast<int>(pl.y * sy),
                   std::max(1, static_cast<int>(pl.width * sx)),
                   std::max(1, static_cast<int>(pl.height * sy))};
    SDL_RenderDrawRect(ren, &pr);
    std::snprintf(idbuf, sizeof(idbuf), "%s", pl.frame_id.c_str());
    draw_text(ren, pr.x + 2, pr.y + 2, idbuf, 255, 220, 60);
  }

  char line[192];
  draw_panel(ren, 8, 8, 470, 36);
  std::snprintf(line, sizeof(line), "Atlas placements: %zu  (click = select frame)",
                viewer.atlas_placements().size());
  draw_text(ren, 16, 16, line, 86, 241, 255);
  (void)ds;
}

// ── playback helpers ───────────────────────────────────────────────────
static void select_clip_at(LivingPackageViewer& viewer, std::ptrdiff_t delta) {
  auto clips = viewer.clips();
  if (clips.empty()) return;
  std::size_t idx = 0;
  std::string_view cur = viewer.current_clip_id();
  for (std::size_t i = 0; i < clips.size(); ++i) {
    if (clips[i].clip_id == cur) { idx = i; break; }
  }
  std::ptrdiff_t next = static_cast<std::ptrdiff_t>(idx) + delta;
  if (next < 0) next = static_cast<std::ptrdiff_t>(clips.size()) - 1;
  if (next >= static_cast<std::ptrdiff_t>(clips.size())) next = 0;
  const auto& ci = clips[static_cast<std::size_t>(next)];
  (void)viewer.select_clip(ci.clip_id);
  viewer.set_looping(ci.looping);
  viewer.seek_tick(0);
}

static void select_frame_in_clip(LivingPackageViewer& viewer,
                                 std::string_view frame_id) {
  for (const auto& ci : viewer.clips()) {
    for (std::size_t i = 0; i < ci.frame_ids.size(); ++i) {
      if (ci.frame_ids[i] == frame_id) {
        (void)viewer.select_clip(ci.clip_id);
        viewer.set_looping(ci.looping);
        std::uint32_t tick = i < ci.frame_start_ticks.size()
                                 ? ci.frame_start_ticks[i]
                                 : 0;
        viewer.seek_tick(tick);
        return;
      }
    }
  }
}

// ── main SDL loop ──────────────────────────────────────────────────────
static int sdl_main(LivingPackageViewer& viewer, bool smoke) {
  if (viewer.clip_count() == 0) {
    std::cerr << "Package has no clips to display\n";
    return 1;
  }
  if (smoke) SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL_Init: " << SDL_GetError() << '\n';
    return 1;
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

  SDL_Window* win = SDL_CreateWindow(
      "GSPL Sprites - Living Visual Package Viewer",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 960, 640,
      SDL_WINDOW_SHOWN);
  if (!win) {
    std::cerr << "SDL_CreateWindow: " << SDL_GetError() << '\n';
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* ren = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren) {
    // Dummy/headless drivers (and some VMs) have no accelerated renderer.
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
  }
  if (!ren) {
    std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << '\n';
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  ViewerDisplayState ds;
  ds.smoke = smoke;

  // Start playback on the first clip (package-driven timing/looping).
  const auto& first = viewer.clips()[0];
  (void)viewer.select_clip(first.clip_id);
  viewer.set_looping(first.looping);
  viewer.play();

  std::cout << "Preview: entity=" << viewer.provenance().entity_id
            << " package=" << viewer.provenance().package_identity
            << " artifacts=" << viewer.provenance().artifact_count << '\n';
  std::cout << "Controls: SP=play/pause R=restart <-/->=step ^=faster v=slower "
               "[/]=clip 1=frame 2=channel 3=atlas C=channel+ N=pixels "
               "Z=zoom D=provenance M=morphology O=onion X=difference "
               "Q/ESC=quit\n";

  bool running = true;
  ds.last_fps_time = SDL_GetTicks();

  while (running) {
    std::uint32_t frame_start = SDL_GetTicks();

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) {
        running = false;
      } else if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.sym) {
          case SDLK_SPACE: viewer.toggle_play(); break;
          case SDLK_RIGHT: (void)viewer.step_frame(1); break;
          case SDLK_LEFT: (void)viewer.step_frame(-1); break;
          case SDLK_UP: viewer.set_speed(viewer.speed() * 1.5); break;
          case SDLK_DOWN:
            viewer.set_speed(std::max(0.1, viewer.speed() / 1.5));
            break;
          case SDLK_r: viewer.stop(); viewer.play(); break;
          case SDLK_RIGHTBRACKET: select_clip_at(viewer, 1); break;
          case SDLK_LEFTBRACKET: select_clip_at(viewer, -1); break;
          case SDLK_1: ds.mode = DisplayMode::frame; break;
          case SDLK_2: ds.mode = DisplayMode::channel; break;
          case SDLK_3: ds.mode = DisplayMode::atlas; break;
          case SDLK_c:
            if (!viewer.channel_info().empty())
              ds.channel_index = (ds.channel_index + 1) % viewer.channel_info().size();
            break;
          case SDLK_z: ds.zoom_mode = (ds.zoom_mode + 1) % 4; break;
          case SDLK_n:
            ds.nearest = !ds.nearest;
            SDL_DestroyRenderer(ren);
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,
                        ds.nearest ? "nearest" : "linear");
            ren = SDL_CreateRenderer(
                win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
            if (!ren) {
              std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << '\n';
              running = false;
            }
            break;
          case SDLK_d: ds.debug = !ds.debug; break;
          case SDLK_m: ds.morphology_panel = !ds.morphology_panel; break;
          case SDLK_o: ds.onion = !ds.onion; ds.difference = false; break;
          case SDLK_x: ds.difference = !ds.difference; ds.onion = false; break;
          case SDLK_q:
          case SDLK_ESCAPE: running = false; break;
          default: break;
        }
      } else if (ev.type == SDL_MOUSEBUTTONDOWN &&
                 ev.button.button == SDL_BUTTON_LEFT) {
        if (ds.mode == DisplayMode::atlas) {
          const ImageRgba8* aimg = viewer.atlas_image();
          if (aimg && !aimg->pixels.empty()) {
            int w = 0, h = 0;
            SDL_GetRendererOutputSize(ren, &w, &h);
            SDL_Rect target = fit_rect(static_cast<int>(aimg->width),
                                       static_cast<int>(aimg->height), w, h, 40);
            if (target.w > 0) {
              double sx = static_cast<double>(target.w) / aimg->width;
              double sy = static_cast<double>(target.h) / aimg->height;
              for (const auto& pl : viewer.atlas_placements()) {
                SDL_Rect pr = {target.x + static_cast<int>(pl.x * sx),
                               target.y + static_cast<int>(pl.y * sy),
                               static_cast<int>(pl.width * sx),
                               static_cast<int>(pl.height * sy)};
                if (ev.button.x >= pr.x && ev.button.x < pr.x + pr.w &&
                    ev.button.y >= pr.y && ev.button.y < pr.y + pr.h) {
                  select_frame_in_clip(viewer, pl.frame_id);
                  ds.mode = DisplayMode::frame;
                  break;
                }
              }
            }
          }
        }
      }
    }
    if (!running || !ren) break;

    // Deterministic package-data tick advancement (speed-scaled).
    const ViewerClipInfo* clip = viewer.clip(viewer.current_clip_id());
    if (clip && viewer.playback_state() == ViewerPlaybackState::playing) {
      const ViewerFrame vf = viewer.current_frame();
      std::uint32_t dur = vf.frame ? std::max(1u, vf.frame->duration_ticks) : 1u;
      ds.tick_accum += static_cast<double>(dur) * viewer.speed();
      while (ds.tick_accum >= 1.0) {
        ds.tick_accum -= 1.0;
        if (!viewer.looping() && viewer.current_tick() + 1 >= clip->duration_ticks) {
          viewer.pause();
          ds.tick_accum = 0.0;
          break;
        }
        viewer.seek_tick(viewer.current_tick() + 1);
      }
    } else {
      ds.tick_accum = 0.0;
    }

    int w = 0, h = 0;
    SDL_GetRendererOutputSize(ren, &w, &h);
    SDL_SetRenderDrawColor(ren, 0x10, 0x10, 0x18, 255);
    SDL_RenderClear(ren);

    switch (ds.mode) {
      case DisplayMode::frame:
        render_frame_mode(ren, w, h, viewer, ds);
        break;
      case DisplayMode::channel:
        render_channel_mode(ren, w, h, viewer, ds);
        break;
      case DisplayMode::atlas:
        render_atlas_mode(ren, w, h, viewer, ds);
        break;
    }

    draw_hud(ren, viewer, ds, w);
    if (ds.debug) draw_provenance_panel(ren, viewer);
    if (ds.morphology_panel) draw_morphology_panel(ren, viewer);

    SDL_RenderPresent(ren);

    // Smoke mode: deterministic scripted pass over modes, then clean exit.
    if (smoke) {
      const std::string frm = std::string(viewer.current_frame_id());
      const ViewerFrame vf = viewer.current_frame();
      std::cout << "SMOKE_FRAME " << frm
                << " w=" << (vf.frame ? vf.frame->image.width : 0)
                << " h=" << (vf.frame ? vf.frame->image.height : 0)
                << " hash=" << (vf.frame ? vf.frame->frame_hash : std::string())
                << " mode=" << static_cast<int>(ds.mode) << '\n';
      switch (ds.frame_count) {
        case 0: break;  // first rendered frame
        case 8:
          viewer.pause();
          (void)viewer.step_frame(-1);
          (void)viewer.step_frame(1);
          ds.mode = DisplayMode::channel;
          break;
        case 14: ds.mode = DisplayMode::atlas; ds.debug = true; break;
        case 20:
          ds.morphology_panel = true;
          ds.channel_index = viewer.channel_info().empty()
                                 ? 0
                                 : (viewer.channel_info().size() - 1);
          break;
        case 26: ds.mode = DisplayMode::frame; break;
        case 32:
          std::cout << "SMOKE_END clean exit after " << ds.frame_count
                    << " frames\n";
          running = false;
          break;
        default: break;
      }
    }

    ++ds.frame_count;
    std::uint32_t now = SDL_GetTicks();
    if (now - ds.last_fps_time >= 1000) {
      ds.fps = ds.frame_count;
      ds.frame_count = 0;
      ds.last_fps_time = now;
    }

    std::uint32_t elapsed = SDL_GetTicks() - frame_start;
    if (elapsed < 16) SDL_Delay(16 - elapsed);
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return running ? 0 : 0;
}

#endif  // GSPL_SPRITES_HAS_SDL2

}  // namespace

extern "C" int main(int argc, char* argv[]) {
  try {
    if (argc < 2) {
      std::cerr << "Usage: gspl_sprites_preview <package_path> [--smoke]\n";
      return 1;
    }

    std::filesystem::path package_path = argv[1];
    bool smoke = false;
    for (int i = 2; i < argc; ++i) {
      if (std::string_view(argv[i]) == "--smoke") smoke = true;
    }

    auto load_result = LivingPackageViewer::load(package_path);
    if (!load_result.ok()) {
      std::cerr << "Package load failed:\n";
      print_diagnostics(load_result.diagnostics);
      return 1;
    }

    auto& viewer = *load_result.value;

    int code;
#ifdef GSPL_SPRITES_HAS_SDL2
    code = sdl_main(viewer, smoke);
#else
    code = headless_main(viewer);
#endif

    return code;
  } catch (const std::exception& e) {
    std::cerr << "GSPL_SPRITES_PREVIEW_FATAL: " << e.what() << '\n';
    return 2;
  }
}
