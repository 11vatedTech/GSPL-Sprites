#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/transformation.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"
#include "gspl_sprites/visual_set.hpp"
#include "gspl_sprites/gltf_export.hpp"
#include "gspl_sprites/gltf_verify.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace gspl::sprites;

namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

void check_throws(std::string_view label, auto fn) {
  try { fn(); throw std::runtime_error(std::string(label) + " did not throw"); }
  catch (const std::exception &) {}
}

std::string voltfox_seed_text() {
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

ImageRgba8 make_frame(std::uint32_t w, std::uint32_t h, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  ImageRgba8 img{w, h, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(w * h * 4)};
  for (std::size_t i = 0; i < img.pixels.size(); i += 4) {
    img.pixels[i] = r; img.pixels[i + 1] = g; img.pixels[i + 2] = b; img.pixels[i + 3] = 255;
  }
  return img;
}

struct TestFixture {
  std::filesystem::path root;

  TestFixture() : root(std::filesystem::temp_directory_path() / "gspl-voltfox-integration") {
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
  }

  ~TestFixture() { std::filesystem::remove_all(root); }

  std::filesystem::path sub(const std::string &name) const { return root / name; }
};

void write_binary(const std::filesystem::path &path, std::span<const std::byte> bytes) {
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  if (!out) throw std::runtime_error("cannot write: " + path.string());
}

void write_text(const std::filesystem::path &path, std::string_view text) {
  write_binary(path, std::as_bytes(std::span(text)));
}

[[maybe_unused]] static bool has_artifact(const PackageVerification &pv, std::string_view path) {
  return std::any_of(pv.validation.diagnostics.begin(), pv.validation.diagnostics.end(),
      [&](const Diagnostic &d) { return d.code == "SPRITE_PACKAGE_REQUIRED_ARTIFACT_MISSING" && d.message.find(path) != std::string_view::npos; });
}
} // namespace

