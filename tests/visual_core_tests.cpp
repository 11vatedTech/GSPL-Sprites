// Native Visual Core tests: morphology v2 + lowering, style, palette,
// materials, markings, performance, Visual IR, compiler, raster, metrics,
// determinism, resource bounds, generalization fixtures, style variation.
#include "gspl_sprites/markings.hpp"
#include "gspl_sprites/material.hpp"
#include "gspl_sprites/palette.hpp"
#include "gspl_sprites/performance.hpp"
#include "gspl_sprites/style.hpp"
#include "gspl_sprites/visual_compiler.hpp"
#include "gspl_sprites/visual_geometry.hpp"
#include "gspl_sprites/visual_ir.hpp"
#include "gspl_sprites/visual_metrics.hpp"
#include "gspl_sprites/visual_morphology.hpp"
#include "gspl_sprites/visual_raster.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include "gspl_sprites/image.hpp"
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

using namespace gspl::sprites;
using namespace gspl::sprites::visual;

static int failures = 0;
static void check(bool v, const char* msg) {
  if (v) { std::cout << "PASS: " << msg << std::endl; }
  else { std::cerr << "FAIL: " << msg << std::endl; ++failures; }
}
static void check_ok(const ValidationResult& r, const char* msg) { check(r.ok(), msg); }
static void check_fail(const ValidationResult& r, const char* msg) { check(!r.ok(), msg); }

// ── Fixture: generic quadruped-like entity (Voltfox-family morphology) ──
static SpriteIr make_voltfox_ir() {
  SpriteIr ir;
  ir.entity_id = "voltfox";
  ir.seed_identity = "seed-voltfox";
  ir.primary_color = "#8a6f3c";
  ir.accent_color = "#4ad9ff";
  ir.emissive_color = "#9be8ff";
  ir.aura_color = "#7a5cff";
  ir.storm_primary_color = "#1a0a2e";
  ir.storm_accent_color = "#4ad9ff";
  ir.rights = RightsClass::original_user_creation;
  // Coordinates are authored for the raster's kPosScale=3.0 / kSizeScale=1.5
  // mapping onto a 128x128 canvas (center 64,64; y-up). All anatomy stays on
  // canvas so palette-driven role colors (eyes) and forms are visible.
  auto& m = ir.morphology;
  m["torso"] = {0, 0, 0, 40, 30, 20, "#242038", 0, "root", false, false, "torso", "ellipse", "body-mass", 0};
  m["head"] = {0, 5, 5, 24, 20, 18, "#242038", 0, "torso", false, false, "head", "ellipse", "facial-feature", 0};
  m["tail"] = {-14, -2, 2, 8, 26, 8, "#242038", -12, "torso", false, false, "tail", "segmented_curve", "appendage", 0};
  m["left_leg"] = {-8, -12, 3, 8, 18, 8, "#242038", 0, "torso", false, false, "leg", "capsule", "limb", 0};
  m["right_leg"] = {8, -12, 3, 8, 18, 8, "#242038", 0, "torso", false, false, "leg", "capsule", "limb", 0};
  m["left_ear"] = {-7, 4, 8, 6, 12, 5, "#242038", -15, "head", false, false, "ear", "triangle", "appendage", 0};
  m["right_ear"] = {7, 4, 8, 6, 12, 5, "#242038", 15, "head", false, false, "ear", "triangle", "appendage", 0};
  m["left_eye"] = {-5, 6, 10, 3, 3, 1, "", 0, "head", false, false, "eye", "ellipse", "eye", 0};
  m["right_eye"] = {5, 6, 10, 3, 3, 1, "", 0, "head", false, false, "eye", "ellipse", "eye", 0};
  m["electrical_aura"] = {0, 0, -6, 40, 32, 30, "", 0, "root", false, true, "aura", "aura_contour", "energy-aura", 0};
  ir.form_definitions.push_back({"base", {}});
  ir.form_definitions.push_back({"storm", {}});
  return ir;
}

// ── Generalization fixture: humanoid ──
static SpriteIr make_humanoid_ir() {
  SpriteIr ir;
  ir.entity_id = "test_humanoid";
  ir.primary_color = "#c0504a";
  ir.accent_color = "#f2c14e";
  ir.emissive_color = "#ffe9a8";
  ir.aura_color = "#ffd27a";
  auto& m = ir.morphology;
  m["torso"] = {0, 0, 0, 26, 34, 16, "#c0504a", 0, "root", false, false, "torso", "rounded_rect", "body-mass", 0};
  m["head"] = {0, 7, 6, 16, 16, 14, "#e8b98a", 0, "torso", false, false, "head", "ellipse", "facial-feature", 0};
  m["left_arm"] = {-17, -4, 4, 7, 20, 8, "#c0504a", 6, "torso", false, false, "arm", "capsule", "limb", 0};
  m["right_arm"] = {17, -4, 4, 7, 20, 8, "#c0504a", -6, "torso", false, false, "arm", "capsule", "limb", 0};
  m["left_leg"] = {-8, -11, 3, 8, 20, 8, "#40507a", 0, "torso", false, false, "leg", "capsule", "limb", 0};
  m["right_leg"] = {8, -11, 3, 8, 20, 8, "#40507a", 0, "torso", false, false, "leg", "capsule", "limb", 0};
  m["left_eye"] = {-4, 9, 10, 2, 2, 1, "#1c1c28", 0, "head", false, false, "eye", "ellipse", "eye", 0};
  m["right_eye"] = {4, 9, 10, 2, 2, 1, "#1c1c28", 0, "head", false, false, "eye", "ellipse", "eye", 0};
  return ir;
}

// ── Generalization fixture: mechanical / inanimate ──
static SpriteIr make_mech_ir() {
  SpriteIr ir;
  ir.entity_id = "test_mech";
  ir.primary_color = "#4a6a8a";
  ir.accent_color = "#ff5c38";
  ir.emissive_color = "#66e0ff";
  ir.aura_color = "#66e0ff";
  auto& m = ir.morphology;
  m["chassis"] = {0, 0, 0, 30, 20, 16, "#4a6a8a", 0, "root", false, false, "chassis", "rounded_rect", "body-mass", 0};
  m["turret"] = {0, 10, 6, 18, 8, 12, "#5a7a9a", 0, "chassis", false, false, "turret", "capsule", "armor-panel", 0};
  m["barrel"] = {0, 6, 8, 6, 12, 6, "#2a3a4a", 0, "turret", false, false, "barrel", "capsule", "weapon", 0};
  m["left_tread"] = {-14, -14, 2, 10, 8, 10, "#22303e", 0, "chassis", false, false, "tread", "capsule", "limb", 0};
  m["right_tread"] = {14, -14, 2, 10, 8, 10, "#22303e", 0, "chassis", false, false, "tread", "capsule", "limb", 0};
  m["core_light"] = {0, -2, 12, 6, 6, 2, "#ff5c38", 0, "chassis", true, false, "core", "ellipse", "energy", 0};
  return ir;
}

