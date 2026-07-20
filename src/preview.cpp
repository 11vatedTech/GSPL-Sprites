#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/living_runtime.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace gspl::sprites;

namespace {

std::string voltfox_seed() {
  return
    "schema=gspl.sprite-seed/0.1\n"
    "id=original.voltfox\n"
    "name=Voltfox\n"
    "classification=biological.fictional.electric-fox\n"
    "rights=ORIGINAL_USER_CREATION\n"
    "entropy_root=11072026\n"
    "primary_color=#242038\n"
    "accent_color=#56F1FF\n"
    "ability=directional-lightning|electric.projectile.directional|25|8|2\n"
    "rig=voltfox.rig\n"
    "bone=root|-|0|0|0|1|1|12|-45|45\n"
    "bone=head|root|4|0|0|1|1|8|-30|30\n"
    "bone=tail|root|-4|0|0|1|1|10|-80|80\n"
    "socket=muzzle|head|8|0|0|1|1\n"
    "clip=idle|10|true\n"
    "track=idle|head|0,4,0,-10,1,1;10,4,0,10,1,1\n"
    "clip_event=idle|blink|5\n"
    "clip=attack|2|false\n"
    "track=attack|head|0,4,0,0,1,1;2,6,0,25,1,1\n"
    "clip_event=attack|release|1\n"
    "initial_state=idle\n"
    "state=idle|idle\n"
    "state=attack|attack\n"
    "transition=idle|attack|attack|GREATER_EQUAL|1|0|1|10\n"
    "transition=attack|idle|attack|LESS|1|2|1|10\n"
    "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
    "collision=bolt|CIRCLE|muzzle|3|0|2|2\n"
    "collision_window=directional-lightning|bolt|0|2|true\n";
}

std::filesystem::path build_temp_package() {
  auto tmp = std::filesystem::temp_directory_path() / "gspl-preview";
  std::filesystem::remove_all(tmp);
  const auto seed = parse_seed(voltfox_seed());
  build_package(seed, tmp);
  return tmp;
}

void print_diagnostics(const ValidationResult& vr) {
  for (const auto& d : vr.diagnostics)
    std::cerr << "  " << d.code << ": " << d.message << '\n';
}

int headless_main(const PackageVerification& verification) {
  std::cout << "Package: entity=" << verification.entity_id
            << " identity=" << verification.package_identity
            << " artifacts=" << verification.artifact_count
            << " bytes=" << verification.total_artifact_bytes
            << '\n';

  LivingRuntimeProgram prog;
  prog.id = "voltfox.runtime";
  prog.ticks_per_second = 60;
  prog.maximum_memory_records = 1024;
  prog.goals = {
    {"idle", 0, {}},
    {"defend", 100, {{"perception.threat", Comparison::greater_equal, 1}}}
  };
  prog.actions = {
    {"observe", "idle", 1, {}, {}, 1, 0, 0, true, {}},
    {"attack", "defend", 10,
     {{"perception.threat", 500000}},
     {{"confidence.threat", Comparison::greater_equal, 500000}},
     2, 3, 20, true, {{"release", 0}}}
  };

  LivingRuntimeState state;
  state.energy = 100;

  for (std::uint64_t t = 0; t < 100; ++t) {
    if (t == 5)
      observe(state, prog, {"threat", "enemy", 2, 900000, state.tick, 2});

    auto result = step_living_runtime(prog, state);

    if (t % 10 == 0 || t == 99) {
      std::cout << "tick " << t
                << " energy=" << state.energy
                << " action=";
      if (state.active_action)
        std::cout << state.active_action->action_id;
      else
        std::cout << "(none)";
      if (result.selected_action)
        std::cout << " selected=" << *result.selected_action;
      std::cout << '\n';
    }
  }

  std::cout << "Completed 100 ticks — energy=" << state.energy << '\n';
  return 0;
}

#ifdef GSPL_SPRITES_HAS_SDL2

#include <SDL.h>
#include <array>
#include <cmath>
#include <cstdio>

// 8x8 bitmap font glyphs (uint64_t, MSB-left per row, row0 in high byte)
static std::uint64_t font_glyph(unsigned char ch) {
  static const std::uint64_t glyphs[] = {
    [' '] = 0x0000000000000000ULL,
    ['0'] = 0x3C666E7E76663C00ULL,
    ['1'] = 0x1838181818187E00ULL,
    ['2'] = 0x3C66060C18307E00ULL,
    ['3'] = 0x3C66061C06663C00ULL,
    ['4'] = 0x0C1C3C6C7E0C0C00ULL,
    ['5'] = 0x7E607C0606663C00ULL,
    ['6'] = 0x3C607C6666663C00ULL,
    ['7'] = 0x7E060C1818181800ULL,
    ['8'] = 0x3C66663C66663C00ULL,
    ['9'] = 0x3C66663E06663C00ULL,
    [':'] = 0x0000180000180000ULL,
    ['%'] = 0x6322180C30637C00ULL,
    ['F'] = 0x7E60607C60606000ULL,
    ['P'] = 0x7C66667C60606000ULL,
    ['S'] = 0x3C66603C06663C00ULL,
    ['c'] = 0x00001E3030301E00ULL,
    ['f'] = 0x0C183E1818181800ULL,
    ['h'] = 0x0000666666663E00ULL,
    ['i'] = 0x1800181818181800ULL,
    ['k'] = 0x0000666C786C6600ULL,
    ['m'] = 0x00006B7F7F636300ULL,
    ['o'] = 0x00003C6666663C00ULL,
    ['p'] = 0x00007C66667C6060ULL,
    ['r'] = 0x00001E3030303000ULL,
    ['t'] = 0x18007E1818180E00ULL,
  };
  if (ch < 128)
    return glyphs[ch];
  return 0;
}

static void draw_char(SDL_Renderer* ren, int x, int y, unsigned char ch,
                      std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  auto gdata = font_glyph(ch);
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

struct PreviewState {
  std::string mode_name = "2D";
  int mode_index = 0;
  bool debug = false;
  std::uint64_t tick = 0;
  int fps = 0;
  int frame_count = 0;
  std::uint32_t last_fps_time = 0;
};

static void render_2d(SDL_Renderer* ren, int w, int h) {
  SDL_Rect rect = {w / 2 - 100, h / 2 - 80, 200, 160};
  SDL_SetRenderDrawColor(ren, 0x24, 0x20, 0x38, 255);
  SDL_RenderFillRect(ren, &rect);
  SDL_SetRenderDrawColor(ren, 0x56, 0xF1, 0xFF, 255);
  SDL_RenderDrawRect(ren, &rect);
}

static void render_25d(SDL_Renderer* ren, int w, int h, int tick) {
  int offset1 = (tick * 2) % 80;
  int offset2 = (tick * 4) % 120;
  int offset3 = (tick * 1) % 60;

  SDL_SetRenderDrawColor(ren, 0x24, 0x20, 0x38, 180);
  SDL_Rect bg = {0, 0, w, h};
  SDL_RenderFillRect(ren, &bg);

  SDL_SetRenderDrawColor(ren, 0x80, 0x40, 0x20, 160);
  SDL_Rect l1 = {w / 2 - 180 + offset1 - 40, h / 2 - 60, 120, 80};
  SDL_RenderFillRect(ren, &l1);

  SDL_SetRenderDrawColor(ren, 0x56, 0xF1, 0xFF, 200);
  SDL_Rect l2 = {w / 2 - 60 + offset2 / 2, h / 2 - 40, 100, 60};
  SDL_RenderFillRect(ren, &l2);

  SDL_SetRenderDrawColor(ren, 0x24, 0x20, 0x38, 220);
  SDL_Rect l3 = {w / 2 - 150 + offset3, h / 2 + 20, 200, 40};
  SDL_RenderFillRect(ren, &l3);
}

struct Vec3 { double x, y, z; };

static Vec3 rotate_x(const Vec3& v, double a) {
  double c = std::cos(a), s = std::sin(a);
  return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}
static Vec3 rotate_y(const Vec3& v, double a) {
  double c = std::cos(a), s = std::sin(a);
  return {v.x * c + v.z * s, v.y, -v.x * s + v.z * c};
}

static void render_3d(SDL_Renderer* ren, int w, int h, int tick) {
  static const Vec3 cube[8] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
  };
  static const int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
  };

