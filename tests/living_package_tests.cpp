#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl/sdk.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

static int failures = 0;
static void check(bool v, const char* msg) {
  if (!v) { std::cerr << "FAIL: " << msg << "\n"; ++failures; }
  else { std::cout << "PASS: " << msg << "\n"; }
}

std::string load_source() {
  fs::path src(GSPL_SPRITES_SOURCE_DIR);
  src = src / "examples" / "living-voltfox" / "voltfox.gspl";
  std::ifstream in(src);
  if (!in) throw std::runtime_error("Cannot open: " + src.string());
  std::stringstream buf; buf << in.rdbuf();
  return buf.str();
}

int main() try {
  std::cout << "=== Living Package Tests ===\n\n";

  std::string src = load_source();
  check(src.size() > 1000, "source loaded");

  gspl::GsplContext ctx;
  auto buf = gspl::SourceBuffer::from_string("voltfox.gspl", src);
  ctx.compile_source(std::move(buf));
  check(!ctx.has_fatal_errors(), "compilation ok");

  gspl::sprites::SpriteSeed seed;
  seed = gspl::SpriteSeedLowering::lower(ctx.compilation_context().canonical);
  check(!seed.stable_id.empty(), "seed has id");

  gspl::sprites::LivingAnimation2dBuildResult living = gspl::sprites::synthesize_living_animation2d(seed);
  check(living.ok(), "synthesis ok");
  auto& lv = *living.value;
  check(lv.all_frames.size() == 48, "48 frames");
  check(lv.clips.size() == 9, "9 clips");
  check(lv.samples.size() == 48, "48 samples");
  check(!lv.base_morphology.empty(), "base morphology");
  check(!lv.storm_morphology.empty(), "storm morphology");
  check(lv.transformation_morphologies.size() == 10, "10 trans morphs");

  auto pkg_dir = fs::temp_directory_path() / "lv_pkg_test";
  fs::remove_all(pkg_dir);

  gspl::sprites::LivingVisualPackageInput pkg_input;
  pkg_input.seed = seed;
  pkg_input.frames = lv.all_frames;
  pkg_input.generated_clips = lv.clips;
  pkg_input.samples = lv.samples;
  pkg_input.events = lv.generated_events;
  pkg_input.channels = lv.channel_maps;
  pkg_input.collision_shapes = lv.collision_shapes;
  pkg_input.collision_windows = lv.collision_windows;
  pkg_input.base_morphology = lv.base_morphology;
  pkg_input.storm_morphology = lv.storm_morphology;
  pkg_input.transformation_morphologies = lv.transformation_morphologies;
  pkg_input.sheet = lv.sheet;

  gspl::sprites::build_living_visual_package(pkg_input, pkg_dir);
  check(fs::exists(pkg_dir / "manifest.json"), "manifest exists");
  check(fs::exists(pkg_dir / "seed.json"), "seed exists");
  check(fs::exists(pkg_dir / "animations-2d.json"), "animations-2d exists");
  check(fs::exists(pkg_dir / "animation-events.json"), "events exist");
  check(fs::exists(pkg_dir / "frame-samples.json"), "samples exist");
  check(fs::exists(pkg_dir / "pose-hashes.json"), "pose hashes exist");
  check(fs::exists(pkg_dir / "frame-hashes.json"), "frame hashes exist");
  check(fs::exists(pkg_dir / "collisions-2d.json"), "collisions exist");

  auto verify = gspl::sprites::verify_living_visual_package(pkg_dir);
  check(verify.ok(), "verification ok");
  check(verify.frame_count == 48, "verify: 48 frames");
  check(verify.clip_count == 9, "verify: 9 clips");
  check(verify.sample_count == 48, "verify: 48 samples");

  auto read = gspl::sprites::read_living_visual_package(pkg_dir);
  check(read.ok(), "read ok");

  fs::remove_all(pkg_dir);

  if (failures == 0)
    std::cout << "\nALL LIVING PACKAGE TESTS PASSED\n";
  else
    std::cerr << failures << " FAILURES\n";
  return failures ? 1 : 0;

} catch (std::exception const& e) {
  std::cerr << "FATAL: " << e.what() << "\n";
  return 1;
}