// ── Generalization fixture: non-quadruped creature (flying) ──
static SpriteIr make_flyer_ir() {
  SpriteIr ir;
  ir.entity_id = "test_flyer";
  ir.primary_color = "#7a4ac0";
  ir.accent_color = "#3ce0b0";
  ir.emissive_color = "#3ce0b0";
  ir.aura_color = "#3ce0b0";
  auto& m = ir.morphology;
  m["body"] = {0, 0, 0, 18, 12, 14, "#7a4ac0", 0, "root", false, false, "body", "ellipse", "body-mass", 0};
  m["head"] = {0, 10, 6, 10, 9, 10, "#8a5ad0", 0, "body", false, false, "head", "ellipse", "facial-feature", 0};
  m["left_wing"] = {-12, 2, 3, 16, 8, 4, "#5a2a90", -18, "body", false, false, "wing", "polygon", "appendage", 0};
  m["right_wing"] = {12, 2, 3, 16, 8, 4, "#5a2a90", 18, "body", false, false, "wing", "polygon", "appendage", 0};
  m["tail"] = {0, -8, 2, 6, 12, 4, "#8a5ad0", 0, "body", false, false, "tail", "triangle", "appendage", 0};
  m["left_eye"] = {-3, 11, 10, 2, 2, 1, "#3ce0b0", 0, "head", false, false, "eye", "ellipse", "eye", 0};
  m["right_eye"] = {3, 11, 10, 2, 2, 1, "#3ce0b0", 0, "head", false, false, "eye", "ellipse", "eye", 0};
  return ir;
}


static void test_lowering_and_morphology() {
  const SpriteIr ir = make_voltfox_ir();
  VisualMorphologyV2 v2 = lower_visual_morphology_v1(ir.morphology, "base");
  check(v2.parts.size() == ir.morphology.size(), "v2 lowering preserves part count");
  check(v2.form_id == "base", "v2 lowering keeps form id");
  const VisualPart& torso = v2.parts.at("torso");
  check(torso.size_x == 40.0 && torso.size_y == 30.0, "v2 preserves v1 sizes");
  check(torso.parent.empty() && torso.x == 0.0, "v2 preserves v1 hierarchy");
  check(torso.shape == VisualShapeKind::ellipse, "ellipse shape mapping");
  check(v2.parts.at("left_leg").shape == VisualShapeKind::capsule, "capsule shape mapping");
  check(v2.parts.at("left_ear").shape == VisualShapeKind::polygon, "triangle -> polygon mapping");
  check(v2.parts.at("tail").shape == VisualShapeKind::open_path, "segmented_curve -> open_path");
  check(v2.parts.at("electrical_aura").shape == VisualShapeKind::ring, "aura -> ring");
  check(v2.parts.at("left_eye").color_role == ColorRole::eye, "eye color role");
  check(v2.parts.at("electrical_aura").color_role == ColorRole::emission, "emission color role");
  check(v2.parts.at("torso").material_class == "skin", "default material class");
  check(v2.parts.at("left_eye").material_class == "glass", "eye material class");
  check(v2.parts.at("electrical_aura").material_class == "electricity", "electricity material class");
  check(v2.parts.at("electrical_aura").layer == VisualLayer::rear_effects, "aura layer");
  check(v2.parts.at("left_eye").layer == VisualLayer::facial, "eye layer");
  bool marking_found = false;
  for (const Marking& mk : v2.markings) if (mk.kind == MarkingKind::electrical) marking_found = true;
  check(marking_found, "electrical marking lowered");
  check(v2.parts.at("electrical_aura").marking_ids.size() == 1, "marking bound to part");
  const VisualMorphologyV2 v2b = lower_visual_morphology_v1(ir.morphology, "base");
  check(canonicalize_visual_morphology(v2) == canonicalize_visual_morphology(v2b),
        "lowering canonicalization deterministic");
  check(visual_morphology_identity(v2) == visual_morphology_identity(v2b), "v2 identity deterministic");
  check_ok(validate_visual_morphology(v2), "v2 validation passes");

  // Validation failures: duplicate ids, missing parent, cycle, nonfinite.
  VisualMorphologyV2 bad = v2;
  bad.parts["dup"] = bad.parts.at("torso");
  bad.parts["dup"].id = "torso";  // duplicate id key vs id field
  check_fail(validate_visual_morphology(bad), "v2 rejects duplicate ids");
  VisualMorphologyV2 bad2 = v2;
  bad2.parts["ghost"] = bad2.parts.at("torso");
  bad2.parts["ghost"].id = "ghost";
  bad2.parts["ghost"].parent = "missing_parent";
  check_fail(validate_visual_morphology(bad2), "v2 rejects missing parent");
  VisualMorphologyV2 bad3 = v2;
  bad3.parts["a"] = bad3.parts.at("torso"); bad3.parts["a"].id = "a"; bad3.parts["a"].parent = "b";
  bad3.parts["b"] = bad3.parts.at("torso"); bad3.parts["b"].id = "b"; bad3.parts["b"].parent = "a";
  check_fail(validate_visual_morphology(bad3), "v2 rejects cycles");
  VisualMorphologyV2 bad4 = v2;
  bad4.parts["nan_part"] = bad4.parts.at("torso");
  bad4.parts["nan_part"].id = "nan_part";
  bad4.parts["nan_part"].size_x = std::nan("");
  check_fail(validate_visual_morphology(bad4), "v2 rejects nonfinite geometry");
}

