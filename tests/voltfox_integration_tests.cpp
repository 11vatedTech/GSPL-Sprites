#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/transformation.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"
#include "gspl_sprites/visual_set.hpp"

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

static bool has_artifact(const PackageVerification &pv, std::string_view path) {
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
    std::vector<SkeletalClip> clips{{"idle", 10, true}, {"storm.idle", 10, true}, {"ascend", 4, false}, {"descend", 2, false}};
    AnimationStateGraph graph{"base", {{"base", "idle", {{"storm", "form", Comparison::equal, 1, 0, 0, 1}}}, {"storm", "storm.idle", {{"base", "form", Comparison::equal, 0, 0, 0, 1}}}}};
    std::vector<Projection3dDefinition> proj3ds;
    for (const auto &form : forms.forms) {
      Projection3dDefinition p;
      p.id = "original.voltfox." + form.id + ".3d";
      p.materials = {{"body", 0x804020FF, 0, 900000}};
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

  // === 8. Negative — parse-time rejections ===
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

  // === 9. Negative — validation failures ===
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

  // === 10. Integrity — package corruption ===
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

  // === 11. Integrity — identity mismatch ===
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

  std::cout << "all GSPL Sprites voltfox integration tests passed\n";
  return 0;

} catch (const std::exception &error) {
  std::cerr << "GSPL_SPRITES_VOLTFOX_INTEGRATION_FAILED: " << error.what() << '\n';
  return 1;
}
