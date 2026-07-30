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
static int assertions = 0;
static void check(bool v, const char* msg) {
  ++assertions;
  if (!v) { std::cerr << "FAIL: " << msg << "\n"; ++failures; }
  else { std::cout << "PASS: " << msg << "\n"; }
}
static std::string read_file_bytes(std::filesystem::path const& p, std::uint64_t max_bytes) {
  std::ifstream in(p, std::ios::binary | std::ios::ate);
  if (!in) throw std::runtime_error("cannot open: " + p.string());
  auto sz = static_cast<std::uint64_t>(in.tellg());
  if (sz > max_bytes) throw std::runtime_error("file too large: " + p.string());
  in.seekg(0, std::ios::beg);
  std::string out(static_cast<std::size_t>(sz), '\0');
  in.read(out.data(), static_cast<std::streamsize>(sz));
  return out;
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

  // ── Scope 1: compile, synthesize, build, record identities ──
  auto pkg_dir = fs::temp_directory_path() / "lv_pkg_test";
  fs::remove_all(pkg_dir);

  std::string expected_seed_id;
  std::string expected_seed_json;
  std::string expected_pkg_id;
  std::set<std::string> expected_frame_ids;
  std::map<std::string, std::string> expected_frame_hash_map;

  {
    gspl::GsplContext ctx;
    auto buf = gspl::SourceBuffer::from_string("voltfox.gspl", src);
    ctx.compile_source(std::move(buf));
    check(!ctx.has_fatal_errors(), "compilation ok");

    gspl::sprites::SpriteSeed seed;
    seed = gspl::SpriteSeedLowering::lower(ctx.compilation_context().canonical);
    check(!seed.stable_id.empty(), "seed has id");
    expected_seed_json = gspl::sprites::canonicalize(seed);
    expected_seed_id = gspl::sprites::sha256(expected_seed_json);

    gspl::sprites::LivingAnimation2dBuildResult living = gspl::sprites::synthesize_living_animation2d(seed);
    check(living.ok(), "synthesis ok");
    auto& lv = *living.value;
    check(lv.all_frames.size() == 48, "48 frames");
    check(lv.clips.size() == 9, "9 clips");
    check(lv.samples.size() == 48, "48 samples");
    check(!lv.base_morphology.empty(), "base morphology");
    check(!lv.storm_morphology.empty(), "storm morphology");
    check(lv.transformation_morphologies.size() == 10, "10 trans morphs");

    // Record expected frame identities before destruction
    for (auto const& f : lv.all_frames) {
      expected_frame_ids.insert(f.id);
      expected_frame_hash_map[f.id] = f.frame_hash;
    }

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
    check(fs::exists(pkg_dir / "frames.json"), "frames.json exists");
    check(fs::exists(pkg_dir / "animations-2d.json"), "animations-2d exists");
    check(fs::exists(pkg_dir / "animation-events.json"), "events exist");
    check(fs::exists(pkg_dir / "frame-samples.json"), "samples exist");
    check(fs::exists(pkg_dir / "pose-hashes.json"), "pose hashes exist");
    check(fs::exists(pkg_dir / "frame-hashes.json"), "frame hashes exist");
    check(fs::exists(pkg_dir / "collisions-2d.json"), "collisions exist");
  }
  // ── End Scope 1: all source objects destroyed ──

  // ── Scope 2: read from filesystem only, verify, inspect ──
  {
    auto verify = gspl::sprites::verify_living_visual_package(pkg_dir);
    check(verify.ok(), "verification ok");
    check(verify.frame_count == 48, "verify: 48 frames");
    check(verify.clip_count == 9, "verify: 9 clips");
    check(verify.sample_count == 48, "verify: 48 samples");

    auto read = gspl::sprites::read_living_visual_package(pkg_dir);
    check(read.ok(), "read ok");
    if (read.value) {
      auto& pkg = *read.value;
      check(!pkg.entity_id.empty(), "reconstructed entity_id");
      check(!pkg.seed_identity.empty(), "reconstructed seed_identity");
      check(!pkg.package_identity.empty(), "reconstructed package_identity");
      check(!pkg.seed.stable_id.empty(), "reconstructed seed.stable_id");

      // Verify seed identity matches expected (recorded before object destruction)
      check(pkg.seed_identity == expected_seed_id, "seed identity matches expected");

      // Canonical seed round-trip: prove byte-identical reconstruction
      auto reconstructed_seed_json = gspl::sprites::canonicalize(pkg.seed);
      check(reconstructed_seed_json == expected_seed_json, "canonical seed round-trip byte-identical");
      check(gspl::sprites::sha256(reconstructed_seed_json) == expected_seed_id, "reconstructed seed SHA matches");

      check(!pkg.seed.abilities.empty(), "seed has abilities");
      check(!pkg.seed.forms.empty(), "seed has forms");
      check(!pkg.seed.morphology.empty(), "seed has morphology");
      check(!pkg.seed.transformations.empty(), "seed has transformations");
      check(!pkg.seed.collision_shapes.empty(), "seed has collision_shapes");

      // Verify frame reconstruction from disk PNGs
      check(pkg.frames.size() == 48, "reconstructed 48 frames");
      std::set<std::string> reconstructed_fids;
      for (auto const& f : pkg.frames) {
        check(!f.id.empty(), "reconstructed frame has id");
        check(!f.frame_hash.empty(), "reconstructed frame has hash");
        check(f.image.width > 0, "reconstructed frame has width");
        check(f.image.height > 0, "reconstructed frame has height");
        check(expected_frame_ids.contains(f.id), "reconstructed frame id was expected");
        reconstructed_fids.insert(f.id);
      }
      check(reconstructed_fids == expected_frame_ids, "all expected frame IDs reconstructed");

      // Verify frame hashes by ID: each frame hash must match expected
      for (auto const& f : pkg.frames) {
        auto it = expected_frame_hash_map.find(f.id);
        check(it != expected_frame_hash_map.end() && it->second == f.frame_hash, "reconstructed frame hash matches expected by ID");
      }

      check(pkg.generated_clips.size() == 9, "reconstructed 9 clips");
      check(pkg.samples.size() == 48, "reconstructed 48 samples");
      check(!pkg.base_morphology.empty(), "reconstructed base morphology");
      check(!pkg.storm_morphology.empty(), "reconstructed storm morphology");
      check(pkg.transformation_morphologies.size() == 10, "reconstructed 10 trans morphs");
    }
  }

  // ── Mutation tests ──
  {
    // M1: corrupt a frame PNG byte
    {
      auto mut_dir = pkg_dir; mut_dir += "_m1"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      for (auto const& e : fs::directory_iterator(mut_dir/"frames")) {
        if (e.is_regular_file() && e.path().extension() == ".png") {
          std::fstream f(e.path(), std::ios::binary | std::ios::in | std::ios::out);
          if (f) { f.seekp(12); f.put(static_cast<char>(0xFF)); f.close(); break; }
        }
      }
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M1: corrupted frame PNG fails");
      fs::remove_all(mut_dir);
    }
    // M2: corrupt seed-identity.txt
    {
      auto mut_dir = pkg_dir; mut_dir += "_m2"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      std::ofstream(mut_dir/"seed-identity.txt", std::ios::trunc) << "0000000000000000000000000000000000000000000000000000000000000000";
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M2: wrong seed identity fails");
      fs::remove_all(mut_dir);
    }
    // M3: missing required artifact
    {
      auto mut_dir = pkg_dir; mut_dir += "_m3"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      fs::remove(mut_dir/"animations-2d.json");
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M3: missing animations-2d fails");
      fs::remove_all(mut_dir);
    }
    // M4: change frames.json schema to wrong value
    {
      auto mut_dir = pkg_dir; mut_dir += "_m4"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto fm_bytes = read_file_bytes(mut_dir/"frames.json", 512ULL*1024*1024);
      std::string fm(fm_bytes.begin(), fm_bytes.end());
      auto pos = fm.find("gspl.frames-2d/0.1");
      if (pos != std::string::npos) fm.replace(pos, 17, "bad-schema/0.0");
      std::ofstream(mut_dir/"frames.json", std::ios::trunc | std::ios::binary).write(fm.data(), fm.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M4: wrong frames schema fails");
      fs::remove_all(mut_dir);
    }
    // M5: delete a frame PNG
    {
      auto mut_dir = pkg_dir; mut_dir += "_m5"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      for (auto const& e : fs::directory_iterator(mut_dir/"frames")) {
        if (e.is_regular_file() && e.path().extension() == ".png") { fs::remove(e.path()); break; }
      }
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M5: missing frame PNG fails");
      fs::remove_all(mut_dir);
    }
    // M6: change a frame hash in frames.json
    {
      auto mut_dir = pkg_dir; mut_dir += "_m6"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto fm_bytes = read_file_bytes(mut_dir/"frames.json", 512ULL*1024*1024);
      std::string fm(fm_bytes.begin(), fm_bytes.end());
      auto pos = fm.find("\"frame_hash\":\"");
      if (pos != std::string::npos) fm.replace(pos+14, 64, std::string(64, '0'));
      std::ofstream(mut_dir/"frames.json", std::ios::trunc | std::ios::binary).write(fm.data(), fm.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M6: wrong frame hash fails");
      fs::remove_all(mut_dir);
    }
    // M7: delete a sample from frame-samples.json
    {
      auto mut_dir = pkg_dir; mut_dir += "_m7"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto fs_bytes = read_file_bytes(mut_dir/"frame-samples.json", 512ULL*1024*1024);
      std::string fs_s(fs_bytes.begin(), fs_bytes.end());
      auto start = fs_s.find("{\"clip_id\"");
      if (start != std::string::npos) {
        auto end = fs_s.find("}", start) + 1;
        if (fs_s[end] == ',') ++end;
        fs_s.erase(start, end - start);
      }
      std::ofstream(mut_dir/"frame-samples.json", std::ios::trunc | std::ios::binary).write(fs_s.data(), fs_s.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M7: deleted sample fails");
      fs::remove_all(mut_dir);
    }
    // M8: change an event frame_id
    {
      auto mut_dir = pkg_dir; mut_dir += "_m8"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto ev_bytes = read_file_bytes(mut_dir/"animation-events.json", 512ULL*1024*1024);
      std::string ev(ev_bytes.begin(), ev_bytes.end());
      auto pos = ev.find("\"frame_id\":\"");
      if (pos != std::string::npos) ev.replace(pos+12, 4, "XXXX");
      std::ofstream(mut_dir/"animation-events.json", std::ios::trunc | std::ios::binary).write(ev.data(), ev.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M8: wrong event frame_id fails");
      fs::remove_all(mut_dir);
    }
    // M9: add undeclared file
    {
      auto mut_dir = pkg_dir; mut_dir += "_m9"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      std::ofstream(mut_dir/"extra.dat") << "undeclared";
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M9: undeclared file fails");
      fs::remove_all(mut_dir);
    }
    // M10: corrupt manifest artifact hash
    {
      auto mut_dir = pkg_dir; mut_dir += "_m10"; fs::remove_all(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto mf_bytes = read_file_bytes(mut_dir/"manifest.json", 4ULL*1024*1024);
      std::string mf(mf_bytes.begin(), mf_bytes.end());
      auto pos = mf.find("\"sha256\":\"");
      if (pos != std::string::npos) mf.replace(pos+10, 64, std::string(64, '0'));
      std::ofstream(mut_dir/"manifest.json", std::ios::trunc | std::ios::binary).write(mf.data(), mf.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M10: corrupt manifest hash fails");
      fs::remove_all(mut_dir);
    }
  }

  fs::remove_all(pkg_dir);

  std::cout << "\n=== LIVING PACKAGE TESTS: " << assertions << " assertions, " << failures << " failures ===\n";
  return failures ? 1 : 0;

} catch (std::exception const& e) {
  std::cerr << "FATAL: " << e.what() << "\n";
  return 1;
}