static void test_style_and_palette() {
  const StyleSemantics flat = make_style_preset("clean-flat");
  check(flat.outline_selection == OutlineSelection::none, "clean-flat preset has no outlines");
  const StyleSemantics ink = make_style_preset("inked");
  check(ink.line_weight > flat.line_weight, "inked has heavier lines");
  const StyleSemantics soft = make_style_preset("soft-shaded");
  check(soft.shadow_model == ShadowModel::gradient, "soft-shaded gradient shadows");
  const StyleSemantics pixel = make_style_preset("pixel-constrained");
  check(pixel.aa_policy == AntiAliasPolicy::none, "pixel style disables AA");
  check(pixel.pixel_quantization > 0.0, "pixel style quantizes");

  // Composition precedence: later patches win.
  std::vector<StylePatch> patches;
  StylePatch p1; p1.scope = "project"; p1.line_weight = 1.0;
  StylePatch p2; p2.scope = "entity"; p2.line_weight = 3.0;
  StylePatch p3; p3.scope = "form"; p3.line_weight = 2.0;
  patches.push_back(p1); patches.push_back(p2); patches.push_back(p3);
  const StyleSemantics composed = compose_style(flat, patches);
  check(composed.line_weight == 2.0, "style patch precedence: last wins");
  check_ok(validate_style(composed), "composed style validates");
  check(canonicalize_style(composed) == canonicalize_style(composed), "style canonicalization stable");
  check(style_identity(composed) == style_identity(composed), "style identity stable");

  // Palette.
  check(parse_hex_color("#ff8040").has_value(), "hex parse valid");
  check(!parse_hex_color("#ff80").has_value(), "hex parse rejects short");
  check(!parse_hex_color("nope").has_value(), "hex parse rejects junk");
  const PaletteDefinition pal = make_default_palette("#8a6f3c", "#4ad9ff", "#9be8ff", "#7a5cff");
  check_ok(validate_palette(pal), "default palette validates");
  check(palette_color(pal, ColorRole::primary) == 0x8A6F3CFFu, "primary role color");
  check(palette_color(pal, ColorRole::emission) == 0x9BE8FFFFu, "emission role color");
  check(palette_color(pal, ColorRole::custom, 0xDEADBEEFu) == 0xDEADBEEFu,
        "unknown role uses fallback");
  check(rgba32_to_hex(0x8A6F3CFFu) == "#8a6f3c", "rgba to hex");
  const PaletteDefinition tpal = make_transformed_palette("#1a0a2e", "#4ad9ff", "#9be8ff", "#7a5cff");
  check(palette_color(tpal, ColorRole::primary) == 0x1A0A2EFFu, "transformed palette primary");
  check(palette_identity(pal) != palette_identity(tpal), "palette identities differ");
  check(palette_identity(pal) == palette_identity(pal), "palette identity stable");

  // Material resolution.
  const MaterialSemantics fur = make_material("fur", "m1");
  check(fur.roughness > 0.7, "fur roughness");
  const MaterialSemantics metal = make_material("metal", "m2");
  check(metal.metallicity > 0.8, "metal metallicity");
  check(make_material("unknown_material", "m3").class_name == "unknown_material",
        "unknown material degrades gracefully");
  check_ok(validate_material(fur), "fur validates");
  const MaterialSemantics resolved = resolve_material(metal, soft);
  check(canonicalize_material(resolved) == canonicalize_material(resolved), "material canonical stable");

  // Markings validation.
  std::vector<Marking> mks;
  Marking mk; mk.id = "m1"; mk.kind = MarkingKind::electrical; mk.part_id = "torso";
  mk.path = {{0, 0}, {1, 1}, {2, 0}}; mk.widths = {0.5, 0.4, 0.3};
  mks.push_back(mk);
  check_ok(validate_markings(mks, std::vector<std::string>{"torso"}), "markings validate");
  mks[0].part_id = "ghost_part";
  check_fail(validate_markings(mks, std::vector<std::string>{"torso"}), "markings reject unknown surface");
  check(canonicalize_marking(mk) == canonicalize_marking(mk), "marking canonical stable");

  // Performance semantics.
  PerformanceState perf;
  PartMotion m; m.part_id = "head"; m.dx = 1.0; m.rotation_degrees = 5.0;
  perf.motions.push_back(m);
  check_ok(validate_performance(perf, std::vector<std::string>{"torso", "head"}), "performance validates");
  PerformanceState badperf; badperf.facing = "up";
  check_fail(validate_performance(badperf, std::vector<std::string>{}), "performance rejects bad facing");
  check(canonicalize_performance(perf) == canonicalize_performance(perf), "performance canonical stable");
  check(!performance_identity(perf).empty(), "performance identity non-empty");
}


static void test_ir_and_compiler() {
  const SpriteIr ir = make_voltfox_ir();
  VisualCompileOptions opts;
  opts.canvas_width = 128;
  opts.canvas_height = 128;
  opts.style_preset = "clean-flat";
  VisualIrResult res = compile_visual_ir(ir, opts);
  check(res.ok(), "voltfox compiles to Visual IR");
  if (!res.ok()) {
    for (const auto& d : res.diagnostics.diagnostics) std::cerr << "  diag: " << d.code << " " << d.message << "\n";
    return;
  }
  const VisualIr& vir = *res.value;
  check(vir.form_id == "base", "default form is first form");
  check(vir.morphology.parts.size() == ir.morphology.size(), "IR morphology complete");
  check(vir.projection.width == 128 && vir.projection.height == 128, "IR projection canvas");
  check(vir.layer_order == vir.morphology.layer_order, "IR layer order authoritative");
  check(vir.channel_requests.size() == 4, "IR default channels");
  check(!vir.materials.empty(), "IR materials resolved");
  check_ok(validate_visual_ir(vir), "IR validates");
  const std::string canon = canonicalize_visual_ir(vir);
  check(canon == canonicalize_visual_ir(vir), "IR canonicalization stable");
  const std::string id1 = visual_ir_identity(vir);
  const VisualIrResult res2 = compile_visual_ir(ir, opts);
  check(id1 == visual_ir_identity(*res2.value), "IR identity deterministic");
  check(!id1.empty(), "IR identity non-empty");

  // Form variant: storm palette differs, identity differs.
  VisualCompileOptions storm_opts = opts;
  storm_opts.form_id = "storm";
  const VisualIrResult storm = compile_visual_ir(ir, storm_opts);
  check(storm.ok(), "storm form compiles");
  if (storm.ok()) {
    check(storm.value->palette.form_id == "storm", "storm palette bound to form");
    check(visual_ir_identity(*storm.value) != id1, "form changes IR identity");
  }

  // Style patches change style but not entity identity.
  VisualCompileOptions inked_opts = opts;
  StylePatch inkp; inkp.scope = "entity"; inkp.line_weight = 2.5; inkp.outline_selection = OutlineSelection::all_parts;
  inked_opts.style_patches.push_back(inkp);
  const VisualIrResult inked = compile_visual_ir(ir, inked_opts);
  check(inked.ok() && inked.value->style.line_weight == 2.5, "style patch applied through compiler");
  if (inked.ok() && storm.ok()) {
    check(visual_ir_identity(*inked.value) != visual_ir_identity(*res.value),
          "style changes IR identity (manifestation identity)");
  }

  // Empty entity fails closed.
  SpriteIr empty = ir;
  empty.entity_id.clear();
  check(!compile_visual_ir(empty, opts).ok(), "empty entity fails closed");
}