  double angle = tick * 0.02;
  Vec3 projected[8];
  for (int i = 0; i < 8; ++i) {
    Vec3 v = rotate_y(rotate_x(cube[i], angle * 0.7), angle);
    double scale = 80.0 / (v.z + 4.0);
    projected[i].x = w / 2.0 + v.x * scale;
    projected[i].y = h / 2.0 + v.y * scale;
  }

  SDL_SetRenderDrawColor(ren, 0x56, 0xF1, 0xFF, 255);
  for (auto& e : edges) {
    SDL_RenderDrawLine(ren,
      static_cast<int>(projected[e[0]].x), static_cast<int>(projected[e[0]].y),
      static_cast<int>(projected[e[1]].x), static_cast<int>(projected[e[1]].y));
  }
}

static void render_debug(SDL_Renderer* ren, int w, int h,
                         const PreviewState& ps,
                         const PackageVerification& verification) {
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
  SDL_Rect bg = {4, 4, 280, 140};
  SDL_RenderFillRect(ren, &bg);

  char line[128];
  std::snprintf(line, sizeof(line), "Form: %s", verification.entity_id.c_str());
  draw_text(ren, 12, 10, line, 86, 241, 255);

  std::snprintf(line, sizeof(line), "HP:  100%%%%");
  draw_text(ren, 12, 22, line, 86, 241, 255);

  std::snprintf(line, sizeof(line), "Tick: %llu", static_cast<unsigned long long>(ps.tick));
  draw_text(ren, 12, 34, line, 86, 241, 255);

  std::snprintf(line, sizeof(line), "FPS:  %d", ps.fps);
  draw_text(ren, 12, 46, line, 86, 241, 255);

  draw_text(ren, 12, 62, "Mode: ", 86, 241, 255);
  draw_text(ren, 12 + 5 * 10, 62, ps.mode_name.c_str(), 255, 200, 100);

  draw_text(ren, 12, 78, "2-2D 3-3D 5-2.5D", 150, 150, 150);
  draw_text(ren, 12, 90, "D-debug Q-quit", 150, 150, 150);
}

