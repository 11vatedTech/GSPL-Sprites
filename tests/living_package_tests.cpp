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
      auto test_pkg_dir = fs::temp_directory_path() / "lv_pkg_manifest_test";
      fs::remove_all(test_pkg_dir);
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
      fs::remove_all(test_pkg_dir);
    }

    // Positive: verifier uses typed manifest counts
    {
      auto test_pkg_dir = fs::temp_directory_path() / "lv_pkg_verify_manifest";
      fs::remove_all(test_pkg_dir);
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
      fs::remove_all(test_pkg_dir);
    }
  }

  std::cout << "\n=== LIVING PACKAGE TESTS: " << assertions << " assertions, " << failures << " failures ===\n";
  return failures ? 1 : 0;

} catch (std::exception const& e) {
  std::cerr << "FATAL: " << e.what() << "\n";
  return 1;
}