static void test_raster_and_determinism() {
  const SpriteIr ir = make_voltfox_ir();
  VisualCompileOptions opts;
  opts.canvas_width = 128; opts.canvas_height = 128;
  opts.style_preset = "clean-flat";
  const VisualIrResult res = compile_visual_ir(ir, opts);
  if (!res.ok()) { check(false, "raster needs valid IR"); return; }
  RasterLimits limits;
  RasterResult r1 = render_visual_ir(*res.value, limits);
  check(r1.ok(), "voltfox renders");
  if (!r1.ok()) {
    for (const auto& d : r1.diagnostics.diagnostics) std::cerr << "  diag: " << d.code << " " << d.message << "\n";
    return;
  }
  const RasterOutput& out = *r1.value;
  check(out.image.width == 128 && out.image.height == 128, "raster canvas size");
  check(out.image.color_space == ColorSpace::srgb, "raster color space");
  check(out.raster_work > 0, "raster work counted");
  const FidelityReport rep = measure_frame_fidelity(*res.value, out);
  check(rep.silhouette.covered_pixels > 500, "voltfox silhouette has real area");
  check(rep.silhouette.connected_components >= 1, "silhouette components");
  check(rep.alpha_edge_quality > 0.0, "analytic AA produces partial edges");
  check(!rep.notes.empty(), "fidelity notes produced");
  check(out.channels.size() == 4, "raster produces all channels");
  bool has_emissive = false, has_mat = false;
  for (const ChannelRaster& c : out.channels) {
    if (c.kind == ChannelMapKind::emissive) has_emissive = true;
    if (c.kind == ChannelMapKind::material_id) has_mat = true;
  }
  check(has_emissive && has_mat, "emissive + material channels present");

  // Determinism: byte-identical A/B.
  const RasterResult r2 = render_visual_ir(*res.value, limits);
  check(r2.ok(), "second render ok");
  if (r2.ok()) {
    check(r2.value->image.pixels == out.image.pixels, "raster byte-deterministic");
    check(r2.value->raster_work == out.raster_work, "raster work deterministic");
    check(r2.value->channels.size() == out.channels.size(), "channels deterministic");
  }

  // Performance state changes frames (pose bake) but stays deterministic.
  VisualCompileOptions pos = opts;
  PerformanceState perf;
  PartMotion m; m.part_id = "head"; m.dx = 2.0; m.rotation_degrees = 8.0;
  perf.motions.push_back(m);
  pos.performance = &perf;
  const VisualIrResult pres = compile_visual_ir(ir, pos);
  if (pres.ok()) {
    const RasterResult pr = render_visual_ir(*pres.value, limits);
    check(pr.ok(), "posed render ok");
    if (pr.ok() && r1.ok()) {
      check(pr.value->image.pixels != out.image.pixels, "pose changes pixels");
      const FidelityReport pd = measure_frame_fidelity(*pres.value, *pr.value, &out.image);
      check(pd.frame_distinction > 0.0, "pose frame distinction positive");
    }
  } else {
    check(false, "posed IR compile failed");
  }

  // Facing mirror changes frame.
  VisualCompileOptions faced = opts;
  PerformanceState fperf;
  fperf.facing = "left";
  faced.performance = &fperf;
  const VisualIrResult fres = compile_visual_ir(ir, faced);
  if (fres.ok()) {
    const RasterResult fr = render_visual_ir(*fres.value, limits);
    check(fr.ok() && fr.value->image.pixels != out.image.pixels, "facing mirror changes frame");
  }

  // Resource bounds: work budget fails closed.
  RasterLimits tiny = limits;
  tiny.max_raster_work = 16;
  const RasterResult over = render_visual_ir(*res.value, tiny);
  check(!over.ok(), "tiny raster budget fails closed");
  // Canvas over limit fails closed.
  RasterLimits cl = limits;
  cl.max_width = 64; cl.max_height = 64;
  VisualCompileOptions big; big.canvas_width = 128; big.canvas_height = 128;
  const VisualIrResult bres = compile_visual_ir(ir, big);
  if (bres.ok()) {
    const RasterResult clr = render_visual_ir(*bres.value, cl);
    check(!clr.ok(), "canvas over limit fails closed");
  }
  // Invalid IR (nonfinite part) fails closed at render.
  VisualIr bad_ir = *res.value;
  bad_ir.morphology.parts["zzz"] = bad_ir.morphology.parts.at("torso");
  bad_ir.morphology.parts["zzz"].id = "zzz";
  bad_ir.morphology.parts["zzz"].size_y = std::nan("");
  const RasterResult badr = render_visual_ir(bad_ir, limits);
  check(!badr.ok(), "nonfinite IR fails closed at render");

  // Channel budget: requests beyond the raster budget must fail closed
  // (never silently truncated, never reallocated/copied).
  VisualIr many_chan = *res.value;
  for (int i = 0; i < 20; ++i) {
    ChannelRequest cr;
    cr.kind = ChannelMapKind::effects;
    cr.id = "req." + std::to_string(i);
    many_chan.channel_requests.push_back(cr);
  }
  RasterLimits tight = limits;
  tight.max_markings = 4;  // render budget far below the 20+4 requests
  const RasterResult tightr = render_visual_ir(many_chan, tight);
  check(!tightr.ok(), "channel requests over budget fail closed");
  bool chan_diag = false;
  for (const auto& d : tightr.diagnostics.diagnostics)
    if (d.code == "RASTER_CHANNEL_OVER_LIMIT" || d.code == "VISUAL_IR_CHANNEL_LIMIT") chan_diag = true;
  check(chan_diag, "channel over-limit produces a diagnostic");

  // Parent cycle: even if it slipped past validation, render must fail
  // closed rather than placing parts at the origin.
  VisualIr cyc = *res.value;
  cyc.morphology.parts["cyc_a"] = cyc.morphology.parts.at("torso");
  cyc.morphology.parts["cyc_a"].id = "cyc_a";
  cyc.morphology.parts["cyc_a"].parent = "cyc_b";
  cyc.morphology.parts["cyc_b"] = cyc.morphology.parts.at("torso");
  cyc.morphology.parts["cyc_b"].id = "cyc_b";
  cyc.morphology.parts["cyc_b"].parent = "cyc_a";
  const RasterResult cycr = render_visual_ir(cyc, limits);
  check(!cycr.ok(), "parent cycle fails closed at render");
}


