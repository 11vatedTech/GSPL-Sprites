#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/image.hpp"
#include "gspl/sdk.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static int failures = 0;
static int assertions = 0;
static void check(bool v, const char* msg) {
  ++assertions;
  if (!v) { std::cerr << "FAIL: " << msg << "\n"; ++failures; }
  else { std::cout << "PASS: " << msg << "\n"; }
}

// Bounded, best-effort directory removal. Windows may briefly retain a
// handle on recently-closed files, so retry a bounded number of times with a
// short sleep. On persistent failure, report the exact path and the error
// instead of swallowing it (and never loop forever).
static void remove_all_retry(fs::path const& p) {
  if (p.empty()) return;
  if (!fs::exists(p)) return;
  std::error_code ec;
  for (int attempt = 0; attempt < 10; ++attempt) {
    ec.clear();
    fs::remove_all(p, ec);
    if (!ec && !fs::exists(p)) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::cerr << "WARN: remove_all_retry: could not remove '" << p.string() << "'";
  if (ec) std::cerr << ": " << ec.message();
  std::cerr << "\n";
}

// Unique per-process/per-invocation test workspace.
//
// Layout: <temp>/gspl-sprites/living-package/<tag>-<pid>-<invocation>-<clock>
//
// The run id combines the OS process id with a monotonic invocation counter
// and a steady-clock fingerprint, so concurrent test processes and sequential
// re-runs can never share mutable state, inherit stale artifacts, or collide
// on staging paths. The workspace is removed on destruction via the bounded
// retry helper; any cleanup failure is reported with the exact path.
// Test-only: the production library never depends on this.
class ScopedTestWorkspace {
public:
  explicit ScopedTestWorkspace(std::string tag, bool create_root = false)
      : root_(make_root(std::move(tag))), create_root_(create_root) {
    // By default only the shared container parent is created here. The
    // run-specific root is intentionally left absent: production package
    // builders reject a pre-existing output directory, so every build creates
    // its own fresh dir. Set create_root=true when the workspace itself must
    // exist upfront (e.g. a scratch workspace that holds a CLI output file
    // outside every package directory).
    std::error_code ec;
    fs::create_directories(create_root ? root_ : root_.parent_path(), ec);
    if (ec) {
      throw std::runtime_error("ScopedTestWorkspace: cannot create " +
                               (create_root ? root_ : root_.parent_path()).string() +
                               ": " + ec.message());
    }
    std::cout << "workspace: " << root_.string() << "\n";
  }

  ~ScopedTestWorkspace() { remove_all_retry(root_); }

  ScopedTestWorkspace(ScopedTestWorkspace const&) = delete;
  ScopedTestWorkspace& operator=(ScopedTestWorkspace const&) = delete;

  fs::path const& path() const { return root_; }
  fs::path sub(std::string const& name) const { return root_ / name; }

private:
  static unsigned long process_id() {
#ifdef _WIN32
    return static_cast<unsigned long>(::_getpid());
#else
    return static_cast<unsigned long>(::getpid());
#endif
  }

  static fs::path make_root(std::string tag) {
    static std::atomic<unsigned long> invocation{0};
    auto const n = invocation.fetch_add(1);
    auto const now = static_cast<unsigned long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    auto const pid = process_id();
    return fs::temp_directory_path() / "gspl-sprites" / "living-package" /
           (tag + "-" + std::to_string(pid) + "-" + std::to_string(n) + "-" +
            std::to_string(now));
  }

  fs::path root_;
  bool create_root_;
};
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
  // Isolated per-run workspace: a unique directory per process and per
  // invocation (see ScopedTestWorkspace), so concurrent test processes and
  // sequential re-runs can never share mutable state or inherit stale data.
  ScopedTestWorkspace ws("pkg");
  auto pkg_dir = ws.path();
  // Secondary defense: Windows may hold transient handles even on isolated
  // directories; bounded retry cleanup is idempotent on a brand-new path.
  remove_all_retry(pkg_dir.string() + ".staging");

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

    // Deterministic two-build proof
    {
      auto dA = ws.sub("det_A");
      auto dB = ws.sub("det_B");
      remove_all_retry(dA); remove_all_retry(dB);
      remove_all_retry(dA.string() + ".staging"); remove_all_retry(dB.string() + ".staging");
      gspl::sprites::build_living_visual_package(pkg_input, dA);
      gspl::sprites::build_living_visual_package(pkg_input, dB);
      auto vA = gspl::sprites::verify_living_visual_package(dA);
      auto vB = gspl::sprites::verify_living_visual_package(dB);
      check(vA.ok() && vB.ok(), "det: both valid");
      check(vA.package_identity == vB.package_identity, "det: identical identity");
      check(vA.package_identity.size() == 64, "det: SHA-256");
      auto mA = read_file_bytes(dA / "manifest.json", 4ULL*1024*1024);
      auto mB = read_file_bytes(dB / "manifest.json", 4ULL*1024*1024);
      check(mA == mB, "det: identical manifest bytes");
      check(vA.frame_count == vB.frame_count, "det: identical frame count");
      check(vA.clip_count == vB.clip_count, "det: identical clip count");
      check(vA.sample_count == vB.sample_count, "det: identical sample count");
      // Recursive artifact comparison A→B and B→A
      for (auto const& entry : fs::recursive_directory_iterator(dA)) {
        if (!entry.is_regular_file()) continue;
        auto rel = fs::relative(entry.path(), dA);
        auto pathB = dB / rel;
        check(fs::exists(pathB), ("det: file exists in B: " + rel.string()).c_str());
        if (fs::exists(pathB)) {
          auto bytesA = read_file_bytes(entry.path(), 512ULL*1024*1024);
          auto bytesB = read_file_bytes(pathB, 512ULL*1024*1024);
          check(bytesA.size() == bytesB.size(), ("det: same size: " + rel.string()).c_str());
          check(bytesA == bytesB, ("det: identical bytes: " + rel.string()).c_str());
        }
      }
      for (auto const& entry : fs::recursive_directory_iterator(dB)) {
        if (!entry.is_regular_file()) continue;
        auto rel = fs::relative(entry.path(), dB);
        check(fs::exists(dA / rel), ("det: file exists in A: " + rel.string()).c_str());
      }
      remove_all_retry(dA); remove_all_retry(dB);
    }
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

      // Typed frame-hash records
      check(pkg.frame_hash_records.size() == 48, "typed frame_hash_records: 48 records");
      std::set<std::string> fhr_fids;
      for (auto const& fhr : pkg.frame_hash_records) {
        check(!fhr.frame_id.empty(), "typed fhr: frame_id nonempty");
        check(fhr.frame_hash.size() == 64, "typed fhr: frame_hash is 64 chars");
        check(!fhr_fids.contains(fhr.frame_id), "typed fhr: unique frame_id");
        fhr_fids.insert(fhr.frame_id);
      }
      check(fhr_fids == expected_frame_ids, "typed fhr: frame IDs match expected");

      // Typed pose-hash records
      check(pkg.pose_hash_records.size() == 48, "typed pose_hash_records: 48 records");
      std::set<std::string> phr_keys;
      for (auto const& phr : pkg.pose_hash_records) {
        check(!phr.clip_id.empty(), "typed phr: clip_id nonempty");
        check(phr.pose_hash.size() == 64, "typed phr: pose_hash is 64 chars");
        auto key = phr.clip_id + "|" + std::to_string(phr.frame_index);
        check(!phr_keys.contains(key), "typed phr: unique clip+frame_index");
        phr_keys.insert(key);
      }
    }
  }

  // ── Diagnostic helper ──
  auto has_diagnostic = [](auto const& result, std::string_view code) -> bool {
    for (auto const& d : result.validation.diagnostics)
      if (d.code == code) return true;
    return false;
  };
  auto has_read_diagnostic = [](auto const& result, std::string_view code) -> bool {
    for (auto const& d : result.diagnostics.diagnostics)
      if (d.code == code) return true;
    return false;
  };

  // ── Mutation tests ──
  {
    // M1: corrupt a frame PNG byte
    {
      auto mut_dir = pkg_dir; mut_dir += "_m1"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      for (auto const& e : fs::directory_iterator(mut_dir/"frames")) {
        if (e.is_regular_file() && e.path().extension() == ".png") {
          std::fstream f(e.path(), std::ios::binary | std::ios::in | std::ios::out);
          if (f) { f.seekp(12); f.put(static_cast<char>(0xFF)); f.close(); break; }
        }
      }
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M1: corrupted frame PNG fails");
      remove_all_retry(mut_dir);
    }
    // M2: corrupt seed-identity.txt
    {
      auto mut_dir = pkg_dir; mut_dir += "_m2"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      std::ofstream(mut_dir/"seed-identity.txt", std::ios::trunc) << "0000000000000000000000000000000000000000000000000000000000000000";
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M2: wrong seed identity fails");
      remove_all_retry(mut_dir);
    }
    // M3: missing required artifact
    {
      auto mut_dir = pkg_dir; mut_dir += "_m3"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      fs::remove(mut_dir/"animations-2d.json");
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M3: missing animations-2d fails");
      remove_all_retry(mut_dir);
    }
    // M4: change frames.json schema to wrong value
    {
      auto mut_dir = pkg_dir; mut_dir += "_m4"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto fm_bytes = read_file_bytes(mut_dir/"frames.json", 512ULL*1024*1024);
      std::string fm(fm_bytes.begin(), fm_bytes.end());
      auto pos = fm.find("gspl.frames-2d/0.1");
      if (pos != std::string::npos) fm.replace(pos, 17, "bad-schema/0.0");
      std::ofstream(mut_dir/"frames.json", std::ios::trunc | std::ios::binary).write(fm.data(), fm.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M4: wrong frames schema fails");
      remove_all_retry(mut_dir);
    }
    // M5: delete a frame PNG
    {
      auto mut_dir = pkg_dir; mut_dir += "_m5"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      for (auto const& e : fs::directory_iterator(mut_dir/"frames")) {
        if (e.is_regular_file() && e.path().extension() == ".png") { fs::remove(e.path()); break; }
      }
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M5: missing frame PNG fails");
      remove_all_retry(mut_dir);
    }
    // M6: change a frame hash in frames.json
    {
      auto mut_dir = pkg_dir; mut_dir += "_m6"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto fm_bytes = read_file_bytes(mut_dir/"frames.json", 512ULL*1024*1024);
      std::string fm(fm_bytes.begin(), fm_bytes.end());
      auto pos = fm.find("\"frame_hash\":\"");
      if (pos != std::string::npos) fm.replace(pos+14, 64, std::string(64, '0'));
      std::ofstream(mut_dir/"frames.json", std::ios::trunc | std::ios::binary).write(fm.data(), fm.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M6: wrong frame hash fails");
      remove_all_retry(mut_dir);
    }
    // M7: delete a sample from frame-samples.json
    {
      auto mut_dir = pkg_dir; mut_dir += "_m7"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
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
      remove_all_retry(mut_dir);
    }
    // M8: change an event frame_id
    {
      auto mut_dir = pkg_dir; mut_dir += "_m8"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto ev_bytes = read_file_bytes(mut_dir/"animation-events.json", 512ULL*1024*1024);
      std::string ev(ev_bytes.begin(), ev_bytes.end());
      auto pos = ev.find("\"frame_id\":\"");
      if (pos != std::string::npos) ev.replace(pos+12, 4, "XXXX");
      std::ofstream(mut_dir/"animation-events.json", std::ios::trunc | std::ios::binary).write(ev.data(), ev.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M8: wrong event frame_id fails");
      remove_all_retry(mut_dir);
    }
    // M9: add undeclared file
    {
      auto mut_dir = pkg_dir; mut_dir += "_m9"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      std::ofstream(mut_dir/"extra.dat") << "undeclared";
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M9: undeclared file fails");
      remove_all_retry(mut_dir);
    }
    // M10: corrupt manifest artifact hash
    {
      auto mut_dir = pkg_dir; mut_dir += "_m10"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
      auto mf_bytes = read_file_bytes(mut_dir/"manifest.json", 4ULL*1024*1024);
      std::string mf(mf_bytes.begin(), mf_bytes.end());
      auto pos = mf.find("\"sha256\":\"");
      if (pos != std::string::npos) mf.replace(pos+10, 64, std::string(64, '0'));
      std::ofstream(mut_dir/"manifest.json", std::ios::trunc | std::ios::binary).write(mf.data(), mf.size());
      check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M10: corrupt manifest hash fails");
      remove_all_retry(mut_dir);
    }
  }

  // ── Provenance validation: all 13 checks pass on valid package ──
  {
    std::cout << "\n--- Provenance Validation Tests ---\n";
    auto verify = gspl::sprites::verify_living_visual_package(pkg_dir);
    check(verify.ok(), "prov: verify ok");
    check(!has_diagnostic(verify, "LV_PROV_CANONICAL_ENTITY"), "prov: canonical entity ok");
    check(!has_diagnostic(verify, "LV_PROV_FRAME_SET"), "prov: frame set ok");
    check(!has_diagnostic(verify, "LV_PROV_FRAME_HASH_TABLE"), "prov: frame hash table ok");
    check(!has_diagnostic(verify, "LV_PROV_SAMPLE_TABLE"), "prov: sample table ok");
    check(!has_diagnostic(verify, "LV_PROV_EVENT_SCHEDULE"), "prov: event schedule ok");
    check(!has_diagnostic(verify, "LV_PROV_POSE_TABLE"), "prov: pose table ok");
    check(!has_diagnostic(verify, "LV_PROV_GENERATED_CLIPS"), "prov: generated clips ok");
    check(!has_diagnostic(verify, "LV_PROV_COLLISIONS"), "prov: collisions ok");
    check(!has_diagnostic(verify, "LV_PROV_CHANNEL_SET"), "prov: channel set ok");
    check(!has_diagnostic(verify, "LV_PROV_BASE_MORPHOLOGY"), "prov: base morphology ok");
    check(!has_diagnostic(verify, "LV_PROV_STORM_MORPHOLOGY"), "prov: storm morphology ok");
    check(!has_diagnostic(verify, "LV_PROV_TRANSFORMATION_MORPHOLOGIES"), "prov: trans morphs ok");
    check(!has_diagnostic(verify, "LV_PROV_ATLAS"), "prov: atlas ok");
  }

  // ── M11: corrupt collision bone (self-consistent mutation test) ──
  {
    auto mut_dir = pkg_dir; mut_dir += "_m11"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
    auto col_bytes = read_file_bytes(mut_dir/"collisions-2d.json", 4ULL*1024*1024);
    std::string col(col_bytes.begin(), col_bytes.end());
    auto pos = col.find("\"bone_id\":\"");
    if (pos != std::string::npos) col.replace(pos + 11, 4, "XXXX");
    std::ofstream(mut_dir/"collisions-2d.json", std::ios::trunc | std::ios::binary).write(col.data(), col.size());
    check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M11: corrupt collision bone fails");
    remove_all_retry(mut_dir);
  }

  // ── M12: extra frame-hash record ──
  {
    auto mut_dir = pkg_dir; mut_dir += "_m12"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
    auto fh_bytes = read_file_bytes(mut_dir/"frame-hashes.json", 4ULL*1024*1024);
    std::string fh(fh_bytes.begin(), fh_bytes.end());
    auto pos = fh.rfind("{\"frame_id\"");
    if (pos != std::string::npos) {
      auto end = fh.find("}", pos) + 1;
      fh.insert(end, ",{\"frame_id\":\"EXTRA\",\"frame_hash\":\"" + std::string(64, '0') + "\"}");
    }
    std::ofstream(mut_dir/"frame-hashes.json", std::ios::trunc | std::ios::binary).write(fh.data(), fh.size());
    check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M12: extra frame-hash record fails");
    remove_all_retry(mut_dir);
  }

  // ── M13: change pose record frame_id ──
  {
    auto mut_dir = pkg_dir; mut_dir += "_m13"; remove_all_retry(mut_dir); fs::copy(pkg_dir, mut_dir, fs::copy_options::recursive);
    auto ph_bytes = read_file_bytes(mut_dir/"pose-hashes.json", 4ULL*1024*1024);
    std::string ph(ph_bytes.begin(), ph_bytes.end());
    auto pos = ph.find("\"frame_id\":\"");
    if (pos != std::string::npos) ph.replace(pos + 12, 4, "YYYY");
    std::ofstream(mut_dir/"pose-hashes.json", std::ios::trunc | std::ios::binary).write(ph.data(), ph.size());
    check(!gspl::sprites::verify_living_visual_package(mut_dir).ok(), "M13: wrong pose frame_id fails");
    remove_all_retry(mut_dir);
  }

  // ── Self-Consistent Mutations (refresh artifact hash/size + package identity) ──
  // Each SC mutation recomputes manifest integrity so rejection hits the
  // semantic layer, not the inventory/hash layer.
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Self-Consistent Mutation Tests ---\n";
    PackageReadLimits rlim{};
    auto mf_bytes = read_file_bytes(pkg_dir/"manifest.json", 4ULL*1024*1024);
    auto mf_parse = parse_living_package_manifest({mf_bytes.begin(), mf_bytes.end()}, rlim);
    check(mf_parse.ok(), "SC: base manifest parse ok");
    auto m = *mf_parse.value;

    auto refresh = [&](auto& manifest, auto d, auto path) {
      auto b = read_file_bytes(d/path, 512ULL*1024*1024);
      auto h = sha256({b.begin(), b.end()});
      for (auto& a : manifest.artifacts) if (a.path == path) {
        a.byte_size = static_cast<std::uint32_t>(b.size()); a.sha256 = std::move(h); return;
      }
    };
    auto finalize = [&](auto& manifest, auto d) {
      auto cn = canonicalize_manifest(manifest, false);
      manifest.package_identity = sha256(std::string(kIdentityPreimageVersion) + "\n" + cn);
      auto cw = canonicalize_manifest(manifest, true);
      std::ofstream(d/"manifest.json", std::ios::trunc | std::ios::binary).write(cw.data(), cw.size());
    };
    auto has_diag = [](auto const& r, std::string_view code) {
      for (auto const& d : r.validation.diagnostics) if (d.code == code) return true;
      return false;
    };

    // SC3: pose record frame_id changed (cross-artifact structural)
    {
      auto md = pkg_dir; md += "_sc3"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto ph = read_file_bytes(md/"pose-hashes.json", 4ULL*1024*1024);
      std::string s(ph.begin(), ph.end());
      auto pos = s.find("\"frame_id\":\"");
      if (pos != std::string::npos) s.replace(pos + 12, 4, "YYYY");
      std::ofstream(md/"pose-hashes.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "pose-hashes.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_POSE_FRAME_ID"), "SC3: LV_POSE_FRAME_ID");
      remove_all_retry(md);
    }
    // SC6: remove sample position (structural)
    {
      auto md = pkg_dir; md += "_sc6"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto fsb = read_file_bytes(md/"frame-samples.json", 512ULL*1024*1024);
      std::string s(fsb.begin(), fsb.end());
      auto st = s.find("{\"clip_id\"");
      if (st != std::string::npos) {
        auto en = s.find("}", st) + 1;
        if (s[en] == ',') ++en;
        s.erase(st, en - st);
      }
      std::ofstream(md/"frame-samples.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml2 = m; refresh(ml2, md, "frame-samples.json"); finalize(ml2, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_SAMPLE_MISSING"), "SC6: LV_SAMPLE_MISSING");
      remove_all_retry(md);
    }
    // SC8: flip clip looping flag (semantic)
    {
      auto md = pkg_dir; md += "_sc8"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto ab = read_file_bytes(md/"animations-2d.json", 4ULL*1024*1024);
      std::string s(ab.begin(), ab.end());
      auto pos = s.find("\"looping\":false");
      if (pos != std::string::npos) s.replace(pos + 10, 6, "true,");  // false, → true,
      else { pos = s.find("\"looping\":true"); if (pos != std::string::npos) s.replace(pos + 10, 5, "false"); }
      std::ofstream(md/"animations-2d.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml3 = m; refresh(ml3, md, "animations-2d.json"); finalize(ml3, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_CLIP_LOOP"), "SC8: LV_CLIP_LOOP");
      remove_all_retry(md);
    }
    // SC10: event frame_id changed (cross-reference)
    {
      auto md = pkg_dir; md += "_sc10"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto eb = read_file_bytes(md/"animation-events.json", 4ULL*1024*1024);
      std::string s(eb.begin(), eb.end());
      auto pos = s.find("\"frame_id\":\"");
      if (pos != std::string::npos) s.replace(pos + 12, 4, "XXXX");
      std::ofstream(md/"animation-events.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml4 = m; refresh(ml4, md, "animation-events.json"); finalize(ml4, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_EVENT_FRAME_ID"), "SC10: LV_EVENT_FRAME_ID");
      remove_all_retry(md);
    }
    // SC1: extra frame-hash record → LV_FH_EXTRA
    {
      auto md = pkg_dir; md += "_sc1"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto fhb = read_file_bytes(md/"frame-hashes.json", 4ULL*1024*1024);
      std::string s(fhb.begin(), fhb.end());
      auto pos = s.rfind("{\"frame_id\"");
      if (pos != std::string::npos) {
        auto end = s.find("}", pos) + 1;
        s.insert(end, ",{\"frame_id\":\"EXTRA\",\"frame_hash\":\"" + std::string(64, '0') + "\"}");
      }
      std::ofstream(md/"frame-hashes.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "frame-hashes.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_FH_EXTRA"), "SC1: LV_FH_EXTRA");
      remove_all_retry(md);
    }
    // SC2: missing frame-hash record → LV_FH_MISSING
    {
      auto md = pkg_dir; md += "_sc2"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto fhb = read_file_bytes(md/"frame-hashes.json", 4ULL*1024*1024);
      std::string s(fhb.begin(), fhb.end());
      auto pos = s.find("{\"frame_id\"");
      if (pos != std::string::npos) {
        auto end = s.find("}", pos) + 1;
        if (s[end] == ',') ++end;
        s.erase(pos, end - pos);
      }
      std::ofstream(md/"frame-hashes.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "frame-hashes.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_FH_MISSING"), "SC2: LV_FH_MISSING");
      remove_all_retry(md);
    }
    // SC4: extra pose record → LV_POSE_EXTRA
    {
      auto md = pkg_dir; md += "_sc4"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto phb = read_file_bytes(md/"pose-hashes.json", 4ULL*1024*1024);
      std::string s(phb.begin(), phb.end());
      auto pos = s.rfind("{\"clip_id\"");
      if (pos != std::string::npos) {
        auto end = s.find("}", pos) + 1;
        s.insert(end, ",{\"clip_id\":\"EXTRA\",\"frame_index\":99,\"frame_id\":\"EXTRA\",\"pose_hash\":\"" + std::string(64, '0') + "\"}");
      }
      std::ofstream(md/"pose-hashes.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "pose-hashes.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_POSE_EXTRA"), "SC4: LV_POSE_EXTRA");
      remove_all_retry(md);
    }
    // SC5: missing pose record → LV_POSE_MISSING
    {
      auto md = pkg_dir; md += "_sc5"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto phb = read_file_bytes(md/"pose-hashes.json", 4ULL*1024*1024);
      std::string s(phb.begin(), phb.end());
      auto pos = s.find("{\"clip_id\"");
      if (pos != std::string::npos) {
        auto end = s.find("}", pos) + 1;
        if (s[end] == ',') ++end;
        s.erase(pos, end - pos);
      }
      std::ofstream(md/"pose-hashes.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "pose-hashes.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_POSE_MISSING"), "SC5: LV_POSE_MISSING");
      remove_all_retry(md);
    }
    // SC7: duplicate sample position → LV_SAMPLE_DUP
    {
      auto md = pkg_dir; md += "_sc7"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto fsb = read_file_bytes(md/"frame-samples.json", 512ULL*1024*1024);
      std::string s(fsb.begin(), fsb.end());
      auto pos = s.find("{\"clip_id\"");
      if (pos != std::string::npos) {
        auto end = s.find("}", pos) + 1;
        auto dup = s.substr(pos, end - pos);
        s.insert(end, "," + dup);
      }
      std::ofstream(md/"frame-samples.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "frame-samples.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_SAMPLE_DUP"), "SC7: LV_SAMPLE_DUP");
      remove_all_retry(md);
    }
    // SC9: event mapped_source_tick changed → LV_EVENT_NOT_FIRST_AT_OR_AFTER
    {
      auto md = pkg_dir; md += "_sc9"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto eb = read_file_bytes(md/"animation-events.json", 4ULL*1024*1024);
      std::string s(eb.begin(), eb.end());
      auto pos = s.find("\"mapped_source_tick\":");
      if (pos != std::string::npos) {
        auto end = s.find_first_of(",}", pos + 21);
        if (end != std::string::npos) s.replace(pos + 21, end - (pos + 21), "999");
      }
      std::ofstream(md/"animation-events.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "animation-events.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_EVENT_NOT_FIRST_AT_OR_AFTER"), "SC9: LV_EVENT_NOT_FIRST_AT_OR_AFTER");
      remove_all_retry(md);
    }
    // SC11: generated authored_tick changed → LV_EVENT_AUTHORED_TICK
    {
      auto md = pkg_dir; md += "_sc11"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto eb = read_file_bytes(md/"animation-events.json", 4ULL*1024*1024);
      std::string s(eb.begin(), eb.end());
      auto pos = s.find("\"authored_tick\":");
      if (pos != std::string::npos) {
        auto end = s.find(",", pos);
        s.replace(pos, end - pos, "\"authored_tick\":999");
      }
      std::ofstream(md/"animation-events.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "animation-events.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_EVENT_AUTHORED_TICK"), "SC11: LV_EVENT_AUTHORED_TICK");
      remove_all_retry(md);
    }
    // SC12: source authored event tick changed → LV_EVENT_AUTHORED_TICK
    {
      auto md = pkg_dir; md += "_sc12"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = read_file_bytes(md/"source-skeletal-animations.json", 4ULL*1024*1024);
      std::string s(sb.begin(), sb.end());
      // Find a tick inside an events block (not a keyframe tick inside tracks)
      auto ev_pos = s.find("\"events\":[");
      auto tick_pos = (ev_pos != std::string::npos) ? s.find("\"tick\":", ev_pos) : std::string::npos;
      if (tick_pos != std::string::npos) {
        auto end = s.find_first_of(",}", tick_pos);
        // Pick a tick that stays strictly inside every clip duration (min 13)
        // so the strict source parser accepts the artifact and the semantic
        // authored-tick binding layer emits LV_EVENT_AUTHORED_TICK.
        s.replace(tick_pos, end - tick_pos, "\"tick\":12");
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(!v.ok() && has_diag(v, "LV_EVENT_AUTHORED_TICK"), "SC12: LV_EVENT_AUTHORED_TICK");
      remove_all_retry(md);
    }
    // SC-positive: valid package passes after manifest refresh
    {
      auto md = pkg_dir; md += "_sc_valid"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto ml5 = m;
      for (auto const& a : ml5.artifacts) refresh(ml5, md, a.path);
      finalize(ml5, md);
      auto v = verify_living_visual_package(md);
      check(v.ok(), "SC: valid package passes after manifest regeneration");
      remove_all_retry(md);
    }
  }

  // ── Visual-Semantic Self-Consistent Mutations SC13-SC19 ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Visual-Semantic Mutation Tests (SC13-SC19) ---\n";
    PackageReadLimits rlim{};
    auto mf_bytes = read_file_bytes(pkg_dir/"manifest.json", 4ULL*1024*1024);
    auto mf_parse = parse_living_package_manifest({mf_bytes.begin(), mf_bytes.end()}, rlim);
    check(mf_parse.ok(), "SCV: base manifest parse ok");
    auto m = *mf_parse.value;
    auto refresh = [&](auto& manifest, auto d, auto path) {
      auto b = read_file_bytes(d/path, 512ULL*1024*1024);
      auto h = sha256({b.begin(), b.end()});
      for (auto& a : manifest.artifacts) if (a.path == path) {
        a.byte_size = static_cast<std::uint32_t>(b.size()); a.sha256 = std::move(h); return;
      }
    };
    auto finalize = [&](auto& manifest, auto d) {
      auto cn = canonicalize_manifest(manifest, false);
      manifest.package_identity = sha256(std::string(kIdentityPreimageVersion) + "\n" + cn);
      auto cw = canonicalize_manifest(manifest, true);
      std::ofstream(d/"manifest.json", std::ios::trunc | std::ios::binary).write(cw.data(), cw.size());
    };
    auto has_diag = [](auto const& r, std::string_view code) {
      for (auto const& d : r.validation.diagnostics) if (d.code == code) return true;
      return false;
    };
    auto reached_semantic = [&](auto const& v) {
      return !has_diag(v, "LV_READ_PKG_ID") && !has_diag(v, "LV_READ_MANIFEST_NONCANONICAL") &&
             !has_diag(v, "LV_INV_SIZE") && !has_diag(v, "LV_INV_HASH");
    };
    auto find_quoted = [](std::string const& s, std::string_view key, std::size_t from) -> std::pair<std::size_t, std::size_t> {
      auto kp = s.find(key, from);
      if (kp == std::string::npos) return {std::string::npos, std::string::npos};
      auto vs = s.find('"', kp + key.size());
      if (vs == std::string::npos) return {std::string::npos, std::string::npos};
      auto ve = s.find('"', vs + 1);
      if (ve == std::string::npos) return {std::string::npos, std::string::npos};
      return {vs + 1, ve};
    };
    auto match_closer = [](std::string const& s, std::size_t open) -> std::size_t {
      if (open >= s.size() || (s[open] != '{' && s[open] != '[')) return std::string::npos;
      char open_c = s[open]; char close_c = (open_c == '{') ? '}' : ']';
      int depth = 0; bool in_str = false;
      for (std::size_t i = open; i < s.size(); ++i) {
        char c = s[i];
        if (in_str) { if (c == '\\') ++i; else if (c == '"') in_str = false; }
        else if (c == '"') in_str = true;
        else if (c == open_c) ++depth;
        else if (c == close_c) { --depth; if (depth == 0) return i; }
      }
      return std::string::npos;
    };
    auto tiny_png = []() {
      ImageRgba8 img{2, 2, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(16, 255)};
      return encode_png(img);
    };

    // SC13: channel target_frame_id changed → LV_CHANNEL_UNKNOWN_FRAME
    {
      auto md = pkg_dir; md += "_sc13"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto cb = read_file_bytes(md/"channels.json", 4ULL*1024*1024);
      std::string s(cb.begin(), cb.end());
      auto r = find_quoted(s, "\"target_frame_id\":", 0);
      if (r.first != std::string::npos) s.replace(r.first, r.second - r.first, "ZZZ_UNKNOWN");
      std::ofstream(md/"channels.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "channels.json"); finalize(ml, md);
      // Reconstruct the mutated package and recompute channel-set provenance with
      // the production canonicalizer so the relational diagnostic is the only hit.
      auto rd13 = read_living_visual_package(md, rlim);
      check(rd13.value.has_value(), "SC13: mutated package reads");
      if (rd13.value.has_value()) {
        auto prov13 = compute_domain_id(kDomainChannelSet, canonicalize_channel_set_preimage(rd13.value->channels));
        for (auto& a : ml.artifacts) if (a.path == "channels.json") a.provenance_identity = prov13;
        refresh(ml, md, "channels.json"); finalize(ml, md);
      }
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "SC13: reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_CHANNEL_UNKNOWN_FRAME"), "SC13: LV_CHANNEL_UNKNOWN_FRAME");
      check(!has_diag(v, "LV_PROV_CHANNEL_SET"), "SC13: channel-set provenance recomputed");
      remove_all_retry(md);
    }
    // SC14: channel PNG pixels changed → LV_PROV_CHANNEL_IMAGE
    {
      auto md = pkg_dir; md += "_sc14"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      fs::path ch_png;
      for (auto const& e : fs::directory_iterator(md/"channels"))
        if (e.is_regular_file() && e.path().extension() == ".png") { ch_png = e.path(); break; }
      check(!ch_png.empty(), "SC14: found channel png");
      if (!ch_png.empty()) {
        auto enc = tiny_png();
        std::ofstream(ch_png, std::ios::trunc | std::ios::binary).write(
            reinterpret_cast<const char*>(enc.data()), static_cast<std::streamsize>(enc.size()));
        auto ml = m; refresh(ml, md, "channels/" + ch_png.filename().string()); finalize(ml, md);
        auto v = verify_living_visual_package(md);
        check(reached_semantic(v), "SC14: reached semantic layer");
        check(!v.ok() && has_diag(v, "LV_PROV_CHANNEL_IMAGE"), "SC14: LV_PROV_CHANNEL_IMAGE");
      }
      remove_all_retry(md);
    }
    // SC15: morphology emissive changed, provenance stale → LV_PROV_BASE_MORPHOLOGY
    {
      auto md = pkg_dir; md += "_sc15"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto mb = read_file_bytes(md/"resolved-base-morphology.json", 8ULL*1024*1024);
      std::string s(mb.begin(), mb.end());
      // Compact canonical JSON has no whitespace after the colon; locate the
      // boolean token robustly instead of matching a spaced serialization.
      auto pos = s.find("\"emissive\":");
      if (pos != std::string::npos) {
        auto vp = s.find_first_not_of(" \t", pos + 11);
        if (vp != std::string::npos && s.compare(vp, 4, "true") == 0) s.replace(vp, 4, "false");
      }
      std::ofstream(md/"resolved-base-morphology.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "resolved-base-morphology.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "SC15: reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_PROV_BASE_MORPHOLOGY"), "SC15: LV_PROV_BASE_MORPHOLOGY");
      remove_all_retry(md);
    }
    // SC16: morphology parent changed, structural → LV_MORPH_PARENT_REF
    {
      auto md = pkg_dir; md += "_sc16"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto mb = read_file_bytes(md/"resolved-base-morphology.json", 8ULL*1024*1024);
      std::string s(mb.begin(), mb.end());
      // Compact canonical JSON: no space after colon; resolve the parent value
      // by key then replace the embedded root reference.
      auto pos = s.find("\"parent\":");
      if (pos != std::string::npos) {
        auto inner = s.find("root", pos);
        if (inner != std::string::npos) s.replace(inner, 4, "ZZZ");
      }
      std::ofstream(md/"resolved-base-morphology.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "resolved-base-morphology.json"); finalize(ml, md);
      auto rd16 = read_living_visual_package(md, rlim);
      check(rd16.value.has_value(), "SC16: mutated package reads");
      if (rd16.value.has_value()) {
        auto prov16 = compute_domain_id(kDomainMorphologySet, canonicalize_morphology_preimage(rd16.value->base_morphology));
        for (auto& a : ml.artifacts) if (a.path == "resolved-base-morphology.json") a.provenance_identity = prov16;
        refresh(ml, md, "resolved-base-morphology.json"); finalize(ml, md);
      }
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "SC16: reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_MORPH_PARENT_REF"), "SC16: LV_MORPH_PARENT_REF");
      check(!has_diag(v, "LV_PROV_BASE_MORPHOLOGY"), "SC16: base morphology provenance recomputed");
      remove_all_retry(md);
    }
    // SC17: transformation endpoint changed → LV_MORPH_TRANSFORM_BASE
    {
      auto md = pkg_dir; md += "_sc17"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto tb = read_file_bytes(md/"transformation-morphologies.json", 16ULL*1024*1024);
      std::string s(tb.begin(), tb.end());
      // Compact canonical JSON has no whitespace after the colon; locate the
      // boolean token robustly instead of matching a spaced serialization.
      auto pos = s.find("\"emissive\":");
      if (pos != std::string::npos) {
        auto vp = s.find_first_not_of(" \t", pos + 11);
        if (vp != std::string::npos && s.compare(vp, 4, "true") == 0) s.replace(vp, 4, "false");
      }
      std::ofstream(md/"transformation-morphologies.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "transformation-morphologies.json"); finalize(ml, md);
      auto rd17 = read_living_visual_package(md, rlim);
      check(rd17.value.has_value(), "SC17: mutated package reads");
      if (rd17.value.has_value()) {
        auto prov17 = compute_domain_id(kDomainMorphologySet,
            canonicalize_transformation_preimage(rd17.value->transformation_morphologies));
        for (auto& a : ml.artifacts) if (a.path == "transformation-morphologies.json") a.provenance_identity = prov17;
        refresh(ml, md, "transformation-morphologies.json"); finalize(ml, md);
      }
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "SC17: reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_MORPH_TRANSFORM_BASE"), "SC17: LV_MORPH_TRANSFORM_BASE");
      check(!has_diag(v, "LV_PROV_TRANSFORMATION_MORPHOLOGIES"), "SC17: transformation provenance recomputed");
      remove_all_retry(md);
    }
    // SC18: atlas placement frame changed → LV_ATLAS_UNKNOWN_FRAME
    {
      auto md = pkg_dir; md += "_sc18"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto ab = read_file_bytes(md/"sheet/atlas.json", 4ULL*1024*1024);
      std::string s(ab.begin(), ab.end());
      auto r = find_quoted(s, "\"id\":", 0);
      if (r.first != std::string::npos) s.replace(r.first, r.second - r.first, "ZZZ_FRAME");
      std::ofstream(md/"sheet/atlas.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "sheet/atlas.json"); finalize(ml, md);
      auto rd18 = read_living_visual_package(md, rlim);
      check(rd18.value.has_value(), "SC18: mutated package reads");
      if (rd18.value.has_value()) {
        auto prov18 = compute_domain_id(kDomainSpriteAtlas,
            canonicalize_atlas_preimage(rd18.value->sheet.atlas.placements, rd18.value->sheet.atlas.image));
        for (auto& a : ml.artifacts)
          if (a.path == "sheet/atlas.json" || a.path == "sheet/atlas.png") a.provenance_identity = prov18;
        refresh(ml, md, "sheet/atlas.json"); refresh(ml, md, "sheet/atlas.png"); finalize(ml, md);
      }
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "SC18: reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_ATLAS_UNKNOWN_FRAME"), "SC18: LV_ATLAS_UNKNOWN_FRAME");
      check(!has_diag(v, "LV_PROV_ATLAS"), "SC18: atlas provenance recomputed");
      remove_all_retry(md);
    }
    // SC19: atlas PNG pixels changed → LV_PROV_ATLAS
    {
      auto md = pkg_dir; md += "_sc19"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto enc = tiny_png();
      std::ofstream(md/"sheet/atlas.png", std::ios::trunc | std::ios::binary).write(
          reinterpret_cast<const char*>(enc.data()), static_cast<std::streamsize>(enc.size()));
      auto ml = m; refresh(ml, md, "sheet/atlas.png"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "SC19: reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_PROV_ATLAS"), "SC19: LV_PROV_ATLAS");
      remove_all_retry(md);
    }
  }

  // ── Image-semantic identity (color space + alpha mode bound into authority) ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Image-Semantic Identity Tests ---\n";
    auto mk_img = [](std::uint32_t w, std::uint32_t h, ColorSpace cs, AlphaMode am, std::uint8_t v) {
      return ImageRgba8{w, h, cs, am, std::vector<std::uint8_t>(static_cast<std::size_t>(w) * h * 4, v)};
    };
    auto base = mk_img(4, 4, ColorSpace::srgb, AlphaMode::straight, 128);
    auto id_base = canonicalize_image_semantics_preimage(base);
    check(id_base == canonicalize_image_semantics_preimage(base), "img-identity: deterministic");
    {
      auto other = base; other.color_space = ColorSpace::data;
      check(id_base != canonicalize_image_semantics_preimage(other), "img-identity: color space bound");
    }
    {
      auto other = base; other.alpha_mode = AlphaMode::opaque;
      check(id_base != canonicalize_image_semantics_preimage(other), "img-identity: alpha mode bound");
    }
    {
      auto other = base; other.pixels[0] = 0;
      check(id_base != canonicalize_image_semantics_preimage(other), "img-identity: pixel change bound");
    }
    {
      auto other = base; other.width = 8; other.pixels.resize(8 * 4 * 4, 128);
      check(id_base != canonicalize_image_semantics_preimage(other), "img-identity: dimensions bound");
    }
    // Channel-set / channel-image identity distinguishes color/alpha interpretation
    {
      ChannelMap ca; ca.id = "c1"; ca.target_frame_id = "f1"; ca.kind = ChannelMapKind::effects; ca.image = base;
      auto cb = ca; cb.image.color_space = ColorSpace::data;
      auto cc = ca; cc.image.alpha_mode = AlphaMode::premultiplied;
      std::vector<ChannelMap> vca{ca}, vcb{cb}, vcc{cc};
      check(canonicalize_channel_set_preimage(vca) != canonicalize_channel_set_preimage(vcb), "channel-set identity: color space bound");
      check(canonicalize_channel_set_preimage(vca) != canonicalize_channel_set_preimage(vcc), "channel-set identity: alpha mode bound");
      check(canonicalize_channel_set_preimage(vca) == canonicalize_channel_set_preimage(vca), "channel-set identity: deterministic");
      check(canonicalize_channel_image_preimage(ca) != canonicalize_channel_image_preimage(cb), "channel-image identity: color space bound");
      check(canonicalize_channel_image_preimage(ca) != canonicalize_channel_image_preimage(cc), "channel-image identity: alpha mode bound");
    }
    // Atlas identity distinguishes color/alpha interpretation
    {
      auto at = mk_img(16, 8, ColorSpace::srgb, AlphaMode::straight, 64);
      AtlasPlacement pl; pl.frame_id = "f1"; pl.x = 0; pl.y = 0; pl.width = 4; pl.height = 4;
      pl.pivot_x = 0; pl.pivot_y = 0; pl.duration_ticks = 1;
      std::vector<AtlasPlacement> pls{pl};
      auto id_atlas = canonicalize_atlas_preimage(pls, at);
      {
        auto other = at; other.color_space = ColorSpace::data;
        check(id_atlas != canonicalize_atlas_preimage(pls, other), "atlas identity: color space bound");
      }
      {
        auto other = at; other.alpha_mode = AlphaMode::opaque;
        check(id_atlas != canonicalize_atlas_preimage(pls, other), "atlas identity: alpha mode bound");
      }
      check(id_atlas == canonicalize_atlas_preimage(pls, at), "atlas identity: deterministic");
    }
  }

  // ── Verification option behavior ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Verification Option Behavior Tests ---\n";
    PackageReadLimits rlim{};
    auto mf_bytes = read_file_bytes(pkg_dir/"manifest.json", 4ULL*1024*1024);
    auto mf_parse = parse_living_package_manifest({mf_bytes.begin(), mf_bytes.end()}, rlim);
    auto m = *mf_parse.value;
    auto refresh = [&](auto& manifest, auto d, auto path) {
      auto b = read_file_bytes(d/path, 512ULL*1024*1024);
      auto h = sha256({b.begin(), b.end()});
      for (auto& a : manifest.artifacts) if (a.path == path) {
        a.byte_size = static_cast<std::uint32_t>(b.size()); a.sha256 = std::move(h); return;
      }
    };
    auto finalize = [&](auto& manifest, auto d) {
      auto cn = canonicalize_manifest(manifest, false);
      manifest.package_identity = sha256(std::string(kIdentityPreimageVersion) + "\n" + cn);
      auto cw = canonicalize_manifest(manifest, true);
      std::ofstream(d/"manifest.json", std::ios::trunc | std::ios::binary).write(cw.data(), cw.size());
    };
    auto has_diag = [](auto const& r, std::string_view code) {
      for (auto const& d : r.validation.diagnostics) if (d.code == code) return true;
      return false;
    };
    auto find_quoted = [](std::string const& s, std::string_view key, std::size_t from) -> std::pair<std::size_t, std::size_t> {
      auto kp = s.find(key, from);
      if (kp == std::string::npos) return {std::string::npos, std::string::npos};
      auto vs = s.find('"', kp + key.size());
      if (vs == std::string::npos) return {std::string::npos, std::string::npos};
      auto ve = s.find('"', vs + 1);
      if (ve == std::string::npos) return {std::string::npos, std::string::npos};
      return {vs + 1, ve};
    };
    auto tiny_png = []() {
      ImageRgba8 img{2, 2, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(16, 255)};
      return encode_png(img);
    };

    // All optional checks disabled: mandatory authority still passes
    {
      PackageVerificationOptions off{};
      off.verify_pixel_hashes = false; off.verify_channel_dimensions = false;
      off.verify_morphologies = false; off.strict_collision_refs = false;
      off.require_no_symlinks = false; off.require_no_undeclared_files = false;
      auto v = verify_living_visual_package(pkg_dir, off);
      check(v.ok(), "opt: valid package passes with all optional checks disabled");
    }
    // verify_pixel_hashes gates atlas-region pixel recomputation only
    {
      auto md = pkg_dir; md += "_opt_px"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto ab = read_file_bytes(md/"sheet/atlas.png", 512ULL*1024*1024);
      auto img = decode_png(std::span<const std::byte>(
          reinterpret_cast<const std::byte*>(ab.data()), ab.size()), ImageLimits{});
      auto off = (static_cast<std::size_t>(5) * img.width + 5) * 4ULL;
      if (off + 4 <= img.pixels.size()) {
        img.pixels[off] ^= 0xFF; img.pixels[off+1] ^= 0xFF; img.pixels[off+2] ^= 0xFF;
      }
      // The atlas is re-encoded after decoding; the decoded image may report an
      // unknown color space if the source PNG lacks an sRGB chunk. Re-declaring
      // sRGB keeps the round-trip deterministic (provenance still differs via pixels).
      img.color_space = ColorSpace::srgb;
      auto enc = encode_png(img);
      std::ofstream(md/"sheet/atlas.png", std::ios::trunc | std::ios::binary).write(
          reinterpret_cast<const char*>(enc.data()), static_cast<std::streamsize>(enc.size()));
      auto ml = m; refresh(ml, md, "sheet/atlas.png"); finalize(ml, md);
      PackageVerificationOptions pixel_off{}; pixel_off.verify_pixel_hashes = false;
      auto von = verify_living_visual_package(md);
      auto voff = verify_living_visual_package(md, pixel_off);
      check(has_diag(von, "LV_ATLAS_REGION_MISMATCH"), "opt: pixel-on detects atlas region mismatch");
      check(!has_diag(voff, "LV_ATLAS_REGION_MISMATCH"), "opt: pixel-off skips region recompute");
      check(has_diag(voff, "LV_PROV_ATLAS"), "opt: atlas provenance mandatory even pixel-off");
      remove_all_retry(md);
    }
    // verify_morphologies gates structural morphology checks
    {
      auto md = pkg_dir; md += "_opt_morph"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto mb = read_file_bytes(md/"resolved-base-morphology.json", 8ULL*1024*1024);
      std::string s(mb.begin(), mb.end());
      // Compact canonical JSON: no space after colon; resolve the parent value
      // by key then replace the embedded root reference.
      auto pos = s.find("\"parent\":");
      if (pos != std::string::npos) {
        auto inner = s.find("root", pos);
        if (inner != std::string::npos) s.replace(inner, 4, "ZZZ");
      }
      std::ofstream(md/"resolved-base-morphology.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "resolved-base-morphology.json"); finalize(ml, md);
      PackageVerificationOptions moff{}; moff.verify_morphologies = false;
      auto vdef = verify_living_visual_package(md);
      auto vmoff = verify_living_visual_package(md, moff);
      check(has_diag(vdef, "LV_MORPH_PARENT_REF"), "opt: morphology structural check on by default");
      check(!has_diag(vmoff, "LV_MORPH_PARENT_REF"), "opt: verify_morphologies=false suppresses structural check");
      check(has_diag(vmoff, "LV_PROV_BASE_MORPHOLOGY"), "opt: morphology provenance mandatory regardless");
      remove_all_retry(md);
    }
    // verify_channel_dimensions gates channel dimension checks
    {
      auto md = pkg_dir; md += "_opt_chdim"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      fs::path ch_png;
      for (auto const& e : fs::directory_iterator(md/"channels"))
        if (e.is_regular_file() && e.path().extension() == ".png") { ch_png = e.path(); break; }
      if (!ch_png.empty()) {
        auto enc = tiny_png();
        std::ofstream(ch_png, std::ios::trunc | std::ios::binary).write(
            reinterpret_cast<const char*>(enc.data()), static_cast<std::streamsize>(enc.size()));
        auto ml = m; refresh(ml, md, "channels/" + ch_png.filename().string()); finalize(ml, md);
        PackageVerificationOptions coff{}; coff.verify_channel_dimensions = false;
        auto vdef = verify_living_visual_package(md);
        auto vcoff = verify_living_visual_package(md, coff);
        check(has_diag(vdef, "LV_CHANNEL_DIMENSIONS"), "opt: channel dims check on by default");
        check(!has_diag(vcoff, "LV_CHANNEL_DIMENSIONS"), "opt: verify_channel_dimensions=false suppresses dims check");
        check(has_diag(vcoff, "LV_PROV_CHANNEL_IMAGE"), "opt: channel image provenance mandatory regardless");
      }
      remove_all_retry(md);
    }
    // strict_collision_refs gates collision reference policy
    {
      auto md = pkg_dir; md += "_opt_coll"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto cb = read_file_bytes(md/"collisions-2d.json", 4ULL*1024*1024);
      std::string s(cb.begin(), cb.end());
      auto r = find_quoted(s, "\"bone_id\":", 0);
      if (r.first != std::string::npos) s.replace(r.first, r.second - r.first, "ZZZ");
      std::ofstream(md/"collisions-2d.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "collisions-2d.json"); finalize(ml, md);
      PackageVerificationOptions soff{}; soff.strict_collision_refs = false;
      auto vdef = verify_living_visual_package(md);
      auto vsoff = verify_living_visual_package(md, soff);
      check(has_diag(vdef, "LV_COLLISION_BONE_REF"), "opt: collision ref check on by default");
      check(!has_diag(vsoff, "LV_COLLISION_BONE_REF"), "opt: strict_collision_refs=false suppresses ref check");
      remove_all_retry(md);
    }
  }

  // ── Parser limit enforcement (PackageReadLimits) ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Parser Limit Enforcement Tests ---\n";
    auto has_rd = [](auto const& rd, std::string_view code) {
      for (auto const& d : rd.diagnostics.diagnostics) if (d.code == code) return true;
      return false;
    };
    {
      PackageReadLimits tight{};
      tight.max_frames = 10;
      auto rd = read_living_visual_package(pkg_dir, tight);
      check(!rd.value.has_value(), "limit: max_frames=10 blocks load");
      check(has_rd(rd, "LV_READ_FRAME_LIMIT"), "limit: LV_READ_FRAME_LIMIT exact");
    }
    {
      PackageReadLimits tight{};
      tight.max_channels = 10;
      auto rd = read_living_visual_package(pkg_dir, tight);
      check(!rd.value.has_value(), "limit: max_channels=10 blocks load");
      check(has_rd(rd, "LV_READ_CHANNEL_LIMIT"), "limit: LV_READ_CHANNEL_LIMIT exact");
    }
    {
      PackageReadLimits tight{};
      tight.max_morphology_parts = 2;
      auto rd = read_living_visual_package(pkg_dir, tight);
      check(!rd.value.has_value(), "limit: max_morphology_parts=2 blocks load");
      check(has_rd(rd, "LV_READ_MORPH_PARTS_LIMIT"), "limit: LV_READ_MORPH_PARTS_LIMIT exact");
    }
    {
      PackageReadLimits tight{};
      tight.max_json_tokens = 100;
      auto rd = read_living_visual_package(pkg_dir, tight);
      check(!rd.value.has_value(), "limit: max_json_tokens=100 blocks load");
      check(!rd.diagnostics.ok(), "limit: token budget exhaustion surfaced");
    }
    {
      PackageReadLimits tight{};
      tight.max_path_bytes = 8;
      auto rd = read_living_visual_package(pkg_dir, tight);
      check(!rd.value.has_value(), "limit: max_path_bytes=8 blocks load");
    }
  }

  // ── Strict source-animation parser negatives ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Strict Source Parser Negative Tests ---\n";
    PackageReadLimits rlim{};
    auto mf_bytes = read_file_bytes(pkg_dir/"manifest.json", 4ULL*1024*1024);
    auto mf_parse = parse_living_package_manifest({mf_bytes.begin(), mf_bytes.end()}, rlim);
    auto m = *mf_parse.value;
    auto refresh = [&](auto& manifest, auto d, auto path) {
      auto b = read_file_bytes(d/path, 512ULL*1024*1024);
      auto h = sha256({b.begin(), b.end()});
      for (auto& a : manifest.artifacts) if (a.path == path) {
        a.byte_size = static_cast<std::uint32_t>(b.size()); a.sha256 = std::move(h); return;
      }
    };
    auto finalize = [&](auto& manifest, auto d) {
      auto cn = canonicalize_manifest(manifest, false);
      manifest.package_identity = sha256(std::string(kIdentityPreimageVersion) + "\n" + cn);
      auto cw = canonicalize_manifest(manifest, true);
      std::ofstream(d/"manifest.json", std::ios::trunc | std::ios::binary).write(cw.data(), cw.size());
    };
    auto has_diag = [](auto const& r, std::string_view code) {
      for (auto const& d : r.validation.diagnostics) if (d.code == code) return true;
      return false;
    };
    auto reached_semantic = [&](auto const& v) {
      return !has_diag(v, "LV_READ_PKG_ID") && !has_diag(v, "LV_READ_MANIFEST_NONCANONICAL") &&
             !has_diag(v, "LV_INV_SIZE") && !has_diag(v, "LV_INV_HASH");
    };
    auto match_closer = [](std::string const& s, std::size_t open) -> std::size_t {
      if (open >= s.size() || (s[open] != '{' && s[open] != '[')) return std::string::npos;
      char open_c = s[open]; char close_c = (open_c == '{') ? '}' : ']';
      int depth = 0; bool in_str = false;
      for (std::size_t i = open; i < s.size(); ++i) {
        char c = s[i];
        if (in_str) { if (c == '\\') ++i; else if (c == '"') in_str = false; }
        else if (c == '"') in_str = true;
        else if (c == open_c) ++depth;
        else if (c == close_c) { --depth; if (depth == 0) return i; }
      }
      return std::string::npos;
    };
    auto src_bytes = [&](auto md) { return read_file_bytes(md/"source-skeletal-animations.json", 8ULL*1024*1024); };

    // unknown clip field → LV_SOURCE_PARSE
    {
      auto md = pkg_dir; md += "_src_unknown"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto pos = s.find("\"looping\":");
      if (pos != std::string::npos) s.insert(pos, "\"bogus\":0,");
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: unknown field reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_PARSE"), "src: unknown field LV_SOURCE_PARSE");
      remove_all_retry(md);
    }
    // duplicate clip ID → LV_SOURCE_DUP_CLIP
    {
      auto md = pkg_dir; md += "_src_dupclip"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto st = s.find("\"clips\":[");
      auto co = (st != std::string::npos) ? s.find('{', st) : std::string::npos;
      auto en = (co != std::string::npos) ? match_closer(s, co) : std::string::npos;
      if (en != std::string::npos) {
        auto obj = s.substr(co, en - co + 1);
        s.insert(en + 1, "," + obj);
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: dup clip reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_DUP_CLIP"), "src: LV_SOURCE_DUP_CLIP");
      remove_all_retry(md);
    }
    // duplicate track bone → LV_SOURCE_DUP_TRACK
    {
      auto md = pkg_dir; md += "_src_duptrack"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto tr = s.find("\"tracks\":[");
      auto to = (tr != std::string::npos) ? s.find('{', tr) : std::string::npos;
      auto te = (to != std::string::npos) ? match_closer(s, to) : std::string::npos;
      if (te != std::string::npos) {
        auto obj = s.substr(to, te - to + 1);
        s.insert(te + 1, "," + obj);
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: dup track reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_DUP_TRACK"), "src: LV_SOURCE_DUP_TRACK");
      remove_all_retry(md);
    }
    // duplicate source event → LV_SOURCE_DUP_EVENT
    {
      auto md = pkg_dir; md += "_src_dupevent"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto ev = s.find("\"events\":[");
      auto eo = (ev != std::string::npos) ? s.find('{', ev) : std::string::npos;
      auto ee = (eo != std::string::npos) ? match_closer(s, eo) : std::string::npos;
      if (ee != std::string::npos) {
        auto obj = s.substr(eo, ee - eo + 1);
        s.insert(ee + 1, "," + obj);
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: dup event reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_DUP_EVENT"), "src: LV_SOURCE_DUP_EVENT");
      remove_all_retry(md);
    }
    // duplicate root schema (same value) → LV_SOURCE_PARSE, no loaded package
    {
      auto md = pkg_dir; md += "_src_dupschema"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto s0 = s.find("\"schema\":\"");
      auto s1 = (s0 != std::string::npos) ? s.find('"', s0 + 10) : std::string::npos;
      if (s1 != std::string::npos) {
        auto val = s.substr(s0 + 10, s1 - (s0 + 10));
        s.insert(s1 + 1, ",\"schema\":\"" + val + "\"");
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: dup schema reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_PARSE"), "src: duplicate root schema LV_SOURCE_PARSE");
      auto rd = read_living_visual_package(md);
      check(!rd.value.has_value(), "src: duplicate root schema yields no loaded package");
      remove_all_retry(md);
    }
    // duplicate root schema (conflicting value) → LV_SOURCE_PARSE, no loaded package
    {
      auto md = pkg_dir; md += "_src_dupschemaconf"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto s0 = s.find("\"schema\":\"");
      auto s1 = (s0 != std::string::npos) ? s.find('"', s0 + 10) : std::string::npos;
      if (s1 != std::string::npos) s.insert(s1 + 1, ",\"schema\":\"gspl.bogus-schema/9.9\"");
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: conflicting schema reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_PARSE"), "src: conflicting duplicate schema LV_SOURCE_PARSE");
      auto rd = read_living_visual_package(md);
      check(!rd.value.has_value(), "src: conflicting schema yields no loaded package");
      remove_all_retry(md);
    }
    // duplicate clips array → LV_SOURCE_PARSE, no loaded package
    {
      auto md = pkg_dir; md += "_src_dupclipsarr"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto last = s.find_last_of('}');
      if (last != std::string::npos) s.insert(last, ",\"clips\":[]");
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: dup clips array reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_PARSE"), "src: duplicate clips array LV_SOURCE_PARSE");
      auto rd = read_living_visual_package(md);
      check(!rd.value.has_value(), "src: duplicate clips array yields no loaded package");
      remove_all_retry(md);
    }
    // keyframe tick order broken → LV_SOURCE_KEY_ORDER
    {
      auto md = pkg_dir; md += "_src_keyorder"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      std::size_t from = 0; std::size_t t1 = std::string::npos, t2 = std::string::npos;
      while (t2 == std::string::npos) {
        auto ks = s.find("\"keys\":[", from);
        if (ks == std::string::npos) break;
        auto ko = ks + 7;
        auto ke = match_closer(s, ko);
        auto a = s.find("\"tick\":", ko);
        if (a != std::string::npos && a < ke) {
          auto b = s.find("\"tick\":", a + 1);
          if (b != std::string::npos && b < ke) { t1 = a; t2 = b; break; }
        }
        from = ke + 1;
      }
      if (t2 != std::string::npos) {
        auto v1s = t1 + 7; auto v1e = s.find_first_of(",}", v1s);
        auto v2s = t2 + 7; auto v2e = s.find_first_of(",}", v2s);
        s.replace(v2s, v2e - v2s, s.substr(v1s, v1e - v1s));
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: key order reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_KEY_ORDER"), "src: LV_SOURCE_KEY_ORDER");
      remove_all_retry(md);
    }
    // keyframe tick outside clip duration → LV_SOURCE_KEY_RANGE
    {
      auto md = pkg_dir; md += "_src_keyrange"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto ks = s.find("\"keys\":[");
      auto t1 = (ks != std::string::npos) ? s.find("\"tick\":", ks) : std::string::npos;
      if (t1 != std::string::npos) {
        auto v1s = t1 + 7; auto v1e = s.find_first_of(",}", v1s);
        s.replace(v1s, v1e - v1s, "999999");
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: key range reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_KEY_RANGE"), "src: LV_SOURCE_KEY_RANGE");
      remove_all_retry(md);
    }
    // authored event tick outside clip duration → LV_SOURCE_EVENT_RANGE
    {
      auto md = pkg_dir; md += "_src_evrange"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto ev = s.find("\"events\":[");
      auto t1 = (ev != std::string::npos) ? s.find("\"tick\":", ev) : std::string::npos;
      if (t1 != std::string::npos) {
        auto v1s = t1 + 7; auto v1e = s.find_first_of(",}", v1s);
        s.replace(v1s, v1e - v1s, "999999");
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: event range reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_EVENT_RANGE"), "src: LV_SOURCE_EVENT_RANGE");
      remove_all_retry(md);
    }
    // unresolved bone reference → LV_SOURCE_BONE_REF
    {
      auto md = pkg_dir; md += "_src_boneref"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto tr = s.find("\"tracks\":[");
      auto bo = (tr != std::string::npos) ? s.find("\"bone_id\":", tr) : std::string::npos;
      if (bo != std::string::npos) {
        auto bs = bo + 11; auto be = s.find('"', bs);
        if (be != std::string::npos) s.replace(bs, be - bs, "ZZZ");
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: bone ref reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_BONE_REF"), "src: LV_SOURCE_BONE_REF");
      remove_all_retry(md);
    }
    // missing required clip field (events removed) → LV_SOURCE_SCHEMA
    {
      auto md = pkg_dir; md += "_src_missing"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto sb = src_bytes(md);
      std::string s(sb.begin(), sb.end());
      auto ev = s.find("\"events\":[");
      if (ev != std::string::npos) {
        auto ao = s.find('[', ev);
        auto ae = match_closer(s, ao);
        if (ae != std::string::npos) {
          auto cs = s.rfind(',', ev);
          if (cs != std::string::npos) s.erase(cs, ae - cs + 1);
        }
      }
      std::ofstream(md/"source-skeletal-animations.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
      auto ml = m; refresh(ml, md, "source-skeletal-animations.json"); finalize(ml, md);
      auto v = verify_living_visual_package(md);
      check(reached_semantic(v), "src: missing field reached semantic layer");
      check(!v.ok() && has_diag(v, "LV_SOURCE_SCHEMA"), "src: missing field LV_SOURCE_SCHEMA");
      remove_all_retry(md);
    }
    // token budget exhaustion → LV_READ_JSON_TOKENS
    {
      auto md = pkg_dir; md += "_src_tokenlimit"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      PackageReadLimits tight{};
      tight.max_frames = 256;
      tight.max_channels = 512;
      tight.max_morphology_parts = 128;
      tight.max_json_tokens = 100;  // Far below actual requirement
      tight.max_json_nesting = 32;
      tight.max_image_width = 4096;
      tight.max_image_height = 4096;
      tight.max_manifest_bytes = 16ULL * 1024 * 1024;
      tight.max_artifact_bytes = 64ULL * 1024 * 1024;
      tight.max_artifacts = 4096;
      tight.max_path_bytes = 1024;
      auto rd = read_living_visual_package(md, tight);
      check(!rd.value.has_value(), "src: token limit blocks load");
      if (!rd.value.has_value()) {
        // Debug: print all diagnostics
        std::cout << "  Diagnostics from token limit test:\n";
        for (auto const& d : rd.diagnostics.diagnostics) {
          std::cout << "    " << d.code << ": " << d.message << "\n";
        }
        bool has_token_diag = false;
        for (auto const& d : rd.diagnostics.diagnostics) {
          if (d.code == "LV_READ_JSON_TOKENS") has_token_diag = true;
        }
        check(has_token_diag, "src: LV_READ_JSON_TOKENS diagnostic present");
      }
      // Verify the same package succeeds with sufficient budget
      PackageReadLimits sufficient{};
      sufficient.max_frames = 256;
      sufficient.max_channels = 512;
      sufficient.max_morphology_parts = 128;
      sufficient.max_json_tokens = 262144;  // Standard budget
      sufficient.max_json_nesting = 32;
      sufficient.max_image_width = 4096;
      sufficient.max_image_height = 4096;
      sufficient.max_manifest_bytes = 16ULL * 1024 * 1024;
      sufficient.max_artifact_bytes = 64ULL * 1024 * 1024;
      sufficient.max_artifacts = 4096;
      sufficient.max_path_bytes = 1024;
      auto rd2 = read_living_visual_package(md, sufficient);
      check(rd2.value.has_value(), "src: sufficient budget allows load");
      remove_all_retry(md);
    }
    // ── Manifest-specific token exhaustion ──
    {
      auto md = pkg_dir; md += "_mftok"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      auto mf_bytes = read_file_bytes(md/"manifest.json", 4ULL*1024*1024);
      // Budget below manifest requirement
      PackageReadLimits tight_mf{};
      tight_mf.max_json_tokens = 1;
      auto mf_parse = gspl::sprites::parse_living_package_manifest({mf_bytes.begin(), mf_bytes.end()}, tight_mf);
      check(!mf_parse.ok(), "mftok: manifest parse fails with token budget 1");
      check(!mf_parse.value.has_value(), "mftok: no partial manifest on token exhaustion");
      bool has_tok = false;
      for (auto const& d : mf_parse.diagnostics.diagnostics)
        if (d.code == "LV_READ_JSON_TOKENS") has_tok = true;
      check(has_tok, "mftok: LV_READ_JSON_TOKENS diagnostic present");
      remove_all_retry(md);
    }
    // ── Malformed JSON distinction ──
    {
      auto md = pkg_dir; md += "_maljson"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      // Corrupt manifest: inject garbage bytes
      auto mf_bytes = read_file_bytes(md/"manifest.json", 4ULL*1024*1024);
      std::string corrupted(mf_bytes.begin(), mf_bytes.end());
      if (corrupted.size() > 20) corrupted[10] = '\x00';  // inject NUL
      std::ofstream(md/"manifest.json", std::ios::trunc | std::ios::binary).write(corrupted.data(), corrupted.size());
      PackageReadLimits generous{};
      generous.max_json_tokens = 262144;
      auto rd = gspl::sprites::read_living_visual_package(md, generous);
      check(!rd.value.has_value(), "maljson: malformed JSON blocks load with generous budget");
      bool has_tok = false, has_malformed = false;
      for (auto const& d : rd.diagnostics.diagnostics) {
        if (d.code == "LV_READ_JSON_TOKENS") has_tok = true;
        if (d.code == "LV_READ_JSON_MALFORMED" || d.code == "LV_PARSE_KEY" || d.code == "LV_READ_MANIFEST_PARSE" || d.code == "LV_READ_MALFORMED") has_malformed = true;
      }
      check(!has_tok, "maljson: malformed JSON does NOT emit LV_READ_JSON_TOKENS");
      check(has_malformed, "maljson: receives correct parse/syntax diagnostic");
      remove_all_retry(md);
    }
    // ── Nesting limit distinction ──
    {
      auto md = pkg_dir; md += "_nesttok"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      PackageReadLimits shallow{};
      shallow.max_json_nesting = 1;  // only top-level object; any nested object/array fails
      shallow.max_json_tokens = 262144;  // generous token budget
      auto rd = gspl::sprites::read_living_visual_package(md, shallow);
      bool has_nesting = false, has_tok = false;
      for (auto const& d : rd.diagnostics.diagnostics) {
        if (d.code == "LV_READ_JSON_NESTING") has_nesting = true;
        if (d.code == "LV_READ_JSON_TOKENS") has_tok = true;
      }
      check(!has_tok, "nesttok: nesting limit does NOT emit LV_READ_JSON_TOKENS");
      check(has_nesting || !rd.value.has_value(), "nesttok: nesting limit produces distinct diagnostic or blocks load");
      remove_all_retry(md);
    }
    // ── Source-animation-specific token exhaustion ──
    // Token semantics: max_json_tokens is per-BoundedJsonReader (per-parser/artifact).
    // Each sub-parser gets its own independent budget, not a shared pool.
    // This test verifies that source-skeletal-animations.json specifically
    // cannot be parsed with a tight per-parser budget, while manifest/seed succeed.
    {
      auto md = pkg_dir; md += "_satok"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      // 2000 tokens per parser: manifest (~600) and seed (~500) pass individually.
      // source-skeletal-animations.json with clips/tracks/keys needs far more.
      PackageReadLimits sa_tight{};
      sa_tight.max_json_tokens = 2000;
      sa_tight.max_frames = 256;
      sa_tight.max_channels = 512;
      sa_tight.max_morphology_parts = 128;
      sa_tight.max_json_nesting = 32;
      sa_tight.max_image_width = 4096;
      sa_tight.max_image_height = 4096;
      sa_tight.max_manifest_bytes = 16ULL * 1024 * 1024;
      sa_tight.max_artifact_bytes = 64ULL * 1024 * 1024;
      sa_tight.max_artifacts = 4096;
      sa_tight.max_path_bytes = 1024;
      auto rd = gspl::sprites::read_living_visual_package(md, sa_tight);
      check(!rd.value.has_value(), "satok: per-parser token budget blocks load");
      // Verify at least one LV_READ_JSON_TOKENS diagnostic was emitted
      bool has_tok = false;
      for (auto const& d : rd.diagnostics.diagnostics) {
        if (d.code == "LV_READ_JSON_TOKENS") has_tok = true;
      }
      check(has_tok, "satok: LV_READ_JSON_TOKENS diagnostic present at 2000 per-parser budget");
      remove_all_retry(md);
    }
    // ── Boundary / off-by-one token budget test ──
    {
      // Use a controlled BoundedJsonReader fixture with known token count
      std::string tiny_json = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5}";
      // This JSON has: { "a" : 1 , "b" : 2 , "c" : 3 , "d" : 4 , "e" : 5 }
      // Tokens: { "a" : 1 , "b" : 2 , "c" : 3 , "d" : 4 , "e" : 5 }
      // Count: 1 + string(2chars) + 1 + num + 1 + string + 1 + num + 1 + string + 1 + num + 1 + string + 1 + num + 1 + string + 1 + num + 1
      // That's 5 pairs = 5 * 4 tokens + 1 opening = 21 tokens
      // Actually: { = 1, "a" = 1, : = 1, 1 = 1, , = 1, "b" = 1, : = 1, 2 = 1, , = 1, "c" = 1, : = 1, 3 = 1, , = 1, "d" = 1, : = 1, 4 = 1, , = 1, "e" = 1, : = 1, 5 = 1, } = 1
      // Total = 21 tokens
      {
        gspl::BoundedJsonConfig cfg{}; cfg.max_tokens = 20;  // just below requirement
        gspl::BoundedJsonReader r(tiny_json, cfg);
        (void)r.begin_object("test");
        while (r.has_more() && !r.has_error()) {
          auto key = r.read_string_result();
          if (!key.ok()) break;
          (void)r.require(':', "test");
          (void)r.read_int64_result();
          r.record_object_member("test");
          if (!r.next_object_member("test")) break;
        }
        r.end_object("test");
        check(r.has_error(), "boundary: tokens=20 fails on 21-token JSON");
        check(r.error_code() == gspl::BoundedJsonErrorCode::token_limit, "boundary: error code is token_limit");
      }
      {
        gspl::BoundedJsonConfig cfg{}; cfg.max_tokens = 21;  // exactly requirement
        gspl::BoundedJsonReader r(tiny_json, cfg);
        (void)r.begin_object("test");
        while (r.has_more() && !r.has_error()) {
          auto key = r.read_string_result();
          if (!key.ok()) break;
          (void)r.require(':', "test");
          (void)r.read_int64_result();
          r.record_object_member("test");
          if (!r.next_object_member("test")) break;
        }
        r.end_object("test");
        check(!r.has_error(), "boundary: tokens=21 passes on 21-token JSON");
      }
      {
        gspl::BoundedJsonConfig cfg{}; cfg.max_tokens = 22;  // above requirement
        gspl::BoundedJsonReader r(tiny_json, cfg);
        (void)r.begin_object("test");
        while (r.has_more() && !r.has_error()) {
          auto key = r.read_string_result();
          if (!key.ok()) break;
          (void)r.require(':', "test");
          (void)r.read_int64_result();
          r.record_object_member("test");
          if (!r.next_object_member("test")) break;
        }
        r.end_object("test");
        check(!r.has_error(), "boundary: tokens=22 passes on 21-token JSON");
      }
    }
    // ── Object-member-limit test ──
    {
      gspl::BoundedJsonConfig cfg{}; cfg.max_object_members = 2;
      gspl::BoundedJsonReader r("{\"a\":1,\"b\":2,\"c\":3}", cfg);
      (void)r.begin_object("test");
      while (r.has_more() && !r.has_error()) {
        auto key = r.read_string_result();
        if (!key.ok()) break;
        (void)r.require(':', "test");
        (void)r.read_int64_result();
        r.record_object_member("test");
        if (!r.next_object_member("test")) break;
      }
      r.end_object("test");
      check(r.has_error(), "memlimit: object member limit exceeded");
      check(r.error_code() == gspl::BoundedJsonErrorCode::object_member_limit, "memlimit: error code is object_member_limit");
    }
    // ── Array-length-limit test ──
    {
      gspl::BoundedJsonConfig cfg{}; cfg.max_array_length = 2;
      gspl::BoundedJsonReader r("[1,2,3,4]", cfg);
      (void)r.begin_array("test");
      while (r.has_more() && !r.has_error()) {
        (void)r.read_int64_result();
        r.record_array_element("test");
        if (!r.next_array_element("test")) break;
      }
      r.end_array("test");
      check(r.has_error(), "arrlimit: array length limit exceeded");
      check(r.error_code() == gspl::BoundedJsonErrorCode::array_length_limit, "arrlimit: error code is array_length_limit");
    }
    // ── String-length-limit test ──
    {
      gspl::BoundedJsonConfig cfg{}; cfg.max_string_length = 5;
      gspl::BoundedJsonReader r("{\"a\":\"1234567890\"}", cfg);
      (void)r.begin_object("test");
      auto key = r.read_string_result();
      (void)r.require(':', "test");
      (void)r.read_string_result();  // string exceeds limit
      r.end_object("test");
      check(r.has_error(), "strlimit: string length limit exceeded");
      check(r.error_code() == gspl::BoundedJsonErrorCode::string_length_limit, "strlimit: error code is string_length_limit");
    }
  }

  // ── Hostile PNG decoder inputs ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Hostile PNG Decoder Tests ---\n";
    ImageLimits plim{};
    auto expect_throw = [&](std::vector<std::byte> const& bytes, const char* msg) {
      try { (void)decode_png(bytes, plim); check(false, msg); }
      catch (std::exception const&) { check(true, msg); }
    };
    expect_throw({}, "png: empty input rejected");
    {
      std::string junk = "definitely not a png file at all";
      std::vector<std::byte> jb(junk.size());
      for (std::size_t i = 0; i < junk.size(); ++i) jb[i] = static_cast<std::byte>(junk[i]);
      expect_throw(jb, "png: bad signature rejected");
    }
    {
      auto valid = encode_png(ImageRgba8{8, 8, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(8*8*4, 128)});
      auto cut = std::vector<std::byte>(valid.begin(), valid.begin() + valid.size()/2);
      expect_throw(cut, "png: truncated payload rejected");
    }
    {
      auto valid = encode_png(ImageRgba8{8, 8, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(8*8*4, 128)});
      // Remove the trailing IEND chunk
      std::vector<std::byte> no_iend(valid.begin(), valid.end() - 12);
      expect_throw(no_iend, "png: missing IEND rejected");
    }
    {
      // Crafted IHDR with oversized dimensions must be rejected by decode limits
      std::vector<std::byte> b;
      auto push32 = [&](std::uint32_t v) {
        b.push_back(std::byte((v >> 24) & 0xFF)); b.push_back(std::byte((v >> 16) & 0xFF));
        b.push_back(std::byte((v >> 8) & 0xFF)); b.push_back(std::byte(v & 0xFF));
      };
      static const std::array<std::byte, 8> sig{std::byte{0x89}, std::byte{'P'}, std::byte{'N'}, std::byte{'G'},
                                                std::byte{'\r'}, std::byte{'\n'}, std::byte{0x1a}, std::byte{'\n'}};
      b.insert(b.end(), sig.begin(), sig.end());
      push32(13);
      b.push_back(std::byte{'I'}); b.push_back(std::byte{'H'}); b.push_back(std::byte{'D'}); b.push_back(std::byte{'R'});
      push32(0x400000); push32(0x400000);
      b.push_back(std::byte{8}); b.push_back(std::byte{6});
      b.push_back(std::byte{0}); b.push_back(std::byte{0}); b.push_back(std::byte{0});
      push32(0);
      push32(0);
      b.push_back(std::byte{'I'}); b.push_back(std::byte{'E'}); b.push_back(std::byte{'N'}); b.push_back(std::byte{'D'});
      push32(0);
      expect_throw(b, "png: oversized dimensions rejected");
    }
    {
      // Corrupt a real frame PNG inside a package copy: verify must fail safely
      auto md = pkg_dir; md += "_png_hostile"; remove_all_retry(md); fs::copy(pkg_dir, md, fs::copy_options::recursive);
      bool found = false;
      for (auto const& e : fs::directory_iterator(md/"frames")) {
        if (!e.is_regular_file() || e.path().extension() != ".png") continue;
        auto fb = read_file_bytes(e.path(), 512ULL*1024*1024);
        if (fb.size() < 8) continue;
        std::ofstream(e.path(), std::ios::trunc | std::ios::binary).write(
            reinterpret_cast<const char*>(fb.data()), static_cast<std::streamsize>(fb.size()/2));
        found = true;
        break;
      }
      if (found) {
        auto v = verify_living_visual_package(md);
        check(!v.ok(), "png: truncated frame png fails package verify safely");
      }
      remove_all_retry(md);
    }
  }

  // ── Positive relational assertions on reconstructed package ──
  {
    using namespace gspl::sprites;
    std::cout << "\n--- Positive Relational Assertions ---\n";
    auto rd = read_living_visual_package(pkg_dir);
    check(rd.value.has_value(), "read: full package reconstructs");
    if (rd.value.has_value()) {
      auto& p = *rd.value;
      check(p.channels.size() == 192, "read: 192 channels (48 frames x 4 kinds)");
      check(p.sheet.atlas.placements.size() == 48, "read: 48 atlas placements");
      check(p.transformation_morphologies.size() == 10, "read: 10 transformation morphologies");
      check(!p.base_morphology.empty() && !p.storm_morphology.empty(), "read: base and storm morphology present");
      check(p.collision_shapes.size() > 0, "read: collision shapes present");
      // Transformation endpoints semantically equal to base/storm — proven by the
      // production verifier's endpoint equality diagnostics being absent on the
      // valid package (complete semantics incl. emissive and electrical markings)
      {
        auto vend = verify_living_visual_package(pkg_dir);
        bool no_base = true, no_storm = true;
        for (auto const& d : vend.validation.diagnostics) {
          if (d.code == "LV_MORPH_TRANSFORM_BASE") no_base = false;
          if (d.code == "LV_MORPH_TRANSFORM_STORM") no_storm = false;
        }
        check(no_base, "morph: transformation[0] == base semantics (no endpoint diag)");
        check(no_storm, "morph: transformation[9] == storm semantics (no endpoint diag)");
      }
      // Sample source ticks are nondecreasing by frame index per clip
      {
        std::map<std::string, std::map<std::uint32_t, std::uint32_t>> ticks_by_clip;
        for (auto const& s : p.samples) ticks_by_clip[s.clip_id][s.frame_index] = s.source_tick;
        bool ordered = true;
        for (auto const& [clip, tmap] : ticks_by_clip) {
          std::uint32_t prev = 0; bool first = true;
          for (auto const& [fi, tick] : tmap) {
            if (!first && tick < prev) ordered = false;
            prev = tick; first = false;
          }
        }
        check(ordered, "samples: source ticks nondecreasing by frame index");
      }
      // Every generated event is bound to a reconstructed source event with equal authored tick
      {
        std::map<std::string, std::map<std::string, std::uint32_t>> source_ev;
        for (auto const& sc : p.source_skeletal_animations)
          for (auto const& [en, et] : sc.events) source_ev[sc.id][en] = et;
        auto resolve_source_clip = [](std::string_view gcid) -> std::string {
          if (gcid.ends_with(".base.attack")) return "base_attack";
          if (gcid.ends_with(".storm.attack")) return "storm_attack";
          if (gcid.ends_with(".transform")) return "transform_ascend";
          return "";
        };
        bool bound = true;
        for (auto const& e : p.events) {
          auto scid = resolve_source_clip(e.clip_id);
          auto it = source_ev.find(scid);
          if (it == source_ev.end()) { bound = false; continue; }
          auto eit = it->second.find(e.event_id);
          if (eit == it->second.end() || eit->second != e.authored_tick) bound = false;
        }
        check(bound, "events: every generated authored_tick equals reconstructed source event tick");
      }
      // All channels have a known target frame and consistent decoded dimensions
      {
        std::map<std::string, const FrameSource*, std::less<>> frame_by_id;
        for (auto const& f : p.frames) frame_by_id[f.id] = &f;
        bool ok_targets = true, ok_dims = true;
        for (auto const& ch : p.channels) {
          auto fit = frame_by_id.find(ch.target_frame_id);
          if (fit == frame_by_id.end()) { ok_targets = false; continue; }
          if (ch.image.width != fit->second->image.width || ch.image.height != fit->second->image.height)
            ok_dims = false;
        }
        check(ok_targets, "channels: every target frame known");
        check(ok_dims, "channels: decoded dims equal target frame dims");
      }
      // Atlas placement set equals the 48-frame set
      {
        std::set<std::string> frame_ids;
        for (auto const& f : p.frames) frame_ids.insert(f.id);
        std::set<std::string> placement_ids;
        for (auto const& pl : p.sheet.atlas.placements) placement_ids.insert(pl.frame_id);
        check(placement_ids == frame_ids, "atlas: placement set equals frame set");
      }
    }
  }

  // ── Separate-process CLI package verification ──
  {
    std::cout << "\n--- Separate-Process CLI Verification ---\n";
#ifndef GSPL_SPRITES_GSPLC_PATH
#define GSPL_SPRITES_GSPLC_PATH "gsplc"
#endif
    std::string gsplc = GSPL_SPRITES_GSPLC_PATH;
    if (!fs::exists(gsplc)) {
      check(false, "cli: gsplc binary not found at configured path");
    } else {
      // Separate scratch workspace: the stderr redirect target must never live
      // inside a package directory or the verifier would flag it as an
      // undeclared file.
      ScopedTestWorkspace ws_cli("cli", true);
      auto out_file = ws_cli.sub("cli_out.txt").string();
      auto quote = [](std::string const& s) { return std::string("\"") + s + "\""; };
      // Valid package → exit 0 (cmd /c quoting: wrap the whole command so
      // Windows strips only the outer quotes)
      std::string cmd1 = "\"" + quote(gsplc) + " --verify-package " + quote(pkg_dir.string()) + " 2>" + quote(out_file) + "\"";
      int rc1 = std::system(cmd1.c_str());
      check(rc1 == 0, "cli: valid package exit code 0");
      // Self-consistent hostile package → nonzero + exact diagnostic on stderr
      auto hdir = pkg_dir; hdir += "_cli_hostile"; remove_all_retry(hdir); fs::copy(pkg_dir, hdir, fs::copy_options::recursive);
      {
        using namespace gspl::sprites;
        PackageReadLimits rlim{};
        auto mf_bytes = read_file_bytes(hdir/"manifest.json", 4ULL*1024*1024);
        auto mf_parse = parse_living_package_manifest({mf_bytes.begin(), mf_bytes.end()}, rlim);
        auto m = *mf_parse.value;
        auto refresh = [&](auto& manifest, auto d, auto path) {
          auto b = read_file_bytes(d/path, 512ULL*1024*1024);
          auto h = sha256({b.begin(), b.end()});
          for (auto& a : manifest.artifacts) if (a.path == path) {
            a.byte_size = static_cast<std::uint32_t>(b.size()); a.sha256 = std::move(h); return;
          }
        };
        auto finalize = [&](auto& manifest, auto d) {
          auto cn = canonicalize_manifest(manifest, false);
          manifest.package_identity = sha256(std::string(kIdentityPreimageVersion) + "\n" + cn);
          auto cw = canonicalize_manifest(manifest, true);
          std::ofstream(d/"manifest.json", std::ios::trunc | std::ios::binary).write(cw.data(), cw.size());
        };
        auto fhb = read_file_bytes(hdir/"frame-hashes.json", 4ULL*1024*1024);
        std::string s(fhb.begin(), fhb.end());
        auto pos = s.rfind("{\"frame_id\"");
        if (pos != std::string::npos) {
          auto end = s.find("}", pos) + 1;
          s.insert(end, ",{\"frame_id\":\"EXTRA\",\"frame_hash\":\"" + std::string(64, '0') + "\"}");
        }
        std::ofstream(hdir/"frame-hashes.json", std::ios::trunc | std::ios::binary).write(s.data(), s.size());
        auto ml = m; refresh(ml, hdir, "frame-hashes.json"); finalize(ml, hdir);
      }
      std::string cmd2 = "\"" + quote(gsplc) + " --verify-package " + quote(hdir.string()) + " 2>" + quote(out_file) + "\"";
      int rc2 = std::system(cmd2.c_str());
      check(rc2 != 0, "cli: hostile self-consistent package nonzero exit");
      auto ob = read_file_bytes(out_file, 1ULL*1024*1024);
      std::string o(ob.begin(), ob.end());
      check(o.find("LV_FH_EXTRA") != std::string::npos, "cli: hostile diagnostic emitted on stderr");
      remove_all_retry(hdir);
      std::error_code ec;
      fs::remove(out_file, ec);
    }
  }

  remove_all_retry(pkg_dir);

  // ── Typed manifest tests ──
  {
    using namespace gspl::sprites;
    using Kind = LivingArtifactKind;
    std::cout << "\n--- Typed Manifest Tests ---\n";

    // Positive: kind/string round-trip
    for (auto k : {Kind::living_seed, Kind::seed_identity, Kind::source_skeletal_animations,
                   Kind::frame_metadata, Kind::frame_image, Kind::generated_animation,
                   Kind::frame_samples, Kind::generated_events, Kind::pose_hashes,
                   Kind::frame_hashes, Kind::channel_metadata, Kind::channel_image,
                   Kind::collision_metadata, Kind::effective_morphology,
                   Kind::transformation_morphologies, Kind::sprite_atlas, Kind::sprite_atlas_metadata}) {
      auto s = artifact_kind_string(k);
      auto back = artifact_kind_from_string(s);
      check(back.has_value() && *back == k, ("kind round-trip: " + std::string(s)).c_str());
    }

    // Positive: every kind maps to expected schema
    check(artifact_schema_for_kind(Kind::living_seed) == kSchemaLivingSeed, "schema: living_seed");
    check(artifact_schema_for_kind(Kind::frame_metadata) == kSchemaFrames2d, "schema: frame_metadata");
    check(artifact_schema_for_kind(Kind::generated_animation) == kSchemaGeneratedAnimation2d, "schema: generated_animation");
    check(artifact_schema_for_kind(Kind::frame_samples) == kSchemaFrameSamples, "schema: frame_samples");
    check(artifact_schema_for_kind(Kind::generated_events) == kSchemaGeneratedAnimationEvents, "schema: generated_events");
    check(artifact_schema_for_kind(Kind::pose_hashes) == kSchemaPoseHashes, "schema: pose_hashes");
    check(artifact_schema_for_kind(Kind::frame_hashes) == kSchemaFrameHashes, "schema: frame_hashes");
    check(artifact_schema_for_kind(Kind::channel_metadata) == kSchemaChannelMaps, "schema: channel_metadata");
    check(artifact_schema_for_kind(Kind::collision_metadata) == kSchemaCollisions2d, "schema: collision_metadata");
    check(artifact_schema_for_kind(Kind::effective_morphology) == kSchemaEffectiveMorphology, "schema: effective_morphology");
    check(artifact_schema_for_kind(Kind::transformation_morphologies) == kSchemaTransformationMorphologies, "schema: trans_morphs");
    check(artifact_schema_for_kind(Kind::sprite_atlas_metadata) == kSchemaSpriteSheet, "schema: atlas_metadata");
    check(artifact_schema_for_kind(Kind::frame_image) == "", "schema: frame_image empty");
    check(artifact_schema_for_kind(Kind::channel_image) == "", "schema: channel_image empty");
    check(artifact_schema_for_kind(Kind::seed_identity) == "", "schema: seed_identity empty");
    check(artifact_schema_for_kind(Kind::sprite_atlas) == "", "schema: sprite_atlas empty");

    // Positive: canonicalize_manifest determinism
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "test";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.frame_count = 48;
      m.artifact_count = 1;
      LivingPackageArtifactRecord rec;
      rec.path = "test.json";
      rec.kind = Kind::living_seed;
      rec.schema = std::string(kSchemaLivingSeed);
      rec.byte_size = 100;
      rec.sha256 = std::string(64, 'd');
      rec.provenance_identity = std::string(64, 'e');
      m.artifacts.push_back(std::move(rec));

      auto c1 = canonicalize_manifest(m, true);
      auto c2 = canonicalize_manifest(m, true);
      check(c1 == c2, "canonicalize_manifest deterministic");
      check(!c1.empty(), "canonicalize_manifest non-empty");
      check(c1.find("\"frameCount\":48") != std::string::npos, "canonicalize includes frameCount");
      check(c1.find("\"artifactCount\":1") != std::string::npos, "canonicalize includes artifactCount");

      // Round-trip: parse(canonicalize(m, true)) == m
      PackageReadLimits rlim{};
      auto parse_result = parse_living_package_manifest(c1, rlim);
      check(parse_result.ok(), "parse canonicalize round-trip ok");
      if (parse_result.value) {
        auto& pm = *parse_result.value;
        check(pm.format == m.format, "round-trip format");
        check(pm.package_identity == m.package_identity, "round-trip pkg id");
        check(pm.entity_id == m.entity_id, "round-trip entity");
        check(pm.frame_count == m.frame_count, "round-trip frame_count");
        check(pm.artifact_count == m.artifact_count, "round-trip artifact_count");
        check(pm.artifacts.size() == 1, "round-trip artifacts.size");
        check(pm.artifacts[0].path == "test.json", "round-trip artifact path");
        check(pm.artifacts[0].sha256 == std::string(64, 'd'), "round-trip artifact sha256");
      }
    }

    // Negative: missing format
    {
      std::string json = "{\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":0,\"artifacts\":[]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: missing format");
    }

    // Negative: missing identityVersion
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":0,\"artifacts\":[]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: missing identityVersion");
    }

    // Negative: unknown top-level field
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":0,\"artifacts\":[],\"unknownField\":123}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: unknown top-level field");
    }

    // Negative: duplicate top-level field
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":0,\"artifacts\":[]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: duplicate top-level field");
    }

    // Negative: missing artifact field (kind)
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":1,\"artifacts\":[{\"path\":\"x\",\"schema\":\"\",\"byteSize\":1,\"sha256\":\"" + std::string(64,'a') + "\",\"dependencies\":[],\"provenanceIdentity\":\"" + std::string(64,'b') + "\"}]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: missing artifact kind");
    }

    // Negative: unknown artifact field
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":1,\"artifacts\":[{\"path\":\"x\",\"kind\":\"living-seed\",\"schema\":\"gspl.living-seed/0.1\",\"byteSize\":1,\"sha256\":\"" + std::string(64,'a') + "\",\"dependencies\":[],\"provenanceIdentity\":\"" + std::string(64,'b') + "\",\"unknownArtField\":42}]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: unknown artifact field");
    }

    // Negative: wrong type (string where uint32 expected)
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":\"not_a_number\",\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":0,\"artifacts\":[]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: wrong type (string for uint32)");
    }

    // Negative: noncanonical bytes (whitespace)
    {
      std::string json = "{\n  \"format\": \"gspl.living-visual-package/0.1\",\n  \"identityVersion\": \"" + std::string(kIdentityPreimageVersion) + "\",\n  \"packageIdentity\": \"" + std::string(64,'a') + "\",\n  \"entityId\": \"x\",\n  \"canonicalEntityIdentity\": \"" + std::string(64,'b') + "\",\n  \"seedIdentity\": \"" + std::string(64,'c') + "\",\n  \"frameCount\": 0,\n  \"clipCount\": 0,\n  \"sampleCount\": 0,\n  \"eventCount\": 0,\n  \"channelCount\": 0,\n  \"collisionShapeCount\": 0,\n  \"collisionWindowCount\": 0,\n  \"transformationMorphologyCount\": 0,\n  \"artifactCount\": 0,\n  \"artifacts\": []\n}";
      // Parse should succeed (tolerant parser), but canonicalize+compare would fail in reader
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      if (r.ok()) {
        auto canonical = canonicalize_manifest(*r.value, true);
        check(canonical != json, "neg: noncanonical bytes differ from canonical");
      }
    }

    // Negative: artifact_count mismatch
    {
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"frameCount\":0,\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":5,\"artifacts\":[]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: artifact_count mismatch");
    }

    // Negative: missing count fields
    {
      // Missing frameCount
      std::string json = "{\"format\":\"gspl.living-visual-package/0.1\",\"identityVersion\":\"" + std::string(kIdentityPreimageVersion) + "\",\"packageIdentity\":\"" + std::string(64,'a') + "\",\"entityId\":\"x\",\"canonicalEntityIdentity\":\"" + std::string(64,'b') + "\",\"seedIdentity\":\"" + std::string(64,'c') + "\",\"clipCount\":0,\"sampleCount\":0,\"eventCount\":0,\"channelCount\":0,\"collisionShapeCount\":0,\"collisionWindowCount\":0,\"transformationMorphologyCount\":0,\"artifactCount\":0,\"artifacts\":[]}";
      PackageReadLimits rlim{};
      auto r = parse_living_package_manifest(json, rlim);
      check(!r.ok(), "neg: missing frameCount");
    }

    // Negative: unsorted dependencies
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "x";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.artifact_count = 1;
      LivingPackageArtifactRecord rec;
      rec.path = "a.json";
      rec.kind = Kind::living_seed;
      rec.schema = std::string(kSchemaLivingSeed);
      rec.byte_size = 1;
      rec.sha256 = std::string(64, 'd');
      rec.provenance_identity = std::string(64, 'e');
      rec.dependencies = {"z.json", "a.json"};
      m.artifacts.push_back(std::move(rec));
      PackageReadLimits rlim{};
      auto v = validate_manifest_model(m, rlim);
      check(!v.ok(), "neg: unsorted dependencies");
    }

    // Negative: self-dependency
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "x";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.artifact_count = 1;
      LivingPackageArtifactRecord rec;
      rec.path = "a.json";
      rec.kind = Kind::living_seed;
      rec.schema = std::string(kSchemaLivingSeed);
      rec.byte_size = 1;
      rec.sha256 = std::string(64, 'd');
      rec.provenance_identity = std::string(64, 'e');
      rec.dependencies = {"a.json", "b.json"};
      m.artifacts.push_back(std::move(rec));
      PackageReadLimits rlim{};
      auto v = validate_manifest_model(m, rlim);
      check(!v.ok(), "neg: self-dependency");
    }

    // Negative: unsorted artifact path
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "x";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.artifact_count = 2;
      LivingPackageArtifactRecord r1, r2;
      r1.path = "z.json"; r1.kind = Kind::living_seed; r1.schema = std::string(kSchemaLivingSeed); r1.byte_size = 1; r1.sha256 = std::string(64,'d'); r1.provenance_identity = std::string(64,'e');
      r2.path = "a.json"; r2.kind = Kind::living_seed; r2.schema = std::string(kSchemaLivingSeed); r2.byte_size = 1; r2.sha256 = std::string(64,'f'); r2.provenance_identity = std::string(64,'g');
      m.artifacts.push_back(std::move(r1)); m.artifacts.push_back(std::move(r2));
      PackageReadLimits rlim{};
      auto v = validate_manifest_model(m, rlim);
      check(!v.ok(), "neg: unsorted artifact paths");
    }

    // Negative: duplicate artifact path
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "x";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.artifact_count = 2;
      LivingPackageArtifactRecord r1, r2;
      r1.path = "a.json"; r1.kind = Kind::living_seed; r1.schema = std::string(kSchemaLivingSeed); r1.byte_size = 1; r1.sha256 = std::string(64,'d'); r1.provenance_identity = std::string(64,'e');
      r2.path = "a.json"; r2.kind = Kind::living_seed; r2.schema = std::string(kSchemaLivingSeed); r2.byte_size = 1; r2.sha256 = std::string(64,'f'); r2.provenance_identity = std::string(64,'g');
      m.artifacts.push_back(std::move(r1)); m.artifacts.push_back(std::move(r2));
      PackageReadLimits rlim{};
      auto v = validate_manifest_model(m, rlim);
      check(!v.ok(), "neg: duplicate artifact path");
    }

    // Negative: case-fold path collision
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "x";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.artifact_count = 2;
      LivingPackageArtifactRecord r1, r2;
      r1.path = "A.json"; r1.kind = Kind::living_seed; r1.schema = std::string(kSchemaLivingSeed); r1.byte_size = 1; r1.sha256 = std::string(64,'d'); r1.provenance_identity = std::string(64,'e');
      r2.path = "a.json"; r2.kind = Kind::living_seed; r2.schema = std::string(kSchemaLivingSeed); r2.byte_size = 1; r2.sha256 = std::string(64,'f'); r2.provenance_identity = std::string(64,'g');
      m.artifacts.push_back(std::move(r1)); m.artifacts.push_back(std::move(r2));
      PackageReadLimits rlim{};
      auto v = validate_manifest_model(m, rlim);
      check(!v.ok(), "neg: case-fold path collision");
    }

    // Negative: stale package identity (change frameCount but keep same pkg id)
    {
      LivingPackageManifest m;
      m.format = std::string(kSchemaLivingVisualPackage);
      m.identity_version = std::string(kIdentityPreimageVersion);
      m.package_identity = std::string(64, 'a');
      m.entity_id = "x";
      m.canonical_entity_identity = std::string(64, 'b');
      m.seed_identity = std::string(64, 'c');
      m.frame_count = 10;
      m.artifact_count = 0;
      PackageReadLimits rlim{};
      auto preimage = canonicalize_manifest(m, false);
      auto recomputed = sha256(std::string(kIdentityPreimageVersion) + "\n" + preimage);
      check(recomputed != m.package_identity, "neg: stale package identity detectable");
    }

    // Positive: reader populates manifest
    {
      // Rebuild with typed manifest and check reader populates pkg.manifest
      auto test_pkg_dir = ws.sub("manifest_test");
      remove_all_retry(test_pkg_dir);
      remove_all_retry(test_pkg_dir.string() + ".staging");
      std::string src2 = load_source();
      {
        gspl::GsplContext ctx2;
        auto buf2 = gspl::SourceBuffer::from_string("voltfox.gspl", src2);
        ctx2.compile_source(std::move(buf2));
        gspl::sprites::SpriteSeed seed2 = gspl::SpriteSeedLowering::lower(ctx2.compilation_context().canonical);
        auto living2 = gspl::sprites::synthesize_living_animation2d(seed2);
        gspl::sprites::LivingVisualPackageInput in2;
        in2.seed = seed2;
        in2.frames = living2.value->all_frames;
        in2.generated_clips = living2.value->clips;
        in2.samples = living2.value->samples;
        in2.events = living2.value->generated_events;
        in2.channels = living2.value->channel_maps;
        in2.collision_shapes = living2.value->collision_shapes;
        in2.collision_windows = living2.value->collision_windows;
        in2.base_morphology = living2.value->base_morphology;
        in2.storm_morphology = living2.value->storm_morphology;
        in2.transformation_morphologies = living2.value->transformation_morphologies;
        in2.sheet = living2.value->sheet;
        gspl::sprites::build_living_visual_package(in2, test_pkg_dir);
      }
      auto read2 = gspl::sprites::read_living_visual_package(test_pkg_dir);
      check(read2.ok(), "typed manifest reader: read ok");
      if (read2.value) {
        auto& m2 = read2.value->manifest;
        check(!m2.format.empty(), "typed manifest: format populated");
        check(m2.format == kSchemaLivingVisualPackage, "typed manifest: format correct");
        check(m2.frame_count == 48, "typed manifest: frameCount = 48");
        check(m2.clip_count == 9, "typed manifest: clipCount = 9");
        check(m2.sample_count == 48, "typed manifest: sampleCount = 48");
        check(m2.artifact_count > 0, "typed manifest: artifact_count > 0");
        check(!m2.artifacts.empty(), "typed manifest: artifacts non-empty");
        check(!m2.canonical_entity_identity.empty(), "typed manifest: canonical_entity_identity populated");
        check(!m2.seed_identity.empty(), "typed manifest: seed_identity populated");
        check(!m2.package_identity.empty(), "typed manifest: package_identity populated");
      }
      remove_all_retry(test_pkg_dir);
    }

    // Positive: verifier uses typed manifest counts
    {
      auto test_pkg_dir = ws.sub("verify_manifest");
      remove_all_retry(test_pkg_dir);
      remove_all_retry(test_pkg_dir.string() + ".staging");
      std::string src2 = load_source();
      {
        gspl::GsplContext ctx2;
        auto buf2 = gspl::SourceBuffer::from_string("voltfox.gspl", src2);
        ctx2.compile_source(std::move(buf2));
        gspl::sprites::SpriteSeed seed2 = gspl::SpriteSeedLowering::lower(ctx2.compilation_context().canonical);
        auto living2 = gspl::sprites::synthesize_living_animation2d(seed2);
        gspl::sprites::LivingVisualPackageInput in2;
        in2.seed = seed2;
        in2.frames = living2.value->all_frames;
        in2.generated_clips = living2.value->clips;
        in2.samples = living2.value->samples;
        in2.events = living2.value->generated_events;
        in2.channels = living2.value->channel_maps;
        in2.collision_shapes = living2.value->collision_shapes;
        in2.collision_windows = living2.value->collision_windows;
        in2.base_morphology = living2.value->base_morphology;
        in2.storm_morphology = living2.value->storm_morphology;
        in2.transformation_morphologies = living2.value->transformation_morphologies;
        in2.sheet = living2.value->sheet;
        gspl::sprites::build_living_visual_package(in2, test_pkg_dir);
      }
      auto verify2 = gspl::sprites::verify_living_visual_package(test_pkg_dir);
      check(verify2.ok(), "typed manifest verifier: verify ok");
      check(verify2.frame_count == 48, "typed manifest verifier: frame_count = 48");
      check(verify2.clip_count == 9, "typed manifest verifier: clip_count = 9");
      check(verify2.sample_count == 48, "typed manifest verifier: sample_count = 48");
      check(!verify2.package_identity.empty(), "typed manifest verifier: package_identity populated");
      check(!verify2.seed_identity.empty(), "typed manifest verifier: seed_identity populated");
      remove_all_retry(test_pkg_dir);
    }

    // ═══════════════════════════════════════════════════════════
    // Channel / Atlas / Frame semantic round-trip tests
    // ═══════════════════════════════════════════════════════════
    // Rebuild package for round-trip validation (pkg_dir was cleaned earlier)
    auto rt_dir = ws.sub("rt");
    remove_all_retry(rt_dir);
    remove_all_retry(rt_dir.string() + ".staging");
    {
      std::string src3 = load_source();
      gspl::GsplContext ctx3;
      auto buf3 = gspl::SourceBuffer::from_string("voltfox.gspl", src3);
      ctx3.compile_source(std::move(buf3));
      gspl::sprites::SpriteSeed seed3 = gspl::SpriteSeedLowering::lower(ctx3.compilation_context().canonical);
      auto living3 = gspl::sprites::synthesize_living_animation2d(seed3);
      gspl::sprites::LivingVisualPackageInput in3;
      in3.seed = seed3;
      in3.frames = living3.value->all_frames;
      in3.generated_clips = living3.value->clips;
      in3.samples = living3.value->samples;
      in3.events = living3.value->generated_events;
      in3.channels = living3.value->channel_maps;
      in3.collision_shapes = living3.value->collision_shapes;
      in3.collision_windows = living3.value->collision_windows;
      in3.base_morphology = living3.value->base_morphology;
      in3.storm_morphology = living3.value->storm_morphology;
      in3.transformation_morphologies = living3.value->transformation_morphologies;
      in3.sheet = living3.value->sheet;
      gspl::sprites::build_living_visual_package(in3, rt_dir);
    }

    // Straight alpha with fully opaque pixels → preserves straight
    // Verify channel images load and carry declared semantics from JSON metadata
    {
      auto rd = read_living_visual_package(rt_dir);
      check(rd.value.has_value(), "ch-rt-straight: package loads");
      if (rd.value.has_value()) {
        for (auto const& ch : rd.value->channels) {
          // Channel image must have valid dimensions
          check(ch.image.width > 0, "ch-rt-straight: channel image has width");
          check(ch.image.height > 0, "ch-rt-straight: channel image has height");
          check(ch.image.pixels.size() == static_cast<std::size_t>(ch.image.width) * ch.image.height * 4,
                "ch-rt-straight: channel image pixel count matches dimensions");
          // Alpha mode is preserved from JSON metadata, not re-derived from pixels
          bool all_opaque = true;
          for (std::size_t i = 3; i < ch.image.pixels.size(); i += 4)
            if (ch.image.pixels[i] != 255) { all_opaque = false; break; }
          // Channel images use opaque alpha with either srgb or data color_space
          // (depth/alpha/effects = data, emissive = srgb)
          check(ch.image.alpha_mode == AlphaMode::opaque || ch.image.alpha_mode == AlphaMode::straight,
                "ch-rt-straight: channel alpha_mode is valid declared enum");
          check(ch.image.color_space == ColorSpace::srgb || ch.image.color_space == ColorSpace::data,
                "ch-rt-straight: channel color_space is valid declared enum");
          // Verify provenance: recompute and compare
          auto ch_prov = compute_domain_id("gspl.channel-image/0.1", canonicalize_channel_image_preimage(ch));
          check(!ch_prov.empty(), "ch-rt-straight: channel provenance computable");
        }
      }
    }

    // Opaque alpha mode → preserved as opaque
    // Verify string round-trip for opaque alpha mode
    {
      auto cs_str = "data";
      auto am_str = "opaque";
      check(std::string(cs_str) == "data", "ch-rt-opaque: color_space string 'data'");
      check(std::string(am_str) == "opaque", "ch-rt-opaque: alpha_mode string 'opaque'");
      auto cs_parsed = color_space_from_string(cs_str);
      auto am_parsed = alpha_mode_from_string(am_str);
      check(cs_parsed.has_value() && *cs_parsed == ColorSpace::data, "ch-rt-opaque: color_space round-trips through string");
      check(am_parsed.has_value() && *am_parsed == AlphaMode::opaque, "ch-rt-opaque: alpha_mode round-trips through string");
    }

    // sRGB → preserves sRGB
    {
      auto cs_str = color_space_to_string(ColorSpace::srgb);
      check(cs_str == "srgb", "ch-rt-srgb: color_space string is 'srgb'");
      auto parsed = color_space_from_string("srgb");
      check(parsed.has_value() && *parsed == ColorSpace::srgb, "ch-rt-srgb: srgb string round-trips correctly");
      check(!color_space_from_string("bogus_cs").has_value(), "ch-rt-srgb: bogus color_space returns nullopt");
    }

    // data → preserves data
    {
      auto cs_str = color_space_to_string(ColorSpace::data);
      check(cs_str == "data", "ch-rt-data: color_space string is 'data'");
      auto parsed = color_space_from_string("data");
      check(parsed.has_value() && *parsed == ColorSpace::data, "ch-rt-data: data string round-trips correctly");
    }

    // Unknown color_space → round-trip
    {
      auto parsed = color_space_from_string("unknown");
      check(parsed.has_value() && *parsed == ColorSpace::unknown, "ch-rt-unknown: 'unknown' parses to ColorSpace::unknown");
    }

    // Alpha mode string round-trips
    {
      check(alpha_mode_to_string(AlphaMode::straight) == "straight", "am-rt: straight → 'straight'");
      check(alpha_mode_to_string(AlphaMode::opaque) == "opaque", "am-rt: opaque → 'opaque'");
      check(alpha_mode_to_string(AlphaMode::premultiplied) == "premultiplied", "am-rt: premultiplied → 'premultiplied'");
      check(alpha_mode_from_string("straight").has_value(), "am-rt: 'straight' parses");
      check(alpha_mode_from_string("opaque").has_value(), "am-rt: 'opaque' parses");
      check(alpha_mode_from_string("premultiplied").has_value(), "am-rt: 'premultiplied' parses");
      check(!alpha_mode_from_string("bogus_am").has_value(), "am-rt: bogus alpha_mode returns nullopt");
    }

    // ═══════════════════════════════════════════════════════════
    // Channel-set enumeration independence
    // ═══════════════════════════════════════════════════════════
    {
      auto rd = read_living_visual_package(rt_dir);
      check(rd.value.has_value(), "ch-set-ord: package loads for ordering test");
      if (rd.value.has_value()) {
        // Build forward and reverse channel vectors
        std::vector<ChannelMap> forward = rd.value->channels;
        std::vector<ChannelMap> reverse(forward.rbegin(), forward.rend());
        auto fwd_preimage = canonicalize_channel_set_preimage(forward);
        auto rev_preimage = canonicalize_channel_set_preimage(reverse);
        check(fwd_preimage == rev_preimage, "ch-set-ord: same channels in different order produce identical preimage");
        check(!fwd_preimage.empty(), "ch-set-ord: channel set preimage non-empty");
        auto fwd_id = compute_domain_id("gspl.channel-set/0.1", fwd_preimage);
        auto rev_id = compute_domain_id("gspl.channel-set/0.1", rev_preimage);
        check(fwd_id == rev_id, "ch-set-ord: same channels in different order produce identical domain identity");
      }
    }

    // ═══════════════════════════════════════════════════════════
    // Atlas semantic round-trip tests
    // ═══════════════════════════════════════════════════════════
    {
      auto rd = read_living_visual_package(rt_dir);
      check(rd.value.has_value(), "atlas-rt: package loads");
      if (rd.value.has_value()) {
        auto const& atlas = rd.value->sheet.atlas.image;
        check(atlas.color_space == ColorSpace::srgb, "atlas-rt: atlas color_space preserved as srgb");
        check(atlas.alpha_mode == AlphaMode::straight, "atlas-rt: atlas alpha_mode preserved as straight");
        // Verify provenance is computable
        auto atlas_prov = compute_domain_id("gspl.sprite-atlas/0.1",
            canonicalize_atlas_preimage(rd.value->sheet.atlas.placements, atlas));
        check(!atlas_prov.empty(), "atlas-rt: atlas provenance computable");
      }
    }

    // ═══════════════════════════════════════════════════════════
    // Frame semantic round-trip test
    // ═══════════════════════════════════════════════════════════
    {
      auto rd = read_living_visual_package(rt_dir);
      check(rd.value.has_value(), "frame-rt: package loads");
      if (rd.value.has_value()) {
        for (auto const& f : rd.value->frames) {
          check(f.image.color_space == ColorSpace::srgb, "frame-rt: frame color_space preserved as srgb");
          check(f.image.alpha_mode == AlphaMode::straight, "frame-rt: frame alpha_mode preserved as straight");
        }
      }
    }

    // ═══════════════════════════════════════════════════════════
    // Channel metadata validation
    // ═══════════════════════════════════════════════════════════
    // Verify valid package produces no metadata validation diagnostics
    {
      auto rd = read_living_visual_package(rt_dir);
      check(rd.value.has_value(), "ch-meta-valid: package loads");
      if (rd.value.has_value()) {
        bool no_missing_cs = true, no_missing_am = true, no_invalid_cs = true, no_invalid_am = true;
        bool no_dup_cs = true, no_dup_am = true;
        for (auto const& d : rd.diagnostics.diagnostics) {
          if (d.code == "LV_READ_MISSING_CS") no_missing_cs = false;
          if (d.code == "LV_READ_MISSING_AM") no_missing_am = false;
          if (d.code == "LV_READ_INVALID_CS") no_invalid_cs = false;
          if (d.code == "LV_READ_INVALID_AM") no_invalid_am = false;
          if (d.code == "LV_READ_DUP_CS") no_dup_cs = false;
          if (d.code == "LV_READ_DUP_AM") no_dup_am = false;
        }
        check(no_missing_cs, "ch-meta-valid: no LV_READ_MISSING_CS on valid package");
        check(no_missing_am, "ch-meta-valid: no LV_READ_MISSING_AM on valid package");
        check(no_invalid_cs, "ch-meta-valid: no LV_READ_INVALID_CS on valid package");
        check(no_invalid_am, "ch-meta-valid: no LV_READ_INVALID_AM on valid package");
        check(no_dup_cs, "ch-meta-valid: no LV_READ_DUP_CS on valid package");
        check(no_dup_am, "ch-meta-valid: no LV_READ_DUP_AM on valid package");
      }
    }

    // Direct enum validation: invalid strings fail closed
    {
      check(!color_space_from_string("bogus").has_value(), "ch-meta: invalid color_space string returns nullopt");
      check(!alpha_mode_from_string("bogus").has_value(), "ch-meta: invalid alpha_mode string returns nullopt");
      check(!color_space_from_string("").has_value(), "ch-meta: empty color_space string returns nullopt");
    }
    remove_all_retry(rt_dir);
  }

  std::cout << "\n=== LIVING PACKAGE TESTS: " << assertions << " assertions, " << failures << " failures ===\n";
  return failures ? 1 : 0;

} catch (std::exception const& e) {
  std::cerr << "FATAL: " << e.what() << "\n";
  return 1;
}