static int sdl_main(const std::filesystem::path& package_path,
                    const PackageVerification& verification) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cerr << "SDL_Init: " << SDL_GetError() << '\n';
    return 1;
  }

  SDL_Window* win = SDL_CreateWindow("GSPL Sprites Preview",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600,
    SDL_WINDOW_SHOWN);
  if (!win) {
    std::cerr << "SDL_CreateWindow: " << SDL_GetError() << '\n';
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* ren = SDL_CreateRenderer(win, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!ren) {
    std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << '\n';
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 1;
  }

  PreviewState ps;
  const char* modes[] = {"2D", "2.5D", "3D"};
  ps.mode_name = modes[ps.mode_index];

  LivingRuntimeProgram prog;
  prog.id = "voltfox.runtime";
  prog.ticks_per_second = 60;
  prog.maximum_memory_records = 1024;
  prog.goals = {
    {"idle", 0, {}},
    {"defend", 100, {{"perception.threat", Comparison::greater_equal, 1}}}
  };
  prog.actions = {
    {"observe", "idle", 1, {}, {}, 1, 0, 0, true, {}},
    {"attack", "defend", 10,
     {{"perception.threat", 500000}},
     {{"confidence.threat", Comparison::greater_equal, 500000}},
     2, 3, 20, true, {{"release", 0}}}
  };

  LivingRuntimeState rstate;
  rstate.energy = 100;

  std::cout << "Preview: entity=" << verification.entity_id
            << " package=" << verification.package_identity
            << " artifacts=" << verification.artifact_count << '\n';
  std::cout << "Controls: 2=2D 3=3D 5=2.5D D=debug Q/ESC=quit\n";

  bool running = true;
  ps.last_fps_time = SDL_GetTicks();
  std::uint64_t step = 0;

  while (running) {
    std::uint32_t frame_start = SDL_GetTicks();

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_QUIT) {
        running = false;
      } else if (ev.type == SDL_KEYDOWN) {
        switch (ev.key.keysym.sym) {
          case SDLK_2: ps.mode_index = 0; ps.mode_name = "2D"; break;
          case SDLK_3: ps.mode_index = 2; ps.mode_name = "3D"; break;
          case SDLK_5: ps.mode_index = 1; ps.mode_name = "2.5D"; break;
          case SDLK_d: ps.debug = !ps.debug; break;
          case SDLK_q:
          case SDLK_ESCAPE: running = false; break;
          default: break;
        }
      }
    }

    if (step == 5)
      observe(rstate, prog, {"threat", "enemy", 2, 900000, rstate.tick, 2});

    auto result = step_living_runtime(prog, rstate);
    static_cast<void>(result);
    ps.tick = rstate.tick;
    ++step;

    int w, h;
    SDL_GetRendererOutputSize(ren, &w, &h);

    SDL_SetRenderDrawColor(ren, 0x10, 0x10, 0x18, 255);
    SDL_RenderClear(ren);

    switch (ps.mode_index) {
      case 0: render_2d(ren, w, h); break;
      case 1: render_25d(ren, w, h, static_cast<int>(ps.tick)); break;
      case 2: render_3d(ren, w, h, static_cast<int>(ps.tick)); break;
    }

    if (ps.debug)
      render_debug(ren, w, h, ps, verification);

    SDL_RenderPresent(ren);

    ++ps.frame_count;
    std::uint32_t now = SDL_GetTicks();
    if (now - ps.last_fps_time >= 1000) {
      ps.fps = ps.frame_count;
      ps.frame_count = 0;
      ps.last_fps_time = now;
    }

    std::uint32_t elapsed = SDL_GetTicks() - frame_start;
    if (elapsed < 16)
      SDL_Delay(16 - elapsed);
  }

  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
  return 0;
}

#endif // GSPL_SPRITES_HAS_SDL2

} // namespace

int main(int argc, char* argv[]) {
  try {
    std::filesystem::path package_path;
    bool cleanup_temp = false;

    if (argc >= 2) {
      package_path = argv[1];
    } else {
      std::cout << "No package path provided — building temp package from voltfox seed\n";
      package_path = build_temp_package();
      cleanup_temp = true;
    }

    auto verification = verify_package(package_path);
    if (!verification.ok()) {
      std::cerr << "Package verification failed:\n";
      print_diagnostics(verification.validation);
      if (cleanup_temp)
        std::filesystem::remove_all(package_path);
      return 1;
    }

    int code;
#ifdef GSPL_SPRITES_HAS_SDL2
    code = sdl_main(package_path, verification);
#else
    code = headless_main(verification);
#endif

    if (cleanup_temp)
      std::filesystem::remove_all(package_path);

    return code;
  } catch (const std::exception& e) {
    std::cerr << "GSPL_SPRITES_PREVIEW_FATAL: " << e.what() << '\n';
    return 2;
  }
}