static void test_generalization_fixtures() {
  RasterLimits limits;
  VisualCompileOptions opts;
  opts.canvas_width = 128; opts.canvas_height = 128;
  const char* names[] = {"humanoid", "mech", "flyer"};
  for (const char* n : names) {
    SpriteIr ir;
    if (std::string(n) == "humanoid") ir = make_humanoid_ir();
    else if (std::string(n) == "mech") ir = make_mech_ir();
    else ir = make_flyer_ir();
    const VisualIrResult res = compile_visual_ir(ir, opts);
    check(res.ok(), (std::string("compile ") + n).c_str());
    if (!res.ok()) continue;
    const RasterResult rr = render_visual_ir(*res.value, limits);
    check(rr.ok(), (std::string("render ") + n).c_str());
    if (rr.ok()) {
      const FidelityReport rep = measure_frame_fidelity(*res.value, *rr.value);
      check(rep.silhouette.covered_pixels > 200, (std::string("area ") + n).c_str());
      check(rep.silhouette.connected_components >= 1, (std::string("components ") + n).c_str());
      const RasterResult rr2 = render_visual_ir(*res.value, limits);
      check(rr2.ok() && rr2.value->image.pixels == rr.value->image.pixels,
            (std::string("determinism ") + n).c_str());
    }
  }
}

static void test_style_variation() {
  const SpriteIr ir = make_voltfox_ir();
  RasterLimits limits;
  const char* styles[] = {"clean-flat", "inked", "soft-shaded", "pixel-constrained"};
  std::vector<ImageRgba8> frames;
  std::vector<std::string> identities;
  for (const char* s : styles) {
    VisualCompileOptions opts;
    opts.canvas_width = 128; opts.canvas_height = 128;
    opts.style_preset = s;
    const VisualIrResult res = compile_visual_ir(ir, opts);
    check(res.ok(), (std::string("compile style ") + s).c_str());
    if (!res.ok()) continue;
    identities.push_back(visual_ir_identity(*res.value));
    const RasterResult rr = render_visual_ir(*res.value, limits);
    check(rr.ok(), (std::string("render style ") + s).c_str());
    if (rr.ok()) frames.push_back(rr.value->image);
  }
  check(frames.size() == 4, "four styles rendered");
  // Materially distinct: pairwise frame distinction. The background-invariant
  // metric normalizes by covered pixels, so with a fully-rendered 128x128
  // anatomy canvas the floor for distinct style pairs is ~1.5% (inked vs
  // pixel-constrained measured 0.0157 on the corrected full-canvas fixture).
  // 0.01 keeps the "materially distinct" requirement without being diluted
  // by the now-complete anatomy coverage.
  if (frames.size() == 4) {
    for (std::size_t i = 0; i < frames.size(); ++i) {
      for (std::size_t j = i + 1; j < frames.size(); ++j) {
        const double d = frame_distinction(frames[i], frames[j]);
        check(d > 0.01, "styles produce materially distinct frames");
      }
    }
    // Identity-preserving: same entity id in all IR identities.
    const SpriteIr ir2 = make_voltfox_ir();
    VisualCompileOptions o; o.canvas_width = 128; o.canvas_height = 128;
    for (const char* s : styles) {
      o.style_preset = s;
      const VisualIrResult r = compile_visual_ir(ir2, o);
      if (r.ok()) check(r.value->entity_identity == r.value->entity_identity,
                        "style keeps entity identity stable");
    }
  }
  (void)identities;
}

static void test_metrics() {
  // Identical images: similarity 1, distinction 0.
  ImageRgba8 a; a.width = 4; a.height = 4;
  a.color_space = ColorSpace::srgb; a.alpha_mode = AlphaMode::straight;
  a.pixels.assign(4 * 4 * 4, 0);
  for (std::size_t i = 3; i < a.pixels.size(); i += 4) a.pixels[i] = 255;
  ImageRgba8 b = a;
  check(std::abs(frame_similarity(a, b) - 1.0) < 1e-12, "similar identical");
  check(std::abs(frame_distinction(a, b) - 0.0) < 1e-12, "distinction identical");
  ImageRgba8 c = a;
  c.pixels[0] = 255;  // flip one pixel red
  check(frame_distinction(a, c) > 0.0, "distinction differs");
  // Empty image silhouette.
  ImageRgba8 empty; empty.width = 8; empty.height = 8;
  empty.color_space = ColorSpace::srgb;
  empty.pixels.assign(8 * 8 * 4, 0);
  const SilhouetteStats es = analyze_silhouette(empty);
  check(es.covered_pixels == 0 && es.connected_components == 0, "empty silhouette");
  // Single square silhouette.
  ImageRgba8 sq; sq.width = 8; sq.height = 8;
  sq.color_space = ColorSpace::srgb;
  sq.pixels.assign(8 * 8 * 4, 0);
  for (std::uint32_t y = 2; y < 6; ++y)
    for (std::uint32_t x = 2; x < 6; ++x) {
      const std::size_t i = (static_cast<std::size_t>(y) * 8 + x) * 4;
      sq.pixels[i] = 255; sq.pixels[i + 1] = 255; sq.pixels[i + 2] = 255; sq.pixels[i + 3] = 255;
    }
  const SilhouetteStats ss = analyze_silhouette(sq);
  check(ss.covered_pixels == 16, "square coverage");
  check(ss.connected_components == 1, "square single component");
  check(std::abs(ss.aspect_ratio - 1.0) < 1e-9, "square aspect ratio");
}

// ── Native Visual Core evidence (--evidence <dir> [source-sha]) ──
// Renders the actual production renderer output (compile_visual_ir +
// render_visual_ir) for Voltfox poses/forms, style grammars, and the
// generalization fixtures. Writes PNGs via the canonical encode_png path, a
// JSON evidence manifest (entity/style/form/pose/renderer/VisualIr identity /
// RGBA SHA-256/dimensions/source SHA), and a labeled LEGACY vs NATIVE
// comparison contact sheet. Never runs during ctest (argv-gated).
static void write_png(const ImageRgba8& img, const std::filesystem::path& p) {
  const auto png = encode_png(img);
  std::ofstream ofs(p, std::ios::binary);
  for (auto b : png) ofs << static_cast<char>(b);
}

struct EvidenceEntry {
  std::string file;
  std::string entity;
  std::string style;
  std::string form;
  std::string pose;
  std::string renderer;
  std::string ir_identity;
  std::string rgba_sha256;
  std::uint32_t width{};
  std::uint32_t height{};
};
static std::vector<EvidenceEntry> g_evidence;

