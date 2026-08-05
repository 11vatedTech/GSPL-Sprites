#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/viewer_model.hpp"
#include "gspl/sdk.hpp"
#include "gspl/lowering.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
static int failures = 0;

static void check(bool v, const char* msg) {
  if (!v) { std::fprintf(stderr, "FAIL: %s\n", msg); ++failures; }
  else { std::fprintf(stdout, "PASS: %s\n", msg); }
}

static std::string read_file(fs::path const& p) {
  std::ifstream in(p, std::ios::binary | std::ios::ate);
  if (!in) throw std::runtime_error("open fail");
  auto sz = static_cast<std::uint64_t>(in.tellg());
  in.seekg(0);
  std::string out(static_cast<std::size_t>(sz), 0);
  in.read(out.data(), static_cast<std::streamsize>(sz));
  return out;
}

static void remove_all_retry(fs::path const& p) {
  for (int attempt = 0; attempt < 10; ++attempt) {
    std::error_code ec;
    fs::remove_all(p, ec);
    if (!fs::exists(p)) return;
    if (attempt > 0) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  fs::remove_all(p);
}

int main() try {
  using namespace gspl::sprites;

  auto ts = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  auto tmp = fs::temp_directory_path() / "gspl-sprites" / ("vi-" + ts);
  fs::create_directories(tmp);
  fs::path pkg_dir = tmp / "pkg";
  remove_all_retry(pkg_dir.string() + ".staging");

  /* Use GSPL compiler (like living_package_tests) */
  fs::path src_path(GSPL_SPRITES_SOURCE_DIR);
  src_path /= "examples/living-voltfox/voltfox.gspl";
  std::string src = read_file(src_path);

  {
    gspl::GsplContext ctx;
    ctx.compile_source(gspl::SourceBuffer::from_string("voltfox.gspl", src));
    check(!ctx.has_fatal_errors(), "vi: compile ok");

    SpriteSeed seed = gspl::SpriteSeedLowering::lower(ctx.compilation_context().canonical);
    check(!seed.stable_id.empty(), "vi: seed id");

    LivingAnimation2dBuildResult living = synthesize_living_animation2d(seed);
    check(living.ok(), "vi: synthesis ok");
    auto& lv = *living.value;
    check(lv.all_frames.size() == 48, "vi: 48 frames");
    check(lv.clips.size() == 9, "vi: 9 clips");
    check(!lv.base_morphology.empty(), "vi: base morph");
    check(lv.transformation_morphologies.size() == 10, "vi: 10 trans morphs");

    LivingVisualPackageInput in;
    in.seed = seed; in.frames = lv.all_frames; in.generated_clips = lv.clips;
    in.samples = lv.samples; in.events = lv.generated_events;
    in.channels = lv.channel_maps;
    in.collision_shapes = lv.collision_shapes;
    in.collision_windows = lv.collision_windows;
    in.base_morphology = lv.base_morphology;
    in.storm_morphology = lv.storm_morphology;
    in.transformation_morphologies = lv.transformation_morphologies;
    in.sheet = lv.sheet;
    build_living_visual_package(in, pkg_dir);
  }

  LivingPackageViewer::LoadResult lr = LivingPackageViewer::load(pkg_dir);
  check(lr.ok(), "vi: viewer loads package");
  LivingPackageViewer& v = *lr.value;

  check(v.frame_count() == 48, "vi: viewer 48 frames");
  check(v.clip_count() == 9, "vi: viewer 9 clips");
  check(v.atlas_placements().size() == 48, "vi: viewer 48 atlas");
  check(!v.base_morphology().empty(), "vi: viewer base morph");
  check(!v.storm_morphology().empty(), "vi: viewer storm morph");
  check(v.transformation_morphology_count() == 10, "vi: viewer 10 trans");

  const ViewerProvenance& prov = v.provenance();
  check(!prov.entity_id.empty(), "vi: entity id");
  check(!prov.package_identity.empty(), "vi: package id");

  if (!v.clips().empty()) {
    std::string cid = v.clips()[0].clip_id;
    check(v.select_clip(cid), "vi: select clip");
    std::vector<std::string> s1, s2;
    for (std::uint32_t t = 0; t < 5; ++t) s1.push_back(v.frame_for_tick(cid, t).frame_id);
    v.stop();
    for (std::uint32_t t = 0; t < 5; ++t) s2.push_back(v.frame_for_tick(cid, t).frame_id);
    check(s1 == s2, "vi: playback deterministic");
  }

  std::string mut = pkg_dir.string() + ".mut";
  fs::remove_all(mut); fs::copy(pkg_dir, mut);
  { std::ofstream o(mut + "/manifest.json", std::ios::trunc); o << "{bad"; }
  check(!LivingPackageViewer::load(mut).ok(), "vi: corrupt fails closed");
  fs::remove_all(mut);

  remove_all_retry(tmp);
  std::printf("\n%d failures\n", failures);
  return failures != 0 ? 1 : 0;
} catch (std::exception const& e) {
  std::fprintf(stderr, "FATAL: %s\n", e.what());
  return 2;
}