int main() try {
  // === 1. Seed parsing, validation, canonicalization ===
  {
    const auto seed = parse_seed(voltfox_seed_text());
    const auto result = validate(seed);
    check(result.ok(), "valid voltfox seed was rejected");
    check(seed.stable_id == "original.voltfox", "parsed stable_id mismatch");
    check(seed.name == "Voltfox", "parsed name mismatch");
    check(seed.rights == RightsClass::original_user_creation, "parsed rights mismatch");
    check(seed.entropy_root == 11072026ULL, "parsed entropy_root mismatch");
    check(seed.abilities.size() == 1, "parsed ability count mismatch");
    check(seed.abilities[0].id == "directional-lightning", "parsed ability id mismatch");

    const auto ir = compile(seed);
    check(ir.seed_identity.size() == 64, "seed identity is not SHA-256 hex");
    check(ir.entity_id == "original.voltfox", "entity_id mismatch");

    const auto id1 = canonicalize(seed);
    const auto id2 = canonicalize(seed);
    check(id1 == id2, "canonicalization is not deterministic");

    const auto svg = render_svg(ir);
    check(svg.find("<svg") != std::string::npos, "rendered SVG is not valid XML");
    check(!svg.empty(), "rendered SVG is empty");
    std::cout << "  [PASS] Seed parsing, canonicalization, SVG render\n";
  }

  // === 2. Build package from seed-only, verify ===
  {
    TestFixture fix;
    const auto seed = parse_seed(voltfox_seed_text());
    const auto output = fix.sub("package");
    build_package(seed, output);
    const auto verification = verify_package(output);
    check(verification.ok(), "seed-only package verification failed");
    check(verification.entity_id == "original.voltfox", "entity_id in verification mismatch");
    check(!verification.seed_identity.empty(), "seed_identity is empty");
    check(!verification.package_identity.empty(), "package_identity is empty");
    check(verification.artifact_count > 0, "no artifacts in package");
    check(verification.total_artifact_bytes > 0, "zero bytes in package");
    std::cout << "  [PASS] Seed-only package build + verify (" << verification.artifact_count
              << " artifacts, " << verification.total_artifact_bytes << " bytes)\n";
  }

  // === 3. Deterministic build ===
  {
    TestFixture fix;
    const auto seed = parse_seed(voltfox_seed_text());
    build_package(seed, fix.sub("pkg1"));
    build_package(seed, fix.sub("pkg2"));
    const auto v1 = verify_package(fix.sub("pkg1"));
    const auto v2 = verify_package(fix.sub("pkg2"));
    check(v1.package_identity == v2.package_identity, "deterministic build failed");
    std::cout << "  [PASS] Deterministic build (identity: " << v1.package_identity.substr(0, 16) << "...)\n";
  }

  // === 4. Synthesis pipeline ===
  {
    const auto base_palette = make_palette("#242038", "#56F1FF");
    check(base_palette.primary != 0, "base palette primary is zero");
    check(base_palette.accent != 0, "base palette accent is zero");

    SynthesisPalette storm_palette;
    storm_palette.primary = 0x40A0FFFF;
    storm_palette.accent = 0xFFFF44FF;
    storm_palette.secondary = 0x205080FF;
    storm_palette.outline = 0x202020FF;
    storm_palette.background = 0x00000000;

    const auto result = synthesize_unified_entity("original.voltfox", base_palette, storm_palette);
    check(!result.proj2d_base.id.empty(), "2D base projection id is empty");
    check(!result.proj2d_transformed.id.empty(), "2D storm projection id is empty");
    check(!result.proj25d_base.id.empty(), "2.5D base projection id is empty");
    check(!result.proj25d_transformed.id.empty(), "2.5D storm projection id is empty");
    check(!result.proj3d_base.id.empty(), "3D base projection id is empty");
    check(!result.proj3d_transformed.id.empty(), "3D storm projection id is empty");
    check(!result.manifest2d.id.empty(), "2D manifestation id is empty");
    check(!result.manifest25d.id.empty(), "2.5D manifestation id is empty");
    check(!result.manifest3d.id.empty(), "3D manifestation id is empty");

    auto v2d = validate_projection2d(result.proj2d_base);
    check(v2d.ok(), "2D base projection validation failed");
    v2d = validate_projection2d(result.proj2d_transformed);
    check(v2d.ok(), "2D storm projection validation failed");

    auto v25d = validate_projection25d(result.proj25d_base);
    check(v25d.ok(), "2.5D base projection validation failed");
    v25d = validate_projection25d(result.proj25d_transformed);
    check(v25d.ok(), "2.5D storm projection validation failed");

    auto v3d = validate_projection3d(result.proj3d_base);
    check(v3d.ok(), "3D base projection validation failed");
    v3d = validate_projection3d(result.proj3d_transformed);
    check(v3d.ok(), "3D storm projection validation failed");

    std::cout << "  [PASS] Synthesis pipeline (all 6 projections validated)\n";
  }

  // === 5. Manifestation validation ===
  {
    CombatProgram combat{"voltfox.combat", 4, 4,
      {{"bite", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 5, 0}}},
       {"storm", CombatTargetRule::enemy, 10, 4, 5000, {{CombatEffectKind::damage, {}, 30, 0}, {CombatEffectKind::status, "shocked", 2, 3}}}}};
    TransformationProgram forms{"voltfox.forms", "base", 8,
      {{"base", 0, 0, {"bite"}}, {"storm", 40, 20, {"bite", "storm"}}},
      {{"ascend", "base", "storm", 20, 4, true}, {"descend", "storm", "base", 0, 2, true}}};
    std::vector<SkeletalClip> clips{{"idle", 10, true, {}, {}}, {"storm.idle", 10, true, {}, {}}, {"ascend", 4, false, {}, {}}, {"descend", 2, false, {}, {}}};
    AnimationStateGraph graph{"base", {{"base", "idle", {{"storm", "form", Comparison::equal, 1, 0, 0, 1}}}, {"storm", "storm.idle", {{"base", "form", Comparison::equal, 0, 0, 0, 1}}}}};
    std::vector<Projection3dDefinition> proj3ds;
    for (const auto &form : forms.forms) {
      Projection3dDefinition p;
      p.id = "original.voltfox." + form.id + ".3d";
      p.materials = {{"body", 0x804020FF, 0, 900000, MaterialAlphaMode::opaque, 500000, false, {}, {}, {}}};
      p.meshes = {{{"body", MeshPurpose::render, "body", false,
        {{{0, 0, 0}, {0, 0, 1000000}, {0, 0}, {}},
         {{1000, 0, 0}, {0, 0, 1000000}, {0, 0}, {}},
         {{0, 1000, 0}, {0, 0, 1000000}, {0, 0}, {}}},
        {0, 2, 1}}}};
      proj3ds.push_back(std::move(p));
    }

    TransformationManifestationProgram manifest{
      "original.voltfox.manifest.3d",
      {{"base", "base", "original.voltfox.base.3d"}, {"storm", "storm", "original.voltfox.storm.3d"}},
      {{"ascend", "ascend"}, {"descend", "descend"}}
    };

    auto vm3d = validate_transformation_manifestation_program(manifest, forms, combat, graph, clips, proj3ds);
    check(vm3d.ok(), "3D manifestation validation failed");
    std::cout << "  [PASS] Manifestation validation\n";
  }

  // === 6. Build with authored visual set ===
  {
    TestFixture fix;

    auto body = make_frame(8, 12, 36, 32, 56);
    auto head = make_frame(8, 12, 86, 241, 255);
    auto tail = make_frame(8, 12, 36, 32, 56);

    auto body_depth = make_frame(8, 12, 128, 128, 128);
    body_depth.color_space = ColorSpace::data;

    write_binary(fix.sub("body.png"), encode_png(body));
    write_binary(fix.sub("head.png"), encode_png(head));
    write_binary(fix.sub("tail.png"), encode_png(tail));
    write_binary(fix.sub("body_depth.png"), encode_png(body_depth));

    auto manifest_text =
      "schema=gspl.visual-set/0.1\n"
      "rights=ORIGINAL_USER_CREATION\n"
      "max_width=32\n"
      "max_height=16\n"
      "padding=1\n"
      "trim=true\n"
      "alpha_threshold=0\n"
      "pixel_art=true\n"
      "grid_width=1\n"
      "grid_height=1\n"
      "maximum_colors=4\n"
      "binary_alpha=true\n"
      "temporal_max_changed_per_million=1000000\n"
      "temporal_min_silhouette_iou_per_million=0\n"
      "layer=body\n"
      "layer=head\n"
      "layer=tail\n"
      "frame=idle|south|body|0|body.png|4|6|10\n"
      "frame=idle|south|head|0|head.png|4|6|10\n"
      "frame=idle|south|tail|0|tail.png|4|6|10\n"
      "map=depth|idle|south|body|0|body_depth.png\n";

    write_text(fix.sub("visual.txt"), manifest_text);

    const auto loaded = load_authored_visual_set(fix.sub("visual.txt"));
    check(loaded.schema == "gspl.visual-set/0.1", "visual set schema mismatch");
    check(loaded.frames.size() == 3, "visual set frame count mismatch");
    check(loaded.channel_maps.size() == 1, "visual set channel map count mismatch");
    check(loaded.canonical_metadata.find("gspl.visual-projection/0.1") != std::string::npos,
          "visual set missing canonical visual projection metadata");

    const auto seed = parse_seed(voltfox_seed_text());
    const auto output = fix.sub("visual-package");
    build_package(seed, loaded, output);
    const auto verification = verify_package(output);
    check(verification.ok(), "visual-set package verification failed");
    check(verification.artifact_count > 9, "visual-set package should have more than 9 artifacts");
    std::cout << "  [PASS] Visual-set package build + verify (" << verification.artifact_count
              << " artifacts, " << verification.total_artifact_bytes << " bytes)\n";
  }

  // === 7. Seed-to-synthesis connection test ===
  {
    const auto seed = parse_seed(voltfox_seed_text());
    const auto ir = compile(seed);
    const auto result = synthesize_unified_entity(ir);

    check(result.proj2d_base.id.find("original.voltfox") == 0, "2D base id does not start with entity_id");
    check(result.proj2d_transformed.id.find("original.voltfox") == 0, "2D storm id does not start with entity_id");
    check(result.proj25d_base.id.find("original.voltfox") == 0, "2.5D base id does not start with entity_id");
    check(result.proj25d_transformed.id.find("original.voltfox") == 0, "2.5D storm id does not start with entity_id");
    check(result.proj3d_base.id.find("original.voltfox") == 0, "3D base id does not start with entity_id");
    check(result.proj3d_transformed.id.find("original.voltfox") == 0, "3D storm id does not start with entity_id");
    check(result.manifest2d.id.find("original.voltfox") == 0, "2D manifest id does not start with entity_id");
    check(result.manifest25d.id.find("original.voltfox") == 0, "2.5D manifest id does not start with entity_id");
    check(result.manifest3d.id.find("original.voltfox") == 0, "3D manifest id does not start with entity_id");

    check(result.proj2d_base.rig.id == "voltfox.rig", "2D rig id mismatch");
    check(result.proj2d_base.rig.bones.size() == 3, "expected 3 bones from voltfox seed");
    check(result.proj2d_base.rig.bones[0].id == "root", "bone 0 id");
    check(result.proj2d_base.rig.bones[1].id == "head", "bone 1 id");
    check(result.proj2d_base.rig.bones[2].id == "tail", "bone 2 id");
    check(result.proj2d_base.rig.sockets.size() == 1, "expected 1 socket");
    check(result.proj2d_base.rig.sockets[0].id == "muzzle", "socket id");

    check(result.proj2d_base.collision_shapes.size() == 2, "expected 2 collision shapes from seed");
    check(result.proj2d_base.collision_shapes[0].id == "body", "collision shape 0 id");
    check(result.proj2d_base.collision_shapes[1].id == "bolt", "collision shape 1 id");
    check(result.proj2d_base.collision_windows.size() == 1, "expected 1 collision window from seed");
    check(result.proj2d_base.collision_windows[0].ability_id == "directional-lightning", "collision window ability");

    check(result.proj2d_transformed.rig.bones.size() == 3, "transformed rig should also have 3 bones");
    check(result.proj2d_transformed.collision_shapes.size() == 2, "transformed collision shapes from seed");
    check(result.proj2d_transformed.collision_windows.size() == 1, "transformed collision windows from seed");

    auto v2d = validate_projection2d(result.proj2d_base);
    check(v2d.ok(), "seed-driven 2D base projection validation failed");
    v2d = validate_projection2d(result.proj2d_transformed);
    check(v2d.ok(), "seed-driven 2D storm projection validation failed");
    auto v25d = validate_projection25d(result.proj25d_base);
    check(v25d.ok(), "seed-driven 2.5D base validation failed");
    v25d = validate_projection25d(result.proj25d_transformed);
    check(v25d.ok(), "seed-driven 2.5D storm validation failed");
    auto v3d = validate_projection3d(result.proj3d_base);
    check(v3d.ok(), "seed-driven 3D base validation failed");
    v3d = validate_projection3d(result.proj3d_transformed);
    check(v3d.ok(), "seed-driven 3D storm validation failed");

    std::cout << "  [PASS] Seed-to-synthesis connection (entity_id, rig, collisions propagated)\n";
  }

  // === 8. Cross-representation consistency + frame hash + resource limits ===
  {
    const auto seed = parse_seed(voltfox_seed_text());
    const auto ir = compile(seed);
    const auto result_1 = synthesize_unified_entity(ir);

    // 8a. All 2D frame hashes are non-empty, 64-hex, lowercase
    auto check_frames = [](std::string_view label, const std::vector<FrameSource>& frames) {
      for (const auto& f : frames) {
        if (f.frame_hash.empty())
          throw std::runtime_error(std::string(label) + " frame " + f.id + " has empty frame_hash");
        if (f.frame_hash.size() != 64)
          throw std::runtime_error(std::string(label) + " frame " + f.id + " hash is not 64 hex chars");
        for (auto c : f.frame_hash)
          if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            throw std::runtime_error(std::string(label) + " frame " + f.id + " hash is not lowercase hex");
      }
    };
    check_frames("proj2d_base", result_1.proj2d_base.source_frames);
    check_frames("proj2d_transformed", result_1.proj2d_transformed.source_frames);
    std::size_t total_2d_frames = result_1.proj2d_base.source_frames.size()
                                + result_1.proj2d_transformed.source_frames.size();
    check(total_2d_frames > 0, "expected at least 1 2D frame");

    // 8b. Frame hashes are deterministic (repeat synthesis produces identical hashes)
    const auto result_2 = synthesize_unified_entity(ir);
    check(result_1.proj2d_base.source_frames.size() == result_2.proj2d_base.source_frames.size(),
          "determinism: 2D base frame count mismatch");
    for (std::size_t i = 0; i < result_1.proj2d_base.source_frames.size(); ++i)
      check(result_1.proj2d_base.source_frames[i].frame_hash == result_2.proj2d_base.source_frames[i].frame_hash,
            "determinism: 2D base frame hash mismatch");
    for (std::size_t i = 0; i < result_1.proj2d_transformed.source_frames.size(); ++i)
      check(result_1.proj2d_transformed.source_frames[i].frame_hash == result_2.proj2d_transformed.source_frames[i].frame_hash,
            "determinism: 2D transformed frame hash mismatch");

    // 8c. Structural consistency across 2D base and transformed
    check(result_1.proj2d_base.rig.bones.size() == result_1.proj2d_transformed.rig.bones.size(),
          "rig bone count mismatch between base and transformed");
    check(result_1.proj2d_base.collision_shapes.size() == result_1.proj2d_transformed.collision_shapes.size(),
          "collision shape count mismatch between base and transformed");
    check(result_1.proj2d_base.collision_windows.size() == result_1.proj2d_transformed.collision_windows.size(),
          "collision window count mismatch between base and transformed");
    for (std::size_t i = 0; i < result_1.proj2d_base.collision_shapes.size(); ++i)
      check(result_1.proj2d_base.collision_shapes[i].id == result_1.proj2d_transformed.collision_shapes[i].id,
            "collision shape id mismatch");

    // 8d. Synthesis resource limits pass (three-arg enforce_resource_limits)
    {
      ResourceLimits rl;
      auto vr = enforce_resource_limits(seed, result_1, rl);
      check(vr.ok(), "synthesis resource limits must pass for voltfox seed");
    }

    // 8e. Synthesis-level limits do not throw during pipeline execution
    // (synthesize_unified_entity already enforces limits internally)
    check(result_1.proj2d_base.source_frames.size() <= ResourceLimits{}.max_frames,
          "2D base frame count must not exceed limit");
    for (const auto& f : result_1.proj2d_base.source_frames) {
      check(f.image.width <= ResourceLimits{}.max_frame_width, "frame width must not exceed limit");
      check(f.image.height <= ResourceLimits{}.max_frame_height, "frame height must not exceed limit");
    }
    std::size_t vertex_count = 0;
    for (const auto& m : result_1.proj3d_base.meshes) vertex_count += m.vertices.size();
    check(vertex_count <= ResourceLimits{}.max_vertices, "vertex count must not exceed limit");

    std::cout << "  [PASS] Cross-representation consistency: "
              << total_2d_frames << " frames, "
              << result_1.proj2d_base.source_frames[0].frame_hash.substr(0, 8) << "... "
              << "hashes deterministic, limits OK\n";
  }

  // === 9. Negative — parse-time rejections ===
  {
    check_throws("invalid rights",
      [] { auto r = parse_seed(
        "schema=gspl.sprite-seed/0.1\nid=test\nname=Test\n"
        "rights=INVALID_RIGHTS_CLASS\nprimary_color=#242038\n"
        "accent_color=#56F1FF\n"); (void)r; });

    check_throws("bad numeric", [] { auto r = parse_seed("schema=gspl.sprite-seed/0.1\nentropy_root=notanumber\n"); (void)r; });

    check_throws("oversized source",
      [] { auto r = parse_seed(std::string(2'000'000, 'x')); (void)r; });

    check_throws("duplicate non-repeatable field",
      [] { auto r = parse_seed(
        "schema=gspl.sprite-seed/0.1\nid=dup\ntest\nname=Duplicate\n"
        "id=dup2\n"); (void)r; });

    std::cout << "  [PASS] Negative — parse-time rejections\n";
  }

  // === 10. Negative — validation failures ===
  {
    auto reject = [](std::string_view label, std::string_view text) {
      const auto s = parse_seed(text);
      const auto v = validate(s);
      check(!v.ok(), std::string(label).c_str());
    };

    reject("missing schema",
      "id=test\nname=Test\nclassification=test.malformed\n"
      "rights=ORIGINAL_USER_CREATION\nprimary_color=#242038\naccent_color=#56F1FF\n");

    reject("missing id",
      "schema=gspl.sprite-seed/0.1\nname=Test\nclassification=test.malformed\n"
      "rights=ORIGINAL_USER_CREATION\nprimary_color=#242038\naccent_color=#56F1FF\n");

    reject("empty classification",
      "schema=gspl.sprite-seed/0.1\nid=test\nname=Test\nclassification=\n"
      "rights=ORIGINAL_USER_CREATION\nprimary_color=#242038\naccent_color=#56F1FF\n");

    reject("prohibited rights",
      "schema=gspl.sprite-seed/0.1\nid=test\nname=Test\nclassification=test.malformed\n"
      "rights=PROHIBITED\nprimary_color=#242038\naccent_color=#56F1FF\n");

    std::cout << "  [PASS] Negative — validation failures\n";
  }

  // === 11. Morphology-driven 3D Voltfox synthesis ===
  {
    const std::string morph_seed_text = voltfox_seed_text() + R"(
[form.Idle]
transformations=IdleToAttack

[form.Attack]
transformations=

[transformation.IdleToAttack]
from_form=Idle
to_form=Attack
trigger=combat:threat_detected

[morphology.torso]
position=0,0,0
size=40,30,20
color=#242038
rotation=0

[morphology.head]
position=0,25,5
size=24,20,18
color=#242038
rotation=0

[morphology.left_ear]
position=-8,33,10
size=8,16,8
color=#56F1FF
rotation=-10

[morphology.right_ear]
position=8,33,10
size=8,16,8
color=#56F1FF
rotation=10

[morphology.left_eye]
position=-5,27,15
size=4,4,4
color=#56F1FF
rotation=0

[morphology.right_eye]
position=5,27,15
size=4,4,4
color=#56F1FF
rotation=0

[morphology.muzzle]
position=0,22,18
size=10,8,6
color=#FFFFFF
rotation=0

[morphology.tail]
position=-20,-5,0
size=6,30,6
color=#242038
rotation=-15

[morphology.left_front_leg]
position=-10,-20,-5
size=8,20,8
color=#1a1828
rotation=0

[morphology.right_front_leg]
position=10,-20,-5
size=8,20,8
color=#1a1828
rotation=0

[morphology.aura]
position=0,0,0
size=60,50,40
color=#56F1FF
rotation=0

[runtime]
)";
    const auto seed = parse_seed(morph_seed_text);
    check(validate(seed).ok(), "morphology seed validation failed");
    const auto ir = compile(seed);
    check(!ir.morphology.empty(), "compiled morphology is empty");
    check(ir.morphology.size() >= 11, "expected 11+ morphology parts");

    const auto result = synthesize_unified_entity(ir);
    auto val3d = validate_projection3d(result.proj3d_base);
    if (!val3d.ok()) {
      for (const auto& d : val3d.diagnostics) {
        std::cerr << "  3D base diagnostic: " << d.code << " - " << d.message << '\n';
      }
    }
    check(val3d.ok(), "morphology-driven 3D base projection validation failed");
    val3d = validate_projection3d(result.proj3d_transformed);
    if (!val3d.ok()) {
      for (const auto& d : val3d.diagnostics) {
        std::cerr << "  3D storm diagnostic: " << d.code << " - " << d.message << '\n';
      }
    }
    check(val3d.ok(), "morphology-driven 3D storm projection validation failed");

    check(result.proj3d_base.skeleton.has_value(), "3D base projection missing skeleton");
    check(result.proj3d_base.skeleton->joints.size() == 13, "expected 13 bones in voltfox skeleton");
    check(result.proj3d_base.meshes.size() >= 8, "expected 8+ meshes from morphology parts");
    check(result.proj3d_transformed.skeleton.has_value(), "3D storm projection missing skeleton");
    check(result.proj3d_transformed.skeleton->joints.size() == 13, "expected 13 bones in storm skeleton");

    std::uint64_t total_verts = 0, total_tris = 0;
    for (const auto& mesh : result.proj3d_base.meshes) {
      total_verts += mesh.vertices.size();
      total_tris += mesh.triangle_indices.size() / 3;
    }
    check(total_verts > 100, "expected >100 total vertices in voltfox 3D mesh");
    check(total_tris > 100, "expected >100 total triangles in voltfox 3D mesh");

    std::cout << "  [PASS] Morphology-driven 3D synthesis (" << result.proj3d_base.meshes.size()
              << " meshes, " << total_verts << " verts, " << total_tris << " tris, "
              << result.proj3d_base.skeleton->joints.size() << " bones)\n";
  }

  // === 12. Integrity — package corruption ===
  {
    TestFixture fix;
    const auto seed = parse_seed(voltfox_seed_text());
    const auto output = fix.sub("integrity-pkg");
    build_package(seed, output);

    auto vr_clean = verify_package(output);
    check(vr_clean.ok(), "clean package verification failed");

    auto corrupt = output / "seed.canonical.json";
    check(std::filesystem::exists(corrupt), "seed.canonical.json must exist");
    std::ofstream bad(corrupt, std::ios::binary | std::ios::trunc);
    bad << "CORRUPTED_INTEGRITY_TEST_DATA";
    bad.close();

    auto vr_bad = verify_package(output);
    check(!vr_bad.ok(), "corrupted package must fail verification");
    std::cout << "  [PASS] Integrity — corrupted package rejected\n";
  }

  // === 13. Integrity — identity mismatch ===
  {
    TestFixture fix;
    const auto seed_a = parse_seed(voltfox_seed_text());
    const auto seed_b = parse_seed(
      "schema=gspl.sprite-seed/0.1\nid=original.test\n"
      "name=Test\nclassification=test.synthetic\n"
      "rights=ORIGINAL_USER_CREATION\n"
      "primary_color=#FF0000\naccent_color=#00FF00\n"
      "ability=dummy|test.ability|1|1|1\n");
    build_package(seed_a, fix.sub("pkg_a"));
    build_package(seed_b, fix.sub("pkg_b"));
    const auto va = verify_package(fix.sub("pkg_a"));
    const auto vb = verify_package(fix.sub("pkg_b"));
    check(va.ok(), "package a verification");
    check(vb.ok(), "package b verification");
    check(va.entity_id != vb.entity_id, "different seeds must produce different entity_id");
    check(va.package_identity != vb.package_identity, "different seeds must produce different package identity");

    std::cout << "  [PASS] Integrity — identity mismatch between different seeds\n";
  }

  // === 14. 3D animation synthesis and GLB export ===
  {
    const std::string morph_anim_seed = voltfox_seed_text() + R"(
[form.Idle]
transformations=IdleToAttack

[form.Attack]
transformations=

[transformation.IdleToAttack]
from_form=Idle
to_form=Attack
trigger=combat:threat_detected

[morphology.torso]
position=0,0,0
size=40,30,20
color=#242038
rotation=0

[morphology.head]
position=0,25,5
size=24,20,18
color=#242038
rotation=0

[morphology.left_ear]
position=-8,33,10
size=8,16,8
color=#56F1FF
rotation=-10

[morphology.right_ear]
position=8,33,10
size=8,16,8
color=#56F1FF
rotation=10

[morphology.left_eye]
position=-5,27,15
size=4,4,4
color=#56F1FF
rotation=0

[morphology.right_eye]
position=5,27,15
size=4,4,4
color=#56F1FF
rotation=0

[morphology.muzzle]
position=0,22,18
size=10,8,6
color=#FFFFFF
rotation=0

[morphology.tail]
position=-20,-5,0
size=6,30,6
color=#242038
rotation=-15

[morphology.left_front_leg]
position=-10,-20,-5
size=8,20,8
color=#1a1828
rotation=0

[morphology.right_front_leg]
position=10,-20,-5
size=8,20,8
color=#1a1828
rotation=0

[morphology.aura]
position=0,0,0
size=60,50,40
color=#56F1FF
rotation=0

[runtime]
)";
    const auto seed = parse_seed(morph_anim_seed);
    check(validate(seed).ok(), "morphology animation seed validation failed");
    const auto ir = compile(seed);
    check(!ir.morphology.empty(), "compiled morphology is empty");

    const auto result = synthesize_unified_entity(ir);
    check(!result.animations3d.empty(), "expected 3D animation clips but got none");

    // Verify animation clips are well-formed
    for (const auto& clip : result.animations3d) {
      auto val = validate_animation_clip3d(clip, result.proj3d_base);
      if (!val.ok()) {
        for (const auto& d : val.diagnostics)
          std::cerr << "  clip validation: " << d.code << " - " << d.message << '\n';
      }
      check(val.ok(), "3D animation clip validation failed");
    }

    // Export to GLB with animations
    const auto glb = export_projection3d_glb(
        result.proj3d_base,
        std::span<const AnimationClip3d>(result.animations3d),
        std::span<const GltfTextureAsset>{}, {});
    check(glb.size() > 100, "GLB animation export produced empty output");
    const auto verification = verify_projection3d_glb(glb);
    check(verification.ok(), "GLB with animations failed verification");

    std::cout << "  [PASS] 3D animation synthesis + GLB export ("
              << result.animations3d.size() << " clips, "
              << glb.size() << " bytes)\n";
  }

  std::cout << "all GSPL Sprites voltfox integration tests passed\n";
  return 0;

} catch (const std::exception &error) {
  std::cerr << "GSPL_SPRITES_VOLTFOX_INTEGRATION_FAILED: " << error.what() << '\n';
  return 1;
}