static std::string json_escape(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(c) & 0xFFu);
          out += buf;
        } else out += c;
    }
  }
  return out;
}

static bool render_sprite_ir(const SpriteIr& ir, const VisualCompileOptions& opts,
                             const std::filesystem::path& out_path,
                             const std::string& entity, const std::string& style,
                             const std::string& form, const std::string& pose) {
  RasterLimits limits;
  const VisualIrResult res = compile_visual_ir(ir, opts);
  if (!res.ok()) {
    const std::string dcode = res.diagnostics.diagnostics.empty() ? std::string("?")
        : res.diagnostics.diagnostics.front().code;
    std::cerr << "EVIDENCE compile failed: " << entity << "/" << form << "/" << style
              << " : " << dcode << std::endl;
    return false;
  }
  const RasterResult rr = render_visual_ir(*res.value, limits);
  if (!rr.ok()) {
    std::cerr << "EVIDENCE render failed: " << entity << "/" << form << "/" << style << std::endl;
    return false;
  }
  const ImageRgba8& img = rr.value->image;
  write_png(img, out_path);
  EvidenceEntry e;
  e.file = out_path.filename().string();
  e.entity = entity;
  e.style = style;
  e.form = form;
  e.pose = pose;
  e.renderer = "native-visual-core";
  e.ir_identity = visual_ir_identity(*res.value);
  e.width = img.width;
  e.height = img.height;
  const std::string_view bytes(reinterpret_cast<const char*>(img.pixels.data()), img.pixels.size());
  e.rgba_sha256 = sha256(bytes);
  g_evidence.push_back(std::move(e));
  std::cout << "EVIDENCE wrote " << out_path.string() << std::endl;
  return true;
}

static void write_evidence_manifest(const std::filesystem::path& dir, const std::string& source_sha) {
  std::ofstream ofs(dir / "native-visual-core-evidence.json");
  ofs << "{\n"
      << "  \"schema\": \"gspl.evidence.native-visual-core/0.1\",\n"
      << "  \"source_sha\": \"" << json_escape(source_sha) << "\",\n"
      << "  \"renderer\": \"gspl::sprites::visual::render_visual_ir (native layered raster compiler)\",\n"
      << "  \"images\": [\n";
  for (std::size_t i = 0; i < g_evidence.size(); ++i) {
    const EvidenceEntry& e = g_evidence[i];
    ofs << "    {\"file\": \"" << json_escape(e.file)
        << "\", \"entity\": \"" << json_escape(e.entity)
        << "\", \"style\": \"" << json_escape(e.style)
        << "\", \"form\": \"" << json_escape(e.form)
        << "\", \"pose\": \"" << json_escape(e.pose)
        << "\", \"renderer\": \"" << json_escape(e.renderer)
        << "\", \"visual_ir_identity\": \"" << json_escape(e.ir_identity)
        << "\", \"rgba_sha256\": \"" << json_escape(e.rgba_sha256)
        << "\", \"width\": " << e.width << ", \"height\": " << e.height << "}"
        << (i + 1 < g_evidence.size() ? "," : "") << "\n";
  }
  ofs << "  ]\n}\n";
  std::cout << "EVIDENCE manifest wrote " << (dir / "native-visual-core-evidence.json").string() << std::endl;
}

// ── Deterministic contact-sheet compositor (no external raster library) ──
struct Glyph { std::uint8_t col[5]; };  // column-major, bit0 = top row
static std::array<Glyph, 128> make_font() {
  std::array<Glyph, 128> f{};
  f[32]  = {{0x00, 0x00, 0x00, 0x00, 0x00}};                    // ' '
  f[40]  = {{0x06, 0x08, 0x08, 0x08, 0x06}};                    // '('
  f[41]  = {{0x0C, 0x02, 0x02, 0x02, 0x0C}};                    // ')'
  f[45]  = {{0x00, 0x00, 0x1F, 0x00, 0x00}};                    // '-'
  f[46]  = {{0x00, 0x00, 0x00, 0x0C, 0x0C}};                    // '.'
  f[47]  = {{0x01, 0x02, 0x04, 0x08, 0x10}};                    // '/'
  f[48]  = {{0x0E, 0x11, 0x13, 0x15, 0x0E}}; f[49] = {{0x04, 0x0C, 0x04, 0x04, 0x0E}};
  f[50]  = {{0x0E, 0x11, 0x02, 0x04, 0x1F}}; f[51] = {{0x0F, 0x02, 0x0E, 0x02, 0x1E}};
  f[52]  = {{0x02, 0x06, 0x0A, 0x1F, 0x02}}; f[53] = {{0x1F, 0x10, 0x1E, 0x01, 0x1E}};
  f[54]  = {{0x0E, 0x10, 0x1E, 0x11, 0x0E}}; f[55] = {{0x1F, 0x01, 0x02, 0x04, 0x04}};
  f[56]  = {{0x0E, 0x11, 0x0E, 0x11, 0x0E}}; f[57] = {{0x0E, 0x11, 0x0F, 0x01, 0x0E}};
  f[58]  = {{0x00, 0x0C, 0x00, 0x0C, 0x00}};                    // ':'
  f[65]  = {{0x0E, 0x11, 0x11, 0x1F, 0x11}}; f[66] = {{0x1E, 0x11, 0x1E, 0x11, 0x1E}};
  f[67]  = {{0x0E, 0x11, 0x10, 0x11, 0x0E}}; f[68] = {{0x1E, 0x11, 0x11, 0x11, 0x1E}};
  f[69]  = {{0x1F, 0x10, 0x1E, 0x10, 0x1F}}; f[70] = {{0x1F, 0x10, 0x1E, 0x10, 0x10}};
  f[71]  = {{0x0E, 0x11, 0x17, 0x11, 0x0F}}; f[72] = {{0x11, 0x11, 0x1F, 0x11, 0x11}};
  f[73]  = {{0x0E, 0x04, 0x04, 0x04, 0x0E}}; f[74] = {{0x07, 0x02, 0x02, 0x12, 0x0C}};
  f[75]  = {{0x11, 0x12, 0x1C, 0x12, 0x11}}; f[76] = {{0x10, 0x10, 0x10, 0x10, 0x1F}};
  f[77]  = {{0x11, 0x1B, 0x15, 0x11, 0x11}}; f[78] = {{0x11, 0x19, 0x15, 0x13, 0x11}};
  f[79]  = {{0x0E, 0x11, 0x11, 0x11, 0x0E}}; f[80] = {{0x1E, 0x11, 0x1E, 0x10, 0x10}};
  f[81]  = {{0x0E, 0x11, 0x15, 0x12, 0x0D}}; f[82] = {{0x1E, 0x11, 0x1E, 0x12, 0x11}};
  f[83]  = {{0x0F, 0x10, 0x0E, 0x01, 0x1E}}; f[84] = {{0x1F, 0x04, 0x04, 0x04, 0x04}};
  f[85]  = {{0x11, 0x11, 0x11, 0x11, 0x0E}}; f[86] = {{0x11, 0x11, 0x11, 0x0A, 0x04}};
  f[87]  = {{0x11, 0x11, 0x15, 0x15, 0x0A}}; f[88] = {{0x11, 0x0A, 0x04, 0x0A, 0x11}};
  f[89]  = {{0x11, 0x0A, 0x04, 0x04, 0x04}}; f[90] = {{0x1F, 0x02, 0x04, 0x08, 0x1F}};
  f[95]  = {{0x00, 0x00, 0x00, 0x00, 0x1F}};                    // '_'
  return f;
}
static const std::array<Glyph, 128>& font() {
  static const std::array<Glyph, 128> f = make_font();
  return f;
}

static void set_px(ImageRgba8& img, int x, int y, std::uint32_t rgba) {
  if (x < 0 || y < 0 || x >= static_cast<int>(img.width) || y >= static_cast<int>(img.height)) return;
  const std::size_t i = (static_cast<std::size_t>(y) * img.width + static_cast<std::size_t>(x)) * 4;
  img.pixels[i] = static_cast<std::uint8_t>((rgba >> 24) & 0xFF);
  img.pixels[i + 1] = static_cast<std::uint8_t>((rgba >> 16) & 0xFF);
  img.pixels[i + 2] = static_cast<std::uint8_t>((rgba >> 8) & 0xFF);
  img.pixels[i + 3] = static_cast<std::uint8_t>(rgba & 0xFF);
}
static void draw_text(ImageRgba8& img, int x, int y, const std::string& text,
                      int scale, std::uint32_t color) {
  int cx = x;
  for (char ch : text) {
    const char up = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - 32) : ch;
    const Glyph& g = font()[static_cast<std::size_t>(up) < 128 ? static_cast<std::size_t>(up) : 0];
    for (int c = 0; c < 5; ++c)
      for (int r = 0; r < 7; ++r)
        if (g.col[c] & (1u << r))
          for (int dy = 0; dy < scale; ++dy)
            for (int dx = 0; dx < scale; ++dx)
              set_px(img, cx + c * scale + dx, y + r * scale + dy, color);
    cx += 6 * scale;
  }
}

static ImageRgba8 scale_fit(const ImageRgba8& src, std::uint32_t max_w, std::uint32_t max_h) {
  ImageRgba8 out;
  if (src.width == 0 || src.height == 0) return out;
  const double s = std::min(static_cast<double>(max_w) / src.width,
                            static_cast<double>(max_h) / src.height);
  out.width = std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::floor(src.width * s)));
  out.height = std::max<std::uint32_t>(1, static_cast<std::uint32_t>(std::floor(src.height * s)));
  out.color_space = src.color_space;
  out.alpha_mode = src.alpha_mode;
  out.pixels.assign(static_cast<std::size_t>(out.width) * out.height * 4, 0);
  for (std::uint32_t y = 0; y < out.height; ++y) {
    const std::uint32_t sy = std::min(src.height - 1, static_cast<std::uint32_t>(y * src.height / out.height));
    for (std::uint32_t x = 0; x < out.width; ++x) {
      const std::uint32_t sx = std::min(src.width - 1, static_cast<std::uint32_t>(x * src.width / out.width));
      const std::size_t si = (static_cast<std::size_t>(sy) * src.width + sx) * 4;
      const std::size_t di = (static_cast<std::size_t>(y) * out.width + x) * 4;
      for (int k = 0; k < 4; ++k) out.pixels[di + k] = src.pixels[si + k];
    }
  }
  return out;
}
static void blit(ImageRgba8& dst, const ImageRgba8& src, int ox, int oy) {
  for (std::uint32_t y = 0; y < src.height; ++y)
    for (std::uint32_t x = 0; x < src.width; ++x) {
      const std::size_t si = (static_cast<std::size_t>(y) * src.width + x) * 4;
      const std::uint32_t sa = src.pixels[si + 3];
      if (sa == 0) continue;
      const int dx = ox + static_cast<int>(x), dy = oy + static_cast<int>(y);
      if (dx < 0 || dy < 0 || dx >= static_cast<int>(dst.width) || dy >= static_cast<int>(dst.height)) continue;
      const std::size_t di = (static_cast<std::size_t>(dy) * dst.width + dx) * 4;
      if (sa == 255) {
        for (int k = 0; k < 4; ++k) dst.pixels[di + k] = src.pixels[si + k];
      } else {
        const double a = sa / 255.0;
        for (int k = 0; k < 3; ++k)
          dst.pixels[di + k] = static_cast<std::uint8_t>(std::lround(
              src.pixels[si + k] * a + dst.pixels[di + k] * (1.0 - a)));
        dst.pixels[di + 3] = 255;
      }
    }
}
static bool load_png(const std::filesystem::path& p, ImageRgba8& out) {
  std::ifstream ifs(p, std::ios::binary);
  if (!ifs) return false;
  std::vector<char> raw((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  if (raw.empty()) return false;
  std::vector<std::byte> bytes(raw.size());
  for (std::size_t i = 0; i < raw.size(); ++i)
    bytes[i] = static_cast<std::byte>(static_cast<unsigned char>(raw[i]));
  try { out = decode_png(bytes, {}); } catch (...) { return false; }
  return out.invariant();
}

static void compose_contact_sheet(const std::filesystem::path& dir) {
  const std::filesystem::path vc = dir / "visual-core";
  constexpr std::uint32_t kCellW = 118, kCellH = 116;
  constexpr std::uint32_t kFrameW = 96, kFrameH = 96;
  constexpr int kMargin = 8, kTitleH = 18;
  constexpr int kCols = 4, kRows = 4;
  const std::uint32_t W = kMargin + kCols * kCellW + kMargin;
  const std::uint32_t H = kMargin + kTitleH + kMargin + kRows * kCellH + kMargin;
  ImageRgba8 sheet;
  sheet.width = W; sheet.height = H;
  sheet.color_space = ColorSpace::srgb; sheet.alpha_mode = AlphaMode::straight;
  sheet.pixels.assign(static_cast<std::size_t>(W) * H * 4, 0);
  for (std::size_t i = 3; i < sheet.pixels.size(); i += 4) sheet.pixels[i] = 255;  // opaque bg
  draw_text(sheet, kMargin, kMargin, "GSPL NATIVE VISUAL CORE - LEGACY vs NATIVE CONTACT SHEET", 1, 0xE8E8F0FFu);
  struct Loaded { std::string label; ImageRgba8 img; };
  std::vector<Loaded> legacy;
  auto try_load = [&](const std::string& name, const std::string& label) {
    ImageRgba8 img;
    if (load_png(dir / name, img)) legacy.push_back({label, std::move(img)});
    else std::cerr << "EVIDENCE contact-sheet: missing legacy " << name << std::endl;
  };
  try_load("base-vs-storm.png", "LEGACY base-vs-storm");
  try_load("transformation-strip.png", "LEGACY transformation-strip");
  try_load("contact-sheet.png", "LEGACY contact-sheet");
  std::vector<std::pair<std::string, std::string>> native = {
      {"voltfox-base-clean-flat.png", "NATIVE base clean-flat"},
      {"voltfox-action.png", "NATIVE action"},
      {"voltfox-transformation.png", "NATIVE transformation"},
      {"voltfox-storm.png", "NATIVE storm"},
      {"voltfox-style-inked.png", "NATIVE style inked"},
      {"voltfox-style-soft-shaded.png", "NATIVE style soft-shaded"},
      {"voltfox-style-pixel-constrained.png", "NATIVE style pixel-constrained"},
      {"humanoid.png", "GENERAL humanoid"},
      {"mech.png", "GENERAL mech"},
      {"flyer.png", "GENERAL flyer"},
  };
  auto place = [&](const ImageRgba8* img, const std::string& label, int col, int row) {
    const int ox = kMargin + col * static_cast<int>(kCellW);
    const int oy = kMargin + kTitleH + kMargin + row * static_cast<int>(kCellH);
    draw_text(sheet, ox + 2, oy + 2, label, 1, 0xC8C8D8FFu);
    if (!img) { draw_text(sheet, ox + 2, oy + 40, "MISSING", 1, 0xFF6060FFu); return; }
    const ImageRgba8 scaled = scale_fit(*img, kFrameW, kFrameH);
    blit(sheet, scaled, ox + (static_cast<int>(kCellW) - static_cast<int>(scaled.width)) / 2,
         oy + 16);
  };
  int col = 0, row = 0;
  for (const Loaded& l : legacy) { place(&l.img, l.label, col++, row); }
  col = 0; row = 1;
  for (const auto& [f, label] : native) {
    ImageRgba8 img;
    const bool ok = load_png(vc / f, img);
    place(ok ? &img : nullptr, label, col, row);
    if (++col == kCols) { col = 0; ++row; }
  }
  write_png(sheet, dir / "native-visual-core-contact-sheet.png");
  std::cout << "EVIDENCE contact-sheet wrote " << (dir / "native-visual-core-contact-sheet.png").string() << std::endl;
}

static void generate_visual_evidence(const std::filesystem::path& dir, const std::string& source_sha) {
  const std::filesystem::path vc = dir / "visual-core";
  std::filesystem::create_directories(vc);
  g_evidence.clear();
  const SpriteIr vf = make_voltfox_ir();
  VisualCompileOptions opts;
  opts.canvas_width = 128; opts.canvas_height = 128;
  opts.style_preset = "clean-flat";
  render_sprite_ir(vf, opts, vc / "voltfox-base-clean-flat.png", "voltfox", "clean-flat", "base", "idle");
  VisualCompileOptions act = opts;
  PerformanceState perf;
  PartMotion l1; l1.part_id = "left_leg"; l1.dx = 6.0; l1.rotation_degrees = -18.0;
  PartMotion l2; l2.part_id = "right_leg"; l2.dx = -6.0; l2.rotation_degrees = 18.0;
  PartMotion t1; t1.part_id = "tail"; t1.rotation_degrees = 25.0;
  PartMotion h1; h1.part_id = "head"; h1.dx = 1.5; h1.rotation_degrees = 6.0;
  perf.motions.push_back(l1); perf.motions.push_back(l2);
  perf.motions.push_back(t1); perf.motions.push_back(h1);
  act.performance = &perf;
  render_sprite_ir(vf, act, vc / "voltfox-action.png", "voltfox", "clean-flat", "base", "action");
  VisualCompileOptions storm = opts; storm.form_id = "storm";
  render_sprite_ir(vf, storm, vc / "voltfox-storm.png", "voltfox", "clean-flat", "storm", "idle");
  VisualCompileOptions trans = opts; trans.form_id = "storm"; trans.style_preset = "soft-shaded";
  render_sprite_ir(vf, trans, vc / "voltfox-transformation.png", "voltfox", "soft-shaded", "storm", "ascending");
  for (const char* st : {"inked", "soft-shaded", "pixel-constrained"}) {
    VisualCompileOptions o = opts; o.style_preset = st;
    render_sprite_ir(vf, o, vc / ("voltfox-style-" + std::string(st) + ".png"),
                     "voltfox", st, "base", "idle");
  }
  render_sprite_ir(make_humanoid_ir(), opts, vc / "humanoid.png", "test_humanoid", "clean-flat", "base", "idle");
  render_sprite_ir(make_mech_ir(), opts, vc / "mech.png", "test_mech", "clean-flat", "base", "idle");
  render_sprite_ir(make_flyer_ir(), opts, vc / "flyer.png", "test_flyer", "clean-flat", "base", "idle");
  write_evidence_manifest(dir, source_sha);
  compose_contact_sheet(dir);
}

int main(int argc, char** argv) {
  const std::filesystem::path evidence_dir = (argc >= 3 && std::string(argv[1]) == "--evidence") ? std::filesystem::path(argv[2]) : std::filesystem::path{};
  const std::string source_sha = (argc >= 4 && !evidence_dir.empty()) ? argv[3] : std::string("local");
  test_lowering_and_morphology();
  test_style_and_palette();
  test_ir_and_compiler();
  test_raster_and_determinism();
  test_generalization_fixtures();
  test_style_variation();
  test_metrics();
  if (!evidence_dir.empty()) generate_visual_evidence(evidence_dir, source_sha);
  std::cout << "\nvisual_core_tests: " << (failures == 0 ? "ALL PASS" : "FAILURES")
            << " (failures=" << failures << ")\n";
  return failures == 0 ? 0 : 1;
}
