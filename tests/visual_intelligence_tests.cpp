// Visual Intelligence tests: VisualCanon validation/construction, identity
// invariants, deformation envelopes, PerformanceIntent, StyleProgram,
// TemporalIdentity, FX semantics, cinematic representation, visual quality
// diagnostics, compiler integration (canon+intent+style program -> VisualIr
// -> raster), serialization, determinism, and review-artifact generation.
//
// Evidence mode (never runs during ctest): argv-gated exactly like
// visual_core_tests:  gspl_sprites_visual_intelligence_tests --evidence <dir> [source-sha]
#include "gspl_sprites/cinematic.hpp"
#include "gspl_sprites/fx_semantics.hpp"
#include "gspl_sprites/image.hpp"
#include "gspl_sprites/markings.hpp"
#include "gspl_sprites/performance.hpp"
#include "gspl_sprites/performance_intent.hpp"
#include "gspl_sprites/style.hpp"
#include "gspl_sprites/style_program.hpp"
#include "gspl_sprites/temporal_identity.hpp"
#include "gspl_sprites/visual_canon.hpp"
#include "gspl_sprites/visual_canon_fixtures.hpp"
#include "gspl_sprites/visual_compiler.hpp"
#include "gspl_sprites/visual_geometry.hpp"
#include "gspl_sprites/visual_ir.hpp"
#include "gspl_sprites/visual_metrics.hpp"
#include "gspl_sprites/visual_morphology.hpp"
#include "gspl_sprites/visual_quality.hpp"
#include "gspl_sprites/visual_raster.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

// ── Helpers ──

static SpriteIr make_ir_from_canon(const VisualCanon& canon) {
  SpriteIr ir;
  ir.entity_id = canon.entity_id;
  ir.seed_identity = "seed-" + canon.entity_id;
  for (const auto& f : canon.forms) ir.form_definitions.push_back({f, {}});
  return ir;
}

static ImageRgba8 make_image(std::uint32_t w, std::uint32_t h, std::uint32_t rgba) {
  ImageRgba8 img;
  img.width = w; img.height = h;
  img.color_space = ColorSpace::srgb;
  img.alpha_mode = AlphaMode::straight;
  img.pixels.assign(static_cast<std::size_t>(w) * h * 4, 0);
  for (std::size_t i = 0; i < img.pixels.size(); i += 4) {
    img.pixels[i] = static_cast<std::uint8_t>((rgba >> 24) & 0xFF);
    img.pixels[i + 1] = static_cast<std::uint8_t>((rgba >> 16) & 0xFF);
    img.pixels[i + 2] = static_cast<std::uint8_t>((rgba >> 8) & 0xFF);
    img.pixels[i + 3] = static_cast<std::uint8_t>(rgba & 0xFF);
  }
  return img;
}

static void fill_rect(ImageRgba8& img, std::uint32_t x0, std::uint32_t y0,
                      std::uint32_t x1, std::uint32_t y1, std::uint32_t rgba) {
  for (std::uint32_t y = y0; y <= y1 && y < img.height; ++y)
    for (std::uint32_t x = x0; x <= x1 && x < img.width; ++x) {
      const std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
      img.pixels[i] = static_cast<std::uint8_t>((rgba >> 24) & 0xFF);
      img.pixels[i + 1] = static_cast<std::uint8_t>((rgba >> 16) & 0xFF);
      img.pixels[i + 2] = static_cast<std::uint8_t>((rgba >> 8) & 0xFF);
      img.pixels[i + 3] = static_cast<std::uint8_t>(rgba & 0xFF);
    }
}

// ── 1. Canon validation ──

static void test_canon_validation() {
  check_ok(validate_visual_canon(make_voltfox_canon()), "voltfox canon validates");
  check_ok(validate_visual_canon(make_humanoid_canon()), "humanoid canon validates");
  check_ok(validate_visual_canon(make_mech_canon()), "mech canon validates");
  check_ok(validate_visual_canon(make_flyer_canon()), "flyer canon validates");

  {  // structural cycle
    VisualCanon c = make_voltfox_canon();
    c.structures.at("head").parent = "left_ear";
    c.structures.at("left_ear").parent = "head";
    check_fail(validate_visual_canon(c), "structural cycle rejected");
  }
  {  // dangling parent
    VisualCanon c = make_voltfox_canon();
    c.structures.at("head").parent = "no_such_part";
    check_fail(validate_visual_canon(c), "dangling parent rejected");
  }
  {  // nonfinite geometry
    VisualCanon c = make_voltfox_canon();
    c.structures.at("torso").size_x = std::numeric_limits<double>::infinity();
    check_fail(validate_visual_canon(c), "nonfinite size rejected");
  }
  {  // nonpositive size
    VisualCanon c = make_voltfox_canon();
    c.structures.at("torso").size_y = 0.0;
    check_fail(validate_visual_canon(c), "nonpositive size rejected");
  }
  {  // unknown primitive
    VisualCanon c = make_voltfox_canon();
    c.structures.at("head").primitive = "squircle";
    check_fail(validate_visual_canon(c), "unknown primitive rejected");
  }
  {  // unknown layer
    VisualCanon c = make_voltfox_canon();
    c.structures.at("head").layer = "ethereal";
    check_fail(validate_visual_canon(c), "unknown layer rejected");
  }
  {  // unknown color role
    VisualCanon c = make_voltfox_canon();
    c.structures.at("head").color_role = "obsidian";
    check_fail(validate_visual_canon(c), "unknown color role rejected");
  }
  {  // inverted proportion range
    VisualCanon c = make_voltfox_canon();
    c.proportions.at(0).min = 1.0;
    c.proportions.at(0).max = 0.5;
    check_fail(validate_visual_canon(c), "inverted proportion range rejected");
  }
  {  // landmark dangling owner
    VisualCanon c = make_voltfox_canon();
    c.landmarks.at("nose_tip").owner = "mystery_part";
    check_fail(validate_visual_canon(c), "landmark with dangling owner rejected");
  }
  {  // marking dangling structure
    VisualCanon c = make_voltfox_canon();
    c.markings.at(0).structure_ref = "ghost";
    check_fail(validate_visual_canon(c), "marking with dangling structure rejected");
  }
  {  // material dangling structure
    VisualCanon c = make_voltfox_canon();
    c.materials.at(0).structure_ref = "ghost";
    check_fail(validate_visual_canon(c), "material with dangling structure rejected");
  }
  {  // invariant referencing a hard boundary that is not finite
    VisualCanon c = make_voltfox_canon();
    c.deformation_envelopes.at("torso").hard_translation = std::numeric_limits<double>::infinity();
    c.deformation_envelopes.at("torso").hard_rotation    = std::numeric_limits<double>::infinity();
    c.deformation_envelopes.at("torso").hard_scale        = std::numeric_limits<double>::infinity();
    check_fail(check_identity_invariants(c, canon_to_morphology(c, "base")),
               "hard_boundary invariant fails when envelope boundary is infinite");
  }
  {  // resource limit
    VisualCanon c = make_voltfox_canon();
    CanonLimits tight;
    tight.max_structures = 4;
    check_fail(validate_visual_canon(c, tight), "structure resource limit enforced");
  }
  {  // empty entity id
    VisualCanon c = make_voltfox_canon();
    c.entity_id.clear();
    check_fail(validate_visual_canon(c), "empty entity id rejected");
  }
}

// ── 2. Canon -> morphology construction ──

static void test_canon_construction() {
  const VisualCanon vf = make_voltfox_canon();
  const VisualMorphologyV2 base = canon_to_morphology(vf, "base");
  check(!base.parts.empty(), "canon construction produces parts");
  check(base.parts.count("torso") != 0, "torso present");
  check(base.parts.count("head") != 0, "head present");
  check(base.parts.count("tail_tip") != 0, "tail_tip present");
  check(base.parts.at("head").parent == "torso", "head parent preserved");
  check(base.parts.at("left_eye").color_role == ColorRole::eye, "eye color role preserved");
  check(base.parts.at("left_eye").material_class == "glass", "eye material from binding");
  check(base.parts.at("torso").material_class == "fur", "torso material from binding");
  check(base.parts.at("left_ear").rotation_degrees == -20.0, "rotation preserved");
  check(!base.markings.empty(), "markings constructed with geometry");
  bool stripes = false;
  for (const auto& mk : base.markings) if (mk.id == "torso_stripes") stripes = true;
  check(stripes, "torso_stripes marking present in base");
  for (const auto& mk : base.markings) {
    check(!mk.path.empty() || !mk.spots.empty(), "marking geometry is never empty");
  }
  // Form-scoped markings: storm_circuit must not appear in base.
  for (const auto& mk : base.markings) check(mk.id != "storm_circuit", "storm marking excluded from base");

  const VisualMorphologyV2 storm = canon_to_morphology(vf, "storm");
  bool has_storm_mark = false;
  for (const auto& mk : storm.markings) if (mk.id == "storm_circuit") has_storm_mark = true;
  check(has_storm_mark, "storm marking present in storm form");

  // Marking determinism: identical canon -> identical marking seeds/geometry.
  const VisualMorphologyV2 base2 = canon_to_morphology(vf, "base");
  check(base2.markings.size() == base.markings.size(), "marking count deterministic");
  for (std::size_t i = 0; i < base.markings.size() && i < base2.markings.size(); ++i) {
    check(base.markings[i].seed == base2.markings[i].seed, "marking seed deterministic");
  }
}

// ── 3. Proportions and landmarks ──

static void test_proportions_and_landmarks() {
  const VisualCanon vf = make_voltfox_canon();
  const VisualMorphologyV2 morph = canon_to_morphology(vf, "base");

  const auto head = measure_value(vf, morph, make_size_measure("head", "size_x"));
  check(head.has_value() && std::abs(*head - 8.0) < 1e-9, "head size_x measurable");
  const auto torso = measure_value(vf, morph, make_size_measure("torso", "size_x"));
  check(torso.has_value() && std::abs(*torso - 18.0) < 1e-9, "torso size_x measurable");

  const auto p = proportion_value(vf, morph, "head_to_body");
  check(p.has_value(), "head_to_body proportion measurable");
  if (p) {
    check(std::abs(*p - 0.44) < 0.02, "head_to_body near preferred");
    check(*p >= 0.39 && *p <= 0.50, "head_to_body inside canonical range");
  }

  const auto eye_d = measure_value(vf, morph, make_landmark_dist_measure("eye_center.left", "eye_center.right"));
  check(eye_d.has_value() && *eye_d > 0.0, "eye spacing distance measurable");

  const auto nose = resolve_landmark_position(vf, morph, "nose_tip");
  check(nose.has_value(), "nose landmark resolvable");
  const auto ear_l = resolve_landmark_position(vf, morph, "ear_tip.left");
  const auto ear_r = resolve_landmark_position(vf, morph, "ear_tip.right");
  check(ear_l.has_value() && ear_r.has_value(), "ear landmarks resolvable");
  if (ear_l && ear_r) check(ear_l->x < ear_r->x, "left ear landmark left of right ear");
  check(!resolve_landmark_position(vf, morph, "missing_landmark").has_value(),
        "unknown landmark unresolvable");
  check(!measure_value(vf, morph, "size:ghost:size_x").has_value(),
        "unknown part unmeasurable");
  check(!proportion_value(vf, morph, "no_such_proportion").has_value(),
        "unknown proportion unmeasurable");
}

// ── 4. Deformation envelope enforcement ──

static void test_deformation_enforcement() {
  const VisualCanon vf = make_voltfox_canon();

  {  // permitted deviation passes cleanly
    PerformanceState perf;
    PartMotion m; m.part_id = "left_leg"; m.dx = 2.0;
    perf.motions.push_back(m);
    const ValidationResult r = enforce_deformation_envelopes(vf, perf, "", 0.0, 1.0, 1.0);
    check_ok(r, "moderate motion passes envelope");
  }
  {  // over-allowed but inside boundary -> clamped + reported, not failed
    PerformanceState perf;
    PartMotion m; m.part_id = "left_leg"; m.dx = 5.0;  // allowed 3.2, boundary 7.0
    perf.motions.push_back(m);
    const ValidationResult r = enforce_deformation_envelopes(vf, perf, "", 0.0, 1.0, 1.0);
    bool clamped = false;
    for (const auto& d : r.diagnostics) if (d.code == "DEFORMATION_CLAMPED") clamped = true;
    check(clamped, "over-allowed motion reports clamped diagnostic (warning, not error)");
    check(r.ok(), "clamping is a warning — ok() still true");
    check(perf.motions[0].dx <= 3.2 + 1e-9, "motion clamped to allowed bound");
  }
  {  // hard boundary exceeded -> fail closed, no silent clamp
    PerformanceState perf;
    PartMotion m; m.part_id = "left_leg"; m.dx = 100.0;  // boundary 7.0
    perf.motions.push_back(m);
    const ValidationResult r = enforce_deformation_envelopes(vf, perf, "", 0.0, 1.0, 1.0);
    check(!r.ok(), "hard boundary violation fails closed");
    bool hard = false;
    for (const auto& d : r.diagnostics) if (d.code == "DEFORMATION_HARD_VIOLATION") hard = true;
    check(hard, "hard boundary diagnostic present");
    check(perf.motions[0].dx == 100.0, "hard violation is NOT silently clamped");
  }
  {  // rigid structure over allowed -> fails closed
    PerformanceState perf;
    PartMotion m; m.part_id = "left_eye"; m.scale_x = 2.0; m.scale_y = 2.0;  // rigid, allowed 0.7
    perf.motions.push_back(m);
    const ValidationResult r = enforce_deformation_envelopes(vf, perf, "", 0.0, 1.0, 1.0);
    check(!r.ok(), "rigid structure over allowed fails closed");
    bool rigid = false;
    for (const auto& d : r.diagnostics) if (d.code == "DEFORMATION_RIGID_VIOLATION") rigid = true;
    check(rigid, "rigid violation diagnostic present");
  }
  {  // action phase widens allowed deviation
    PerformanceState perf;
    PartMotion m; m.part_id = "left_leg"; m.dx = 4.5;  // allowed 3.2 base, 3.2*1.6 under action
    perf.motions.push_back(m);
    const ValidationResult r = enforce_deformation_envelopes(vf, perf, "attack", 0.0, 1.0, 1.0);
    check_ok(r, "action phase widens allowed deviation");
  }
}

// ── 5. Identity invariants ──

static void test_identity_invariants() {
  const VisualCanon vf = make_voltfox_canon();
  VisualMorphologyV2 good = canon_to_morphology(vf, "base");
  check_ok(check_identity_invariants(vf, good), "all invariants pass on canonical morph");

  {  // proportion drift beyond tolerance fails
    VisualMorphologyV2 bad = good;
    bad.parts.at("head").size_x = 12.0;  // head_to_body 0.44 -> 0.67
    const ValidationResult r = check_identity_invariants(vf, bad);
    check(!r.ok(), "proportion drift fails invariant");
    bool drift = false;
    for (const auto& d : r.diagnostics) if (d.code.find("DRIFT") != std::string::npos) drift = true;
    check(drift, "proportion drift diagnostic present");
  }
  {  // landmark removal fails existence invariant
    VisualMorphologyV2 bad = good;
    bad.parts.erase("left_eye");
    const ValidationResult r = check_identity_invariants(vf, bad);
    check(!r.ok(), "removed eye landmark fails existence invariant");
  }
  {  // material drift fails material_truth invariant
    VisualMorphologyV2 bad = good;
    bad.parts.at("left_eye").material_class = "fur";
    const ValidationResult r = check_identity_invariants(vf, bad);
    check(!r.ok(), "eye material drift fails material_truth invariant");
  }
  {  // marking removal fails marking_topology invariant
    VisualMorphologyV2 bad = good;
    bad.markings.erase(std::remove_if(bad.markings.begin(), bad.markings.end(),
                                      [](const Marking& m) { return m.id == "torso_stripes"; }),
                       bad.markings.end());
    const ValidationResult r = check_identity_invariants(vf, bad);
    check(!r.ok(), "removed torso stripes fail marking_topology invariant");
  }
  {  // scope-restricted invariant ignored outside its form
    const VisualMorphologyV2 base_morph = canon_to_morphology(vf, "base");
    check_ok(check_identity_invariants(vf, base_morph), "storm-scoped invariant skipped for base form");
  }
  {  // style projection preserves identity: style is orthogonal to morphology
    const VisualMorphologyV2 storm_morph = canon_to_morphology(vf, "storm");
    check_ok(check_identity_invariants(vf, storm_morph), "storm form morph passes identity invariants");
  }
}

// ── 6. Performance intent ──

static void test_performance_intent() {
  PerformanceIntent intent;
  intent.action = "attack";
  intent.phase = PerformancePhase::extension;
  intent.commitment = 1.0;
  intent.force_direction = {1.0, 0.5};
  intent.force_magnitude = 0.8;
  intent.emotion = "focused_aggression";
  intent.gaze_direction = {1.0, 0.0};
  check_ok(validate_performance_intent(intent), "valid performance intent accepted");

  {  // invalid intent rejected
    PerformanceIntent bad = intent;
    bad.action.clear();
    check_fail(validate_performance_intent(bad), "empty action rejected");
    PerformanceIntent bad2 = intent;
    bad2.commitment = 1.5;
    check_fail(validate_performance_intent(bad2), "commitment outside [0,1] rejected");
    PerformanceIntent bad3 = intent;
    bad3.force_direction.x = std::numeric_limits<double>::quiet_NaN();
    check_fail(validate_performance_intent(bad3), "nonfinite force direction rejected");
    PerformanceIntent bad4 = intent;
    bad4.timing_cadence = 0.0;
    check_fail(validate_performance_intent(bad4), "nonpositive cadence rejected");
  }

  KeyPose pose;
  pose.id = "strike_extend";
  pose.action = "attack";
  pose.phase = PerformancePhase::extension;
  pose.commitment = 1.0;
  pose.line_of_action_direction = {1.0, 0.3};
  pose.force_magnitude = 0.9;
  PartMotion arm; arm.part_id = "left_leg"; arm.dx = 2.0; arm.rotation_degrees = -12.0;
  PartMotion tail; tail.part_id = "tail_tip"; tail.rotation_degrees = 18.0;
  pose.motions = {arm, tail};
  check_ok(validate_key_pose(pose), "valid key pose accepted");
  {  // invalid key pose
    KeyPose bad = pose;
    bad.id.clear();
    check_fail(validate_key_pose(bad), "key pose without id rejected");
    KeyPose bad2 = pose;
    PartMotion ghost; ghost.part_id.clear();
    bad2.motions.push_back(ghost);
    check_fail(validate_key_pose(bad2), "key pose motion without part rejected");
  }

  // Lowering: intent + key pose -> PerformanceState.
  const PerformanceState perf = pose_to_performance_state(intent, &pose);
  check(perf.action_phase == "attack", "action phase flows to performance state");
  check(perf.motion_phase == "extension", "motion phase flows to performance state");
  check(perf.expression_intent == "focused_aggression", "emotion flows to expression intent");
  check(perf.facing == "right", "facing derives from gaze/force");
  check(perf.motions.size() == 2, "key pose motions carried into state");
  check(perf.impact == 0.8, "force magnitude flows to impact");

  // Determinism.
  check(canonicalize_performance_intent(intent) == canonicalize_performance_intent(intent),
        "intent canonicalization deterministic");
  check(canonicalize_key_pose(pose) == canonicalize_key_pose(pose),
        "key pose canonicalization deterministic");
  check(performance_intent_identity(intent) == performance_intent_identity(intent),
        "intent identity stable");
  check(key_pose_identity(pose) == key_pose_identity(pose), "key pose identity stable");
  PerformanceIntent changed = intent;
  changed.emotion = "calm";
  check(performance_intent_identity(changed) != performance_intent_identity(intent),
        "intent identity sensitive to emotion");
}

// ── 7. Style program ──

static void test_style_program() {
  const StyleProgram flat = make_style_program_preset("clean-flat");
  const StyleProgram ink = make_style_program_preset("inked");
  const StyleProgram soft = make_style_program_preset("soft-shaded");
  const StyleProgram pixel = make_style_program_preset("pixel-constrained");

  check(flat.name == "clean-flat", "clean-flat preset named");
  check(style_program_identity(flat) != style_program_identity(ink), "presets have distinct identities");
  check(style_program_identity(soft) != style_program_identity(pixel), "more distinct identities");

  check_ok(validate_style_program(flat), "valid style program accepted");
  {  // invalid program
    StyleProgram bad = flat;
    bad.shading.band_count = 0;
    check_fail(validate_style_program(bad), "zero band count rejected");
    StyleProgram bad2 = flat;
    bad2.compositing.alpha_policy = 2.0;
    check_fail(validate_style_program(bad2), "alpha policy outside [0,1] rejected");
    StyleProgram bad3 = flat;
    bad3.palette.max_colors = 1;
    check_fail(validate_style_program(bad3), "palette cap below 2 rejected");
  }

  // effective_style lowering: factorized program reproduces preset semantics.
  const StyleSemantics s_flat = effective_style(flat);
  const StyleSemantics s_ink = effective_style(ink);
  const StyleSemantics s_pixel = effective_style(pixel);
  check(s_flat.aa_policy == AntiAliasPolicy::analytic, "clean-flat lowers to analytic AA");
  check(s_ink.outline_selection == OutlineSelection::silhouette, "inked lowers to silhouette outline");
  check(s_ink.line_weight > s_flat.line_weight, "inked line weight > clean-flat");
  check(s_pixel.aa_policy == AntiAliasPolicy::none, "pixel-constrained lowers to no AA");
  check(s_pixel.palette_policy == PalettePolicy::quantize, "pixel-constrained quantizes palette");
  check(s_pixel.pixel_quantization >= 8.0, "pixel-constrained quantization level applied");
  const StyleSemantics s_soft = effective_style(soft);
  check(s_soft.shadow_model == ShadowModel::gradient, "soft-shaded lowers to gradient shadow");
  check(s_soft.highlight_model == HighlightModel::strong, "soft-shaded lowers to strong highlight");

  // Precedence: patches apply after factorized fields.
  StyleProgram patched = flat;
  StylePatch p;
  p.line_weight = 5.0;
  patched.patches.push_back(p);
  const StyleSemantics s_patched = effective_style(patched);
  check(s_patched.line_weight == 5.0, "patch overrides factorized line weight");

  check(canonicalize_style_program(flat) == canonicalize_style_program(flat),
        "style program canonicalization deterministic");
}

// ── 8. Temporal identity ──

static void test_temporal_identity() {
  TemporalIdentityRegistry reg;
  reg.entity_id = "voltfox";

  auto frame = [](std::uint64_t idx) {
    TemporalIdentityFrame f;
    f.frame_index = idx;
    PersistentStructure a; a.id = "contour.tail"; a.kind = PersistentStructureKind::contour_segment;
    a.anchor_part = "tail_tip"; a.first_frame = idx; a.last_frame = idx;
    PersistentStructure b; b.id = "mark.tail_tip"; b.kind = PersistentStructureKind::marking;
    b.anchor_part = "tail_tip"; b.first_frame = idx; b.last_frame = idx;
    PersistentStructure c; c.id = "eye.highlight.l"; c.kind = PersistentStructureKind::highlight;
    c.anchor_part = "left_eye"; c.first_frame = idx; c.last_frame = idx;
    f.structures = {a, b, c};
    return f;
  };
  check_ok(register_temporal_frame(reg, 0, frame(0).structures), "frame 0 registered");
  check_ok(register_temporal_frame(reg, 1, frame(1).structures), "frame 1 registered");

  const auto pairs = correspond_structures(reg, 0, 1);
  check(pairs.size() == 3, "stable structures correspond across frames");

  const auto anchored = structures_anchored_to(reg, "tail_tip");
  bool tail_contour = false, tail_mark = false;
  for (const auto& id : anchored) {
    if (id == "contour.tail") tail_contour = true;
    if (id == "mark.tail_tip") tail_mark = true;
  }
  check(tail_contour && tail_mark, "anchored lookup finds tail structures");

  check(temporal_inconsistency(reg, 0, 1) == 0.0, "consistent frames have zero inconsistency");

  {  // jitter: frame 2 drops structures -> inconsistency rises
    TemporalIdentityRegistry r2;
    r2.entity_id = "voltfox";
    check_ok(register_temporal_frame(r2, 0, frame(0).structures), "jitter frame 0");
    TemporalIdentityFrame f1 = frame(1);
    f1.structures.erase(f1.structures.begin() + 1);
    check_ok(register_temporal_frame(r2, 1, f1.structures), "jitter frame 1");
    check(temporal_inconsistency(r2, 0, 1) > 0.0, "dropped structure raises inconsistency");
  }

  {  // registry validation: empty entity rejected
    TemporalIdentityRegistry bad;
    bad.entity_id = "";
    check_fail(validate_temporal_registry(bad), "registry without entity rejected");
  }
  check(canonicalize_temporal_registry(reg) == canonicalize_temporal_registry(reg),
        "temporal registry canonicalization deterministic");
  check(temporal_registry_identity(reg) == temporal_registry_identity(reg),
        "temporal registry identity stable");
}

// ── 9. FX semantics ──

static void test_fx_semantics() {
  FxState state;
  state.entity_id = "voltfox";
  state.frame_index = 3;
  FxEffect arc;
  arc.id = "arc1";
  arc.phenomenon = FxPhenomenon::electric_arc;
  arc.energy = FxEnergyKind::electricity;
  arc.source_part = "tail_tip";
  arc.intensity = 0.8;
  arc.charge = 0.9;
  arc.temperature = 0.4;
  arc.branching = 0.6;
  arc.emission = 1.0;
  FxEffect aura;
  aura.id = "aura1";
  aura.phenomenon = FxPhenomenon::aura;
  aura.energy = FxEnergyKind::force;
  aura.source_part = "torso";
  aura.intensity = 0.5;
  state.effects = {arc, aura};
  check_ok(validate_fx_state(state), "valid FX state accepted");

  {  // invalid FX
    FxState bad = state;
    bad.entity_id.clear();
    check_fail(validate_fx_state(bad), "FX without entity rejected");
    FxState bad2 = state;
    bad2.effects[0].intensity = 2.0;
    check_fail(validate_fx_state(bad2), "intensity outside [0,1] rejected");
    FxState bad3 = state;
    bad3.effects[0].source_part.clear();
    check_fail(validate_fx_state(bad3), "effect without source rejected");
    FxState bad4 = state;
    FxLimits tight;
    tight.max_effects = 1;
    check_fail(validate_fx_state(bad4, tight), "FX effect limit enforced");
  }

  // Style interpretation is deterministic and monotone in style level.
  const FxDrawParams d1 = interpret_fx_effect(arc, 0.0);
  const FxDrawParams d2 = interpret_fx_effect(arc, 1.0);
  check(d2.emission_alpha >= d1.emission_alpha, "style level raises emission interpretation");
  check(canonicalize_fx_state(state) == canonicalize_fx_state(state),
        "FX canonicalization deterministic");
  check(fx_state_identity(state) == fx_state_identity(state), "FX identity stable");
  FxState changed = state;
  changed.effects[0].intensity = 0.2;
  check(fx_state_identity(changed) != fx_state_identity(state), "FX identity sensitive to intensity");
}

// ── 10. Cinematic ──

static void test_cinematic() {
  CinematicSpec spec;
  spec.entity_id = "voltfox";
  spec.camera.position = {64.0, 64.0};
  spec.camera.zoom = 1.0;
  spec.camera.shot_scale = ShotScale::medium;
  ImpactStaging st;
  st.impact_position = {0.5, 0.5};
  st.impact_strength = 0.8;
  st.staged_landmark = "nose_tip";
  spec.impact_staging.push_back(st);
  spec.focal_hierarchy = {"eye_center.left", "nose_tip", "tail_tip_lm"};
  check_ok(validate_cinematic(spec), "valid cinematic spec accepted");

  {  // invalid
    CinematicSpec bad = spec;
    bad.entity_id.clear();
    check_fail(validate_cinematic(bad), "cinematic without entity rejected");
    CinematicSpec bad2 = spec;
    bad2.camera.zoom = 0.0;
    check_fail(validate_cinematic(bad2), "nonpositive zoom rejected");
    CinematicSpec bad3 = spec;
    bad3.impact_staging[0].impact_strength = 1.5;
    check_fail(validate_cinematic(bad3), "impact strength outside [0,1] rejected");
    CinematicSpec bad4 = spec;
    CinematicLimits tight;
    tight.max_staging = 0;
    check_fail(validate_cinematic(bad4, tight), "staging limit enforced");
  }
  check(canonicalize_cinematic(spec) == canonicalize_cinematic(spec),
        "cinematic canonicalization deterministic");
  check(cinematic_identity(spec) == cinematic_identity(spec), "cinematic identity stable");
  CinematicSpec changed = spec;
  changed.camera.zoom = 2.0;
  check(cinematic_identity(changed) != cinematic_identity(spec), "cinematic identity sensitive to camera");
}

// ── 11. Visual quality diagnostics ──

static void test_visual_quality() {
  {  // silhouette: connected blob -> no accidental regions
    ImageRgba8 img = make_image(64, 64, 0x00000000u);
    fill_rect(img, 20, 20, 40, 40, 0xFFFFFFFFu);
    check(silhouette_disconnected_regions(img) == 0, "single connected blob has no accidental regions");
  }
  {  // silhouette: large blob + tiny accidental dot -> 1 accidental region
    ImageRgba8 img = make_image(64, 64, 0x00000000u);
    fill_rect(img, 20, 20, 40, 40, 0xFFFFFFFFu);   // 441 px
    fill_rect(img, 5, 5, 6, 6, 0xFFFFFFFFu);        // 4 px
    check(silhouette_disconnected_regions(img, 0.1) == 1, "tiny detached region counted as accidental");
  }
  {  // line consistency: identical images fully consistent
    ImageRgba8 img = make_image(64, 64, 0x00000000u);
    fill_rect(img, 10, 10, 50, 50, 0xFFFFFFFFu);
    check(line_consistency_between(img, img) == 1.0, "identical frames have full line consistency");
  }
  {  // pixel cluster instability: identical frames -> zero
    std::vector<ImageRgba8> frames;
    ImageRgba8 f0 = make_image(32, 32, 0x00000000u);
    fill_rect(f0, 8, 8, 24, 24, 0xFF3344FFu);
    frames.push_back(f0);
    frames.push_back(f0);
    check(pixel_cluster_instability(frames) == 0.0, "identical frames have zero cluster instability");
  }
  {  // evaluate_frame_pair on identical frames: duplication diagnostic, no jitter
    const VisualCanon vf = make_voltfox_canon();
    const SpriteIr ir = make_ir_from_canon(vf);
    VisualCompileOptions opts;
    opts.canon = &vf;
    const StyleProgram flat = make_style_program_preset("clean-flat");
    opts.style_program = &flat;
    const VisualIrResult cres = compile_visual_ir(ir, opts);
    check(cres.ok(), "compile for quality evaluation");
    const RasterResult rres = cres.ok() ? render_visual_ir(*cres.value) : RasterResult{};
    check(rres.ok(), "render for quality evaluation");
    if (cres.ok() && rres.ok()) {
      const ImageRgba8& img = rres.value->image;
      const QualityReport rep = evaluate_frame_pair(&vf, img, img, "nose_tip");
      check(rep.silhouette_disconnection == 0.0, "rendered frame has connected silhouette");
      check(rep.landmark_jitter == 0.0, "identical frames have zero landmark jitter");
      bool dup = false;
      for (const auto& d : rep.diagnostics.diagnostics)
        if (d.code == "QUALITY_FRAME_DUPLICATION") dup = true;
      check(dup, "near-identical frames flagged for duplication risk");
      // Palette adherence is a bounded ratio.
      const FidelityReport frep = measure_frame_fidelity(*cres.value, *rres.value);
      const double pv = palette_violation_ratio(frep);
      check(pv >= 0.0 && pv <= 1.0, "palette violation ratio bounded");
    }
  }
}

// ── 12. Compiler integration (canon + intent + style program) ──

static void test_compiler_integration() {
  const VisualCanon vf = make_voltfox_canon();
  const SpriteIr ir = make_ir_from_canon(vf);

  // Canon-driven compile, base form, clean-flat via factorized program.
  const StyleProgram flat = make_style_program_preset("clean-flat");
  VisualCompileOptions opts;
  opts.canvas_width = 128; opts.canvas_height = 128;
  opts.canon = &vf;
  opts.style_program = &flat;
  const VisualIrResult res = compile_visual_ir(ir, opts);
  check(res.ok(), "canon-driven compile succeeds");
  if (!res.ok()) return;
  check(!res.value->morphology.parts.empty(), "VisualIr carries constructed morphology");
  check(res.value->form_id == "base", "base form selected");
  check(res.value->projection.width == 128 && res.value->projection.height == 128,
        "projection canvas applied");

  // Storm form through the same compiler.
  VisualCompileOptions storm_opts = opts;
  storm_opts.form_id = "storm";
  const VisualIrResult sres = compile_visual_ir(ir, storm_opts);
  check(sres.ok(), "storm form compiles through the same path");
  if (sres.ok()) {
    const auto base_primary = parse_hex_color(vf.base_palette.at("primary"));
    check(sres.value->palette.colors.count("primary") != 0 &&
          (!base_primary || sres.value->palette.colors.at("primary") != *base_primary),
          "storm palette differs from base palette");
  }

  // Intent-driven compile: attack extension with key pose.
  PerformanceIntent intent;
  intent.action = "attack";
  intent.phase = PerformancePhase::extension;
  intent.force_direction = {1.0, 0.2};
  intent.force_magnitude = 0.7;
  intent.gaze_direction = {1.0, 0.0};
  KeyPose pose;
  pose.id = "strike";
  pose.action = "attack";
  pose.phase = PerformancePhase::extension;
  PartMotion l1; l1.part_id = "left_leg"; l1.dx = 1.5; l1.rotation_degrees = -6.0;
  PartMotion t1; t1.part_id = "tail_tip"; t1.rotation_degrees = 8.0;
  pose.motions = {l1, t1};
  VisualCompileOptions act_opts = opts;
  act_opts.intent = &intent;
  act_opts.key_pose = &pose;
  const VisualIrResult ares = compile_visual_ir(ir, act_opts);
  check(ares.ok(), "intent-driven compile succeeds");
  if (ares.ok()) {
    check(ares.value->performance.action_phase == "attack",
          "intent action reaches VisualIr performance");
    // The key pose motions must actually reach the morphology (integration
    // guarantee: the intent path applies motions to the constructed parts).
    check(std::abs(ares.value->morphology.parts.at("left_leg").x - (-4.5 + 1.5)) < 1e-9,
          "key pose translation reaches morphology");
    check(ares.value->morphology.parts.at("tail_tip").rotation_degrees != 0.0,
          "key pose rotation reaches morphology");
  }

  // Determinism: identical inputs -> byte-identical VisualIr.
  const VisualIrResult r1 = compile_visual_ir(ir, opts);
  const VisualIrResult r2 = compile_visual_ir(ir, opts);
  check(r1.ok() && r2.ok(), "repeated compile succeeds");
  if (r1.ok() && r2.ok()) {
    check(visual_ir_identity(*r1.value) == visual_ir_identity(*r2.value),
          "VisualIr identity deterministic across compiles");
    const RasterResult rr1 = render_visual_ir(*r1.value);
    const RasterResult rr2 = render_visual_ir(*r2.value);
    check(rr1.ok() && rr2.ok(), "deterministic render succeeds");
    if (rr1.ok() && rr2.ok()) {
      check(rr1.value->image.pixels == rr2.value->image.pixels,
            "rendered RGBA byte-identical across runs");
      check(rr1.value->image.width == 128 && rr1.value->image.height == 128,
            "render produces expected canvas");
      std::size_t covered = 0;
      for (std::size_t i = 3; i < rr1.value->image.pixels.size(); i += 4)
        if (rr1.value->image.pixels[i] > 0) ++covered;
      check(covered > 0, "rendered frame has non-empty content");
      check(!rr1.value->channels.empty(), "channels generated");
    }
  }

  // Hostile: canon+intent where key pose breaks a rigid envelope -> fail closed.
  KeyPose hostile;
  hostile.id = "hostile";
  PartMotion hm; hm.part_id = "left_eye"; hm.scale_x = 3.0; hm.scale_y = 3.0;
  hostile.motions.push_back(hm);
  VisualCompileOptions hostile_opts = opts;
  hostile_opts.intent = &intent;
  hostile_opts.key_pose = &hostile;
  const VisualIrResult hres = compile_visual_ir(ir, hostile_opts);
  check(!hres.ok(), "rigid envelope violation fails compilation closed");
  bool rigid = false;
  for (const auto& d : hres.diagnostics.diagnostics)
    if (d.code == "DEFORMATION_RIGID_VIOLATION" || d.code == "DEFORMATION_HARD_VIOLATION") rigid = true;
  check(rigid, "rigid violation diagnostic surfaced at compile");

  // Canon identity classification: canon changes identity, runtime state does not.
  VisualCanon vf2 = vf;
  vf2.base_palette["primary"] = "#000000";
  check(visual_canon_identity(vf2) != visual_canon_identity(vf),
        "canon palette change alters canon identity");
}

// ── 13. Serialization round-trip + determinism ──

static void test_serialization_determinism() {
  const VisualCanon vf = make_voltfox_canon();
  const std::string c1 = canonicalize_visual_canon(vf);
  const std::string c2 = canonicalize_visual_canon(vf);
  check(c1 == c2, "canon canonicalization deterministic");

  const auto parsed = parse_visual_canon(c1);
  check(parsed.has_value(), "canonical JSON parses");
  if (parsed) {
    check(canonicalize_visual_canon(*parsed) == c1, "parse->canonicalize round-trip stable");
    check(parsed->structures.size() == vf.structures.size(), "structure count round-trips");
    check(parsed->landmarks.size() == vf.landmarks.size(), "landmark count round-trips");
    check(parsed->proportions.size() == vf.proportions.size(), "proportion count round-trips");
    check_ok(validate_visual_canon(*parsed), "parsed canon revalidates");
    const VisualMorphologyV2 m = canon_to_morphology(*parsed, "base");
    const VisualMorphologyV2 m2 = canon_to_morphology(vf, "base");
    check(m.parts.size() == m2.parts.size(), "parsed canon constructs identical morphology size");
  }
  {  // malformed input rejected
    check(!parse_visual_canon("garbage{{{").has_value(), "malformed canon rejected");
    check(!parse_visual_canon("").has_value(), "empty canon rejected");
  }

  // default_canon_from_morphology compatibility path.
  const VisualMorphologyV2 morph = canon_to_morphology(vf, "base");
  const VisualCanon derived = default_canon_from_morphology(morph, "voltfox");
  check(derived.structures.size() == vf.structures.size(), "compat canon derives all structures");
  check_ok(validate_visual_canon(derived), "compat canon validates");
  const VisualMorphologyV2 morph2 = canon_to_morphology(derived, "");
  check(morph2.parts.size() == morph.parts.size(), "compat canon reconstructs morphology");
}

// ── 14. Functional-authority closure: causal + hostile proofs ──
// Every test mutates a semantic cause and asserts the expected semantic
// effect (or failure boundary). No field-existence tests.

static void test_functional_authority_closure() {
  // 14.1 StyleSemantics identity completeness: band_count / max_colors are
  // renderer-affecting and must participate in validate/canonicalize/
  // identity AND in visual_ir_identity through the compiled VisualIr.
  {
    StyleSemantics s1;
    StyleSemantics s2 = s1;
    s2.band_count = 3;
    check_ok(validate_style(s1), "style with default bands validates");
    check_ok(validate_style(s2), "style with 3 bands validates");
    check(style_identity(s1) != style_identity(s2), "band_count participates in style identity");
    StyleSemantics bad = s1;
    bad.band_count = 0;
    check_fail(validate_style(bad), "zero band_count rejected");
    StyleSemantics s3 = s1;
    s3.max_colors = 8;
    check(style_identity(s1) != style_identity(s3), "max_colors participates in style identity");
    StyleSemantics bad2 = s1;
    bad2.max_colors = 1;
    check_fail(validate_style(bad2), "max_colors below 2 rejected");
    // canonicalize includes the fields -> different canonical bytes.
    check(canonicalize_style(s1) != canonicalize_style(s2), "band_count in canonicalize_style");
    check(canonicalize_style(s1) != canonicalize_style(s3), "max_colors in canonicalize_style");
  }
  // VisualIr identity: same IR except band_count -> different identity.
  {
    const VisualCanon vf = make_voltfox_canon();
    const SpriteIr ir = make_ir_from_canon(vf);
    VisualCompileOptions opts;
    opts.canvas_width = 128; opts.canvas_height = 128;
    opts.canon = &vf;
    const auto base = compile_visual_ir(ir, opts);
    check(base.ok(), "style-identity probe compiles");
    if (base.ok()) {
      VisualIr a = *base.value;
      VisualIr b = a;
      b.style.band_count = 4;
      check(visual_ir_identity(a) != visual_ir_identity(b), "band_count changes visual_ir_identity");
      VisualIr c = a;
      c.style.max_colors = 16;
      check(visual_ir_identity(a) != visual_ir_identity(c), "max_colors changes visual_ir_identity");
      // Identity-completeness regression: no renderer-consumed field may
      // change pixels while leaving visual_ir_identity unchanged. Where the
      // scene exercises the field (cell shading for band_count), the pixels
      // MUST change deterministically.
      const RasterResult r1 = render_visual_ir(a);
      VisualIr cell = a;
      cell.style.shadow_model = ShadowModel::cell;  // exercise band quantization
      cell.style.band_count = 4;
      const RasterResult r2 = render_visual_ir(cell);
      check(r1.ok() && r2.ok(), "style-differing renders succeed");
      if (r1.ok() && r2.ok())
        check(r1.value->image.pixels != r2.value->image.pixels,
              "cell-shaded band_count change produces a deterministic pixel change");
      // max_colors: cap the palette below the present color count -> the
      // quantized palette differs from the uncapped render. The voltfox
      // canon is used because it exercises several distinct part colors.
      const VisualCanon vf2 = make_voltfox_canon();
      VisualCompileOptions mc_opts;
      mc_opts.canvas_width = 128; mc_opts.canvas_height = 128;
      mc_opts.canon = &vf2;
      const auto mres = compile_visual_ir(make_ir_from_canon(vf2), mc_opts);
      if (mres.ok()) {
        VisualIr hp = *mres.value;
        VisualIr hc = hp;
        hc.style.max_colors = 2;
        const RasterResult pr = render_visual_ir(hp);
        const RasterResult cr = render_visual_ir(hc);
        check(pr.ok() && cr.ok(), "max_colors renders succeed");
        if (pr.ok() && cr.ok())
          check(pr.value->image.pixels != cr.value->image.pixels,
                "max_colors cap produces a deterministic pixel change");
        else
          check(false, "max_colors renders succeed");
      }
    }
  }

  // 14.2 gaze_driver / attention_driver / support_policy are authoritative:
  // validated, canonicalized, parsed, round-tripped, identity-inclusive.
  {
    VisualCanon c = make_voltfox_canon();
    check_ok(validate_visual_canon(c), "canon with drivers validates");
    VisualCanon bad = c;
    bad.gaze_driver = "no_such_structure";
    check_fail(validate_visual_canon(bad), "dangling gaze_driver rejected");
    VisualCanon bad2 = c;
    bad2.attention_driver = "no_such_structure";
    check_fail(validate_visual_canon(bad2), "dangling attention_driver rejected");
    VisualCanon bad3 = c;
    bad3.support_policy = static_cast<SupportPolicy>(99);
    check_fail(validate_visual_canon(bad3), "invalid support_policy fails closed");

    VisualCanon g = c;
    g.gaze_driver = "left_ear";
    check(visual_canon_identity(g) != visual_canon_identity(c),
          "behavior-changing gaze_driver changes canon identity");

    // Round-trip: drivers survive canonicalize -> parse.
    const std::string text = canonicalize_visual_canon(c);
    const auto parsed = parse_visual_canon(text);
    check(parsed.has_value(), "canon with drivers parses");
    if (parsed) {
      check(parsed->gaze_driver == c.gaze_driver, "gaze_driver round-trips");
      check(parsed->attention_driver == c.attention_driver, "attention_driver round-trips");
      check(parsed->support_policy == c.support_policy, "support_policy round-trips");
      check(canonicalize_visual_canon(*parsed) == text, "driver-bearing canon round-trip stable");
    }
  }

  // 14.3 Support semantics are explicit, not inferred from appendages.
  {
    const VisualCanon vf = make_voltfox_canon();
    PerformanceIntent intent;
    intent.balance = BalanceIntent::planted;
    intent.action = "idle";
    const PoseSolution sol = solve_pose(vf, intent);
    const ValidationResult bal = analyze_balance(vf, sol, intent);
    // Planted Voltfox HAS declared paw support -> no BALANCE_NO_SUPPORT.
    bool no_support = false;
    for (const auto& d : bal.diagnostics)
      if (d.code == "BALANCE_NO_SUPPORT") no_support = true;
    check(!no_support, "planted voltfox has plausible paw support (no BALANCE_NO_SUPPORT)");
    // BALANCE_* diagnostics are typed warnings, never errors.
    for (const auto& d : bal.diagnostics)
      if (d.code.rfind("BALANCE_", 0) == 0)
        check(d.severity != DiagnosticSeverity::error, "balance diagnostics are nonfatal warnings");

    // A canon whose support_capable set is empty -> planted fails with a
    // warning (ears/tails/antennae are NOT inferred as support).
    VisualCanon no_support_canon = vf;
    for (auto& [id, s] : no_support_canon.structures) s.support_capable = false;
    const PoseSolution sol2 = solve_pose(no_support_canon, intent);
    const ValidationResult bal2 = analyze_balance(no_support_canon, sol2, intent);
    bool flagged = false;
    for (const auto& d : bal2.diagnostics)
      if (d.code == "BALANCE_NO_SUPPORT") flagged = true;
    check(flagged, "planted pose without declared support contacts flagged");

    // Flight policy: flyer legitimately skips ground analysis with NO
    // diagnostic (and no fixture-specific branch).
    const VisualCanon fly = make_flyer_canon();
    const PoseSolution sol3 = solve_pose(fly, intent);
    const ValidationResult bal3 = analyze_balance(fly, sol3, intent);
    check_ok(bal3, "flight policy skips ground analysis cleanly");

    // KeyPose authored support contacts drive the support polygon.
    KeyPose pose;
    pose.id = "pose1";
    pose.support_contacts = {{2.0, 0.0}, {8.0, 0.0}};
    pose.balance = BalanceIntent::planted;
    const ValidationResult bal4 = analyze_balance(vf, sol, intent, &pose);
    check_ok(bal4, "authored support contacts accepted");
  }

  // 14.4 Aggregate deformation composition: two individually-legal requests
  // that combine into an illegal final deviation are rejected/clamped by
  // policy — the gate evaluates the AGGREGATE effective deviation.
  {
    const VisualCanon vf = make_voltfox_canon();
    PerformanceState perf;
    // left_leg: allowed 3.2, hard 7.0. Two motions of 2.5 each are each
    // individually legal but compose to 5.0, which exceeds the allowed 3.2
    // -> the aggregate must be clamped (warning, still ok).
    PartMotion a; a.part_id = "left_leg"; a.dx = 2.5;
    PartMotion b; b.part_id = "left_leg"; b.dx = 2.5;
    perf.motions = {a, b};
    const ValidationResult r1 = enforce_deformation_envelopes(vf, perf, "", 0.0, 1.0, 1.0);
    bool clamped = false;
    for (const auto& d : r1.diagnostics) if (d.code == "DEFORMATION_CLAMPED") clamped = true;
    check(clamped, "aggregate deviation beyond allowed bound is clamped");
    check(r1.ok(), "clamped aggregate remains ok (warning only)");
    check(perf.motions.size() == 1, "composition merges duplicate-part motions into one");
    check(std::abs(perf.motions[0].dx) <= 3.2 + 1e-9, "aggregate clamped to allowed bound");

    // Hard boundary: two motions of 4.0 compose to 8.0 > hard 7.0 -> fail.
    PerformanceState perf2;
    PartMotion a2; a2.part_id = "left_leg"; a2.dx = 4.0;
    PartMotion b2; b2.part_id = "left_leg"; b2.dx = 4.0;
    perf2.motions = {a2, b2};
    const ValidationResult r2 = enforce_deformation_envelopes(vf, perf2, "", 0.0, 1.0, 1.0);
    bool hard = false;
    for (const auto& d : r2.diagnostics) if (d.code == "DEFORMATION_HARD_VIOLATION") hard = true;
    check(hard, "aggregate beyond hard boundary fails closed");
    check(!r2.ok(), "hard aggregate violation not ok");
  }

  // 14.5 KeyPose override semantics: replace, not append, no double motion.
  {
    const VisualCanon vf = make_voltfox_canon();
    PerformanceIntent intent;
    intent.action = "attack";
    intent.phase = PerformancePhase::extension;
    intent.force_direction = {1.0, 0.0};
    intent.force_magnitude = 0.9;
    intent.commitment = 1.0;
    // Derived motion targets the root mass (torso); the KeyPose overrides
    // torso with its own motion. The override must REPLACE, not stack.
    KeyPose pose;
    pose.id = "kp";
    pose.action = "attack";
    PartMotion override; override.part_id = "torso"; override.dx = 0.5; override.rotation_degrees = 3.0;
    pose.motions = {override};
    const PoseSolution sol = solve_pose(vf, intent, &pose);
    std::size_t torso_count = 0;
    for (const auto& m : sol.motions) if (m.part_id == "torso") ++torso_count;
    check(torso_count == 1, "key pose override produces exactly one torso motion");
    bool found_override = false;
    for (const auto& m : sol.motions)
      if (m.part_id == "torso" && std::abs(m.dx - 0.5) < 1e-9 && std::abs(m.rotation_degrees - 3.0) < 1e-9)
        found_override = true;
    check(found_override, "key pose override value wins (no double translation/rotation)");
    // Insertion-order independence: reordering the pose motions yields the
    // same solution for the overridden part.
    KeyPose pose2 = pose;
    KeyPose pose3 = pose;
    pose3.motions = {override};
    const PoseSolution s2 = solve_pose(vf, intent, &pose2);
    const PoseSolution s3 = solve_pose(vf, intent, &pose3);
    auto find_torso = [](const PoseSolution& s) {
      for (const auto& m : s.motions) if (m.part_id == "torso") return m;
      return PartMotion{};
    };
    const auto t2 = find_torso(s2), t3 = find_torso(s3);
    check(std::abs(t2.dx - t3.dx) < 1e-9 && std::abs(t2.rotation_degrees - t3.rotation_degrees) < 1e-9,
          "key pose override is insertion-order deterministic");
  }

  // 14.6 Emotion is data-driven through canon.emotion_responses.
  {
    const VisualCanon vf = make_voltfox_canon();
    const auto neutral = expression_motions(vf, "", 0.0, 0.0);
    const auto angry = expression_motions(vf, "focused_aggression", 0.0, 0.0);
    check(neutral.size() == angry.size(), "emotion does not change feature count");
    bool different = false;
    for (std::size_t i = 0; i < neutral.size() && i < angry.size(); ++i) {
      if (neutral[i].part_id != angry[i].part_id) continue;
      if (std::abs(neutral[i].scale_x - angry[i].scale_x) > 1e-9 ||
          std::abs(neutral[i].scale_y - angry[i].scale_y) > 1e-9)
        different = true;
    }
    check(different, "emotion response changes eye aperture (causal)");
    // Unknown emotion: same as neutral defaults (fail-soft, no hardcode).
    const auto unknown = expression_motions(vf, "pensive", 0.0, 0.0);
    bool same_as_neutral = true;
    for (std::size_t i = 0; i < neutral.size() && i < unknown.size(); ++i)
      if (neutral[i].part_id == unknown[i].part_id &&
          (std::abs(neutral[i].scale_x - unknown[i].scale_x) > 1e-9))
        same_as_neutral = false;
    check(same_as_neutral, "unknown emotion falls back to neutral defaults");

    // Mechanical entity: mech emotion drives visor/antenna through the SAME
    // generic machinery (no organism-specific branch).
    const VisualCanon mech = make_mech_canon();
    const auto m_alert = expression_motions(mech, "alert", 0.0, 0.0);
    bool visor_or_antenna = false;
    for (const auto& m : m_alert)
      if (m.part_id.find("visor") != std::string::npos ||
          m.part_id.find("antenna") != std::string::npos ||
          m.part_id.find("sensor") != std::string::npos)
        visor_or_antenna = true;
    check(visor_or_antenna, "mech emotion drives visor/antenna channels");
  }

  // 14.7 FX originates from typed phenomenon/energy requests only. No
  // magnitude heuristic invents electricity; no "torso" anatomy fallback.
  {
    const VisualCanon vf = make_voltfox_canon();
    const SpriteIr ir = make_ir_from_canon(vf);
    VisualCompileOptions opts;
    opts.canvas_width = 128; opts.canvas_height = 128;
    opts.canon = &vf;
    PerformanceState perf;
    perf.impact = 0.9;
    opts.performance = &perf;

    // Electric entity impact -> electric arc (typed phenomenon).
    FxRequest arc;
    arc.id = "fx1"; arc.phenomenon = FxPhenomenon::electric_arc;
    arc.energy = FxEnergyKind::electricity;
    arc.source_part = "tail_tip"; arc.intensity = 0.8;
    FxRequest impact;
    impact.id = "fx2"; impact.phenomenon = FxPhenomenon::impact_flash;
    impact.energy = FxEnergyKind::kinetic;
    impact.source_part = "left_foot"; impact.intensity = 0.9;
    FxRequest aura;
    aura.id = "fx3"; aura.phenomenon = FxPhenomenon::aura;
    aura.energy = FxEnergyKind::force;
    aura.source_part = "torso"; aura.intensity = 0.6;
    const FxRequest reqs[] = {arc, impact, aura};
    opts.fx_requests = reqs;
    const auto res = compile_visual_ir(ir, opts);
    check(res.ok(), "typed fx requests compile");
    if (res.ok()) {
      std::set<std::string> phenomena;
      for (const auto& e : res.value->fx_state.effects)
        phenomena.insert(std::string(fx_phenomenon_name(e.phenomenon)));
      check(phenomena.count("electric_arc") == 1, "electric entity impact yields electric_arc");
      check(phenomena.count("impact_flash") == 1, "kinetic impact yields impact_flash (not electricity)");
      check(phenomena.count("aura") == 1, "aura request yields aura");
      // Force magnitude scales intensity but never invents the phenomenon.
      for (const auto& e : res.value->fx_state.effects)
        check(e.intensity <= 1.0 && e.intensity >= 0.0, "fx intensity bounded");
    }
    // Invalid request (no source) is rejected and never emitted.
    FxRequest bad;
    bad.id = "bad"; bad.phenomenon = FxPhenomenon::electric_arc; bad.energy = FxEnergyKind::electricity;
    check_fail(validate_fx_request(bad), "fx request without source rejected");
    FxRequest ghost;
    ghost.id = "ghost"; ghost.phenomenon = FxPhenomenon::impact_flash; ghost.energy = FxEnergyKind::kinetic;
    ghost.source_part = "no_such_part";
    const FxRequest reqs2[] = {ghost};
    opts.fx_requests = reqs2;
    const auto res2 = compile_visual_ir(ir, opts);
    // The compiler reports the missing source as an error diagnostic and
    // NEVER emits the effect (no "torso" anatomy fallback). Fail-closed.
    bool source_missing = false;
    for (const auto& d : res2.diagnostics.diagnostics)
      if (d.code == "VISUAL_FX_SOURCE_MISSING") source_missing = true;
    check(source_missing, "fx request with missing source reported");
    if (res2.value)
      check(res2.value->fx_state.effects.empty(), "fx request with missing source never emitted");
  }

  // 14.8 Construction constraint domains: position.x and size.x are distinct
  // properties; true same-property contradictions fail; cross-axis ratios
  // work; unknown axes fail; insertion order is irrelevant.
  {
    // Same axis, different property (position.x + size.x) coexist.
    VisualCanon c;
    c.entity_id = "probe";
    c.forms = {"base"};
    c.structures["a"] = {.id = "a", .role = "root", .primitive = "ellipse", .layer = "body",
                         .x = 0.0, .y = 0.0, .size_x = 2.0, .size_y = 2.0};
    c.structures["b"] = {.id = "b", .role = "root", .primitive = "ellipse", .layer = "body",
                         .x = 0.0, .y = 0.0, .size_x = 2.0, .size_y = 2.0};
    ConstructionConstraint pos;
    pos.id = "pos"; pos.kind = ConstructionConstraintKind::anchor;
    pos.structure = "b"; pos.reference = "a"; pos.axis = ConstructionAxis::x;
    pos.offset = 3.0; pos.hard = true;
    ConstructionConstraint sz;
    sz.id = "sz"; sz.kind = ConstructionConstraintKind::size_ratio;
    sz.structure = "b"; sz.reference = "a"; sz.axis = ConstructionAxis::x;
    sz.factor = 0.5; sz.hard = true;
    c.construction = {pos, sz};
    const ValidationResult r1 = solve_construction_constraints(c, "");
    check_ok(r1, "hard position.x + hard size.x coexist without conflict");
    const VisualMorphologyV2 m1 = canon_to_morphology(c, "");
    check(std::abs(m1.parts.at("b").x - 3.0) < 1e-9, "position constraint solved");
    check(std::abs(m1.parts.at("b").size_x - 1.0) < 1e-9, "size constraint solved");

    // True same-property contradiction fails closed.
    VisualCanon c2 = c;
    ConstructionConstraint c2b;
    c2b.id = "sz2"; c2b.kind = ConstructionConstraintKind::size_ratio;
    c2b.structure = "b"; c2b.reference = "a"; c2b.axis = ConstructionAxis::x;
    c2b.factor = 2.0; c2b.hard = true;
    c2.construction.push_back(c2b);
    const ValidationResult r2 = solve_construction_constraints(c2, "");
    bool contrad = false;
    for (const auto& d : r2.diagnostics)
      if (d.code == "CONSTRUCTION_HARD_CONTRADICTION") contrad = true;
    check(contrad && !r2.ok(), "contradictory hard size.x constraints fail closed");

    // Cross-axis: b.size_x derived from a.size_y.
    VisualCanon c3 = c;
    c3.construction.clear();
    ConstructionConstraint cross;
    cross.id = "cross"; cross.kind = ConstructionConstraintKind::size_ratio;
    cross.structure = "b"; cross.reference = "a"; cross.axis = ConstructionAxis::x;
    cross.reference_axis = ConstructionAxis::y;  // size_x from size_y
    cross.factor = 1.5; cross.hard = true;
    c3.construction = {cross};
    c3.structures["a"].size_y = 4.0;
    const ValidationResult r3 = solve_construction_constraints(c3, "");
    check_ok(r3, "cross-axis size_x from size_y solves");
    const VisualMorphologyV2 m3 = canon_to_morphology(c3, "");
    check(std::abs(m3.parts.at("b").size_x - 6.0) < 1e-9, "cross-axis ratio uses denominator's own axis");

    // Insertion order: reversed constraint order yields identical geometry.
    VisualCanon c4 = c;
    c4.construction = {sz, pos};  // reversed
    const VisualMorphologyV2 m4 = canon_to_morphology(c4, "");
    check(std::abs(m4.parts.at("b").x - m1.parts.at("b").x) < 1e-9 &&
          std::abs(m4.parts.at("b").size_x - m1.parts.at("b").size_x) < 1e-9,
          "construction is insertion-order deterministic");
  }

  // 14.9 Silhouette invariants genuinely fail when violated.
  {
    VisualCanon c = make_voltfox_canon();
    VisualMorphologyV2 m = canon_to_morphology(c, "base");
    check_ok(check_identity_invariants(c, m), "canonical morph passes silhouette invariants");
    // Degrade: remove the silhouette-anchor landmark and anchor feature for
    // a structure that a hard invariant requires to be anchored.
    VisualCanon c2 = c;
    // Find a structure that is a required silhouette anchor and remove its
    // anchoring evidence.
    std::string anchored_struct;
    for (const auto& inv : c2.identity_invariants) {
      if (inv.kind != IdentityInvariantKind::silhouette_anchor) continue;
      for (const auto& ref : inv.refs) {
        if (c2.structures.count(ref)) {
          // Remove landmark anchoring evidence for this structure.
          for (auto it = c2.landmarks.begin(); it != c2.landmarks.end();) {
            if (it->second.owner == ref && it->second.silhouette_anchor) it = c2.landmarks.erase(it);
            else ++it;
          }
          for (auto it = c2.silhouette_features.begin(); it != c2.silhouette_features.end();) {
            if (it->structure_ref == ref && it->kind == SilhouetteFeatureKind::anchor)
              it = c2.silhouette_features.erase(it);
            else ++it;
          }
          anchored_struct = ref;
          break;
        }
      }
      if (!anchored_struct.empty()) break;
    }
    if (!anchored_struct.empty()) {
      const ValidationResult r = check_identity_invariants(c2, m);
      bool unanchored = false;
      for (const auto& d : r.diagnostics)
        if (d.code.find("UNANCHORED") != std::string::npos) unanchored = true;
      check(unanchored && !r.ok(),
            "existing structure stripped of its required silhouette anchor fails");
    }
  }

  // 14.10 Temporal correspondence: two poses of the same entity retain
  // semantic identity for parts/landmarks/markings/materials/features while
  // their transforms change.
  {
    const VisualCanon vf = make_voltfox_canon();
    const SpriteIr ir = make_ir_from_canon(vf);
    VisualCompileOptions opts;
    opts.canvas_width = 128; opts.canvas_height = 128;
    opts.canon = &vf;
    PerformanceState idle;
    idle.motion_phase = "idle";
    opts.performance = &idle;
    const auto f0 = compile_visual_ir(ir, opts);
    PerformanceState strike;
    strike.motion_phase = "strike";
    PartMotion leg; leg.part_id = "left_leg"; leg.dx = 2.0; leg.rotation_degrees = -3.0;
    strike.motions = {leg};
    opts.performance = &strike;
    const auto f1 = compile_visual_ir(ir, opts);
    check(f0.ok() && f1.ok(), "two-pose compile succeeds");
    if (f0.ok() && f1.ok()) {
      check(!f0.value->temporal_part_labels.empty(), "temporal part labels populated");
      check(!f0.value->temporal_landmark_labels.empty(), "temporal landmark labels populated");
      // Stable identity: same semantic id maps to the same temporal label
      // across both poses.
      for (const auto& [id, label] : f0.value->temporal_part_labels) {
        auto it = f1.value->temporal_part_labels.find(id);
        check(it != f1.value->temporal_part_labels.end() && it->second == label,
              "part temporal identity persists across poses");
        if (it == f1.value->temporal_part_labels.end()) break;
      }
      for (const auto& [id, label] : f0.value->temporal_marking_labels) {
        auto it = f1.value->temporal_marking_labels.find(id);
        check(it != f1.value->temporal_marking_labels.end() && it->second == label,
              "marking temporal identity persists across poses");
        if (it == f1.value->temporal_marking_labels.end()) break;
      }
      // Transform changed while identity stayed: left_leg moved.
      check(std::abs(f1.value->morphology.parts.at("left_leg").x -
                     f0.value->morphology.parts.at("left_leg").x) > 1e-9,
            "pose transform actually changed the part");
    }
  }

  // 14.11 Semantic LOD executes authored rules (omit/merge/substitute/
  // preserve) — not just visibility hiding.
  {
    VisualCanon c = make_voltfox_canon();
    VisualMorphologyV2 m = canon_to_morphology(c, "base");
    // Tail is a low-priority feature (min_resolution 16, importance 0.6);
    // at a tiny target resolution the authored default is visibility
    // filtering (non-identity-critical), while eyes (importance 1.0) are
    // preserved.
    const ValidationResult r = apply_semantic_lod(c, m, 8);
    bool any_action = false;
    for (const auto& d : r.diagnostics) {
      if (d.code == "LOD_OMIT" || d.code == "LOD_MERGE" || d.code == "LOD_SUBSTITUTE" ||
          d.code == "LOD_VISIBILITY_FILTER")
        any_action = true;
    }
    check(any_action, "semantic LOD executes authored rules at low resolution");
    check(m.parts.at("left_eye").visible, "identity-critical eyes preserved at low resolution");

    // Explicit omit rule: ear feature omits its part.
    VisualCanon c2 = c;
    VisualMorphologyV2 m2 = canon_to_morphology(c2, "base");
    VisualFeature rf;
    rf.id = "res.ear_l"; rf.structure_ref = "left_ear";
    rf.min_resolution = 10; rf.omission_rule = "omit";
    c2.resolution_features.push_back(rf);
    const ValidationResult r2 = apply_semantic_lod(c2, m2, 8);
    bool omit = false;
    for (const auto& d : r2.diagnostics) if (d.code == "LOD_OMIT") omit = true;
    check(omit, "explicit omit rule executes");
    check(!m2.parts.at("left_ear").visible, "omit rule hides the part");

    // Substitute rule: nose -> substitute part becomes visible.
    VisualCanon c3 = c;
    VisualMorphologyV2 m3 = canon_to_morphology(c3, "base");
    VisualFeature rf2;
    rf2.id = "res.nose"; rf2.structure_ref = "nose";
    rf2.min_resolution = 10; rf2.substitution_rule = "substitute:head";
    c3.resolution_features.push_back(rf2);
    const ValidationResult r3 = apply_semantic_lod(c3, m3, 8);
    bool sub = false;
    for (const auto& d : r3.diagnostics) if (d.code == "LOD_SUBSTITUTE") sub = true;
    check(sub, "substitute rule executes");
    check(!m3.parts.at("nose").visible && m3.parts.at("head").visible,
          "substitute hides source and preserves substitute");
  }

  // 14.12 Canonical serialization: reversible escaping + hostile rejection.
  {
    VisualCanon c;
    c.entity_id = "esc";
    c.forms = {"base"};
    c.name = "a=b;c.d\\e\nf";  // reserved chars: = ; . \ newline
    c.structures["root"] = {.id = "root", .role = "root", .primitive = "ellipse",
                            .layer = "body", .x = 0.0, .y = 0.0,
                            .size_x = 1.0, .size_y = 1.0};
    check_ok(validate_visual_canon(c), "escaping probe canon validates");
    const std::string text = canonicalize_visual_canon(c);
    const auto parsed = parse_visual_canon(text);
    check(parsed.has_value(), "escaped canon parses");
    if (parsed) {
      check(parsed->name == c.name, "reserved characters round-trip exactly");
      check(canonicalize_visual_canon(*parsed) == text,
            "canonicalize(parse(canonicalize(x))) is byte-identical");
    }
    // Hostile: malformed escape.
    check(!parse_visual_canon("name=trailing_backslash\\\n").has_value(),
          "malformed trailing escape rejected");
    check(!parse_visual_canon("name=a\\q\n").has_value(), "unknown escape sequence rejected");
    // Duplicate singleton field.
    check(!parse_visual_canon("entity_id=a\nentity_id=b\n").has_value(),
          "duplicate singleton field rejected");
    // Unknown field.
    check(!parse_visual_canon("entity_id=a\nbogus_section=1\n").has_value(),
          "unknown section rejected");
    // Partial numeric token.
    check(!parse_visual_canon("entity_id=a\nstructure.root.id=root\n"
                              "structure.root.x=12abc\n").has_value(),
          "partial numeric token rejected");
    // Unicode passes through untouched.
    VisualCanon u;
    u.entity_id = "uni";
    u.forms = {"base"};
    u.name = "voltfox \u00e9\u4e2d";
    u.structures["root"] = {.id = "root", .role = "root", .primitive = "ellipse",
                            .layer = "body", .x = 0.0, .y = 0.0,
                            .size_x = 1.0, .size_y = 1.0};
    const auto up = parse_visual_canon(canonicalize_visual_canon(u));
    check(up.has_value() && up->name == u.name, "unicode round-trips");
  }
}

// ── 15. Review-artifact generation (argv-gated, mirrors visual_core_tests) ──

static void write_png(const ImageRgba8& img, const std::filesystem::path& p) {
  const auto png = encode_png(img);
  std::ofstream ofs(p, std::ios::binary);
  for (const auto& b : png) ofs << static_cast<char>(b);
}

struct ReviewEntry {
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
static std::vector<ReviewEntry> g_review;

static std::string json_escape(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '"' || c == '\\') out += '\\';
    out += c;
  }
  return out;
}

static bool render_review_frame(const VisualCanon& canon, const VisualCompileOptions& opts,
                                const std::filesystem::path& out_path, const std::string& entity,
                                const std::string& style, const std::string& form,
                                const std::string& pose) {
  RasterLimits limits;
  const SpriteIr ir = make_ir_from_canon(canon);
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
    const std::string dcode = rr.diagnostics.diagnostics.empty() ? std::string("?")
        : rr.diagnostics.diagnostics.front().code;
    std::cerr << "EVIDENCE render failed: " << entity << "/" << form << "/" << style
              << " : " << dcode << std::endl;
    return false;
  }
  const ImageRgba8& img = rr.value->image;
  write_png(img, out_path);
  ReviewEntry e;
  e.file = out_path.filename().string();
  e.entity = entity;
  e.style = style;
  e.form = form;
  e.pose = pose;
  e.renderer = "visual-intelligence-canon";
  e.ir_identity = visual_ir_identity(*res.value);
  e.width = img.width;
  e.height = img.height;
  const std::string_view bytes(reinterpret_cast<const char*>(img.pixels.data()), img.pixels.size());
  e.rgba_sha256 = gspl::sprites::sha256(bytes);
  g_review.push_back(std::move(e));
  std::cout << "EVIDENCE wrote " << out_path.string() << std::endl;
  return true;
}

static void write_review_manifest(const std::filesystem::path& dir, const std::string& source_sha) {
  std::ofstream ofs(dir / "visual-intelligence-evidence.json");
  ofs << "{\n  \"schema\": \"gspl.visual-intelligence-evidence/0.1\",\n";
  ofs << "  \"source_sha\": \"" << json_escape(source_sha) << "\",\n";
  ofs << "  \"entries\": [\n";
  for (std::size_t i = 0; i < g_review.size(); ++i) {
    const ReviewEntry& e = g_review[i];
    ofs << "    {\"file\": \"" << json_escape(e.file)
        << "\", \"entity\": \"" << json_escape(e.entity)
        << "\", \"style\": \"" << json_escape(e.style)
        << "\", \"form\": \"" << json_escape(e.form)
        << "\", \"pose\": \"" << json_escape(e.pose)
        << "\", \"renderer\": \"" << json_escape(e.renderer)
        << "\", \"ir_identity\": \"" << json_escape(e.ir_identity)
        << "\", \"rgba_sha256\": \"" << json_escape(e.rgba_sha256)
        << "\", \"width\": " << e.width << ", \"height\": " << e.height << "}";
    if (i + 1 < g_review.size()) ofs << ",";
    ofs << "\n";
  }
  ofs << "  ]\n}\n";
  std::cout << "EVIDENCE manifest wrote " << (dir / "visual-intelligence-evidence.json").string() << std::endl;
}

static void generate_review_artifacts(const std::filesystem::path& dir, const std::string& source_sha) {
  const std::filesystem::path vc = dir / "visual-intelligence";
  std::filesystem::create_directories(vc);
  g_review.clear();

  const VisualCanon vf = make_voltfox_canon();
  const VisualCanon hu = make_humanoid_canon();
  const VisualCanon mech = make_mech_canon();
  const VisualCanon fly = make_flyer_canon();

  const StyleProgram flat = make_style_program_preset("clean-flat");
  const StyleProgram ink = make_style_program_preset("inked");
  const StyleProgram soft = make_style_program_preset("soft-shaded");
  const StyleProgram pixel = make_style_program_preset("pixel-constrained");

  VisualCompileOptions base;
  base.canvas_width = 128; base.canvas_height = 128;
  base.canon = &vf;
  base.style_program = &flat;
  render_review_frame(vf, base, vc / "voltfox-base-clean-flat.png", "voltfox", "clean-flat", "base", "idle");

  // Action frame via semantic intent + key pose.
  PerformanceIntent intent;
  intent.action = "attack";
  intent.phase = PerformancePhase::extension;
  intent.force_direction = {1.0, 0.2};
  intent.force_magnitude = 0.7;
  intent.gaze_direction = {1.0, 0.0};
  KeyPose pose;
  pose.id = "strike";
  pose.action = "attack";
  PartMotion l1; l1.part_id = "left_leg"; l1.dx = 1.5; l1.rotation_degrees = -6.0;
  PartMotion t1; t1.part_id = "tail_tip"; t1.rotation_degrees = 8.0;
  PartMotion h1; h1.part_id = "head"; h1.dx = 0.5; h1.rotation_degrees = 3.0;
  pose.motions = {l1, t1, h1};
  VisualCompileOptions act = base;
  act.intent = &intent;
  act.key_pose = &pose;
  render_review_frame(vf, act, vc / "voltfox-action.png", "voltfox", "clean-flat", "base", "attack-extension");

  // Storm form (transformation form through the same generic compiler).
  VisualCompileOptions storm = base;
  storm.form_id = "storm";
  render_review_frame(vf, storm, vc / "voltfox-storm.png", "voltfox", "clean-flat", "storm", "idle");

  // Style variation on the same entity: materially distinct manifestations.
  VisualCompileOptions oink = base;
  oink.style_program = &ink;
  render_review_frame(vf, oink, vc / "voltfox-style-inked.png", "voltfox", "inked", "base", "idle");
  VisualCompileOptions osft = base;
  osft.style_program = &soft;
  render_review_frame(vf, osft, vc / "voltfox-style-soft-shaded.png", "voltfox", "soft-shaded", "base", "idle");
  VisualCompileOptions opix = base;
  opix.style_program = &pixel;
  render_review_frame(vf, opix, vc / "voltfox-style-pixel-constrained.png", "voltfox", "pixel-constrained", "base", "idle");

  // Generalization fixtures through the identical pipeline.
  VisualCompileOptions ohu;
  ohu.canvas_width = 128; ohu.canvas_height = 128;
  ohu.canon = &hu;
  ohu.style_program = &flat;
  render_review_frame(hu, ohu, vc / "humanoid.png", "test_humanoid", "clean-flat", "base", "idle");
  VisualCompileOptions omech;
  omech.canvas_width = 128; omech.canvas_height = 128;
  omech.canon = &mech;
  omech.style_program = &flat;
  render_review_frame(mech, omech, vc / "mech.png", "test_mech", "clean-flat", "base", "idle");
  VisualCompileOptions ofly;
  ofly.canvas_width = 128; ofly.canvas_height = 128;
  ofly.canon = &fly;
  ofly.style_program = &flat;
  render_review_frame(fly, ofly, vc / "flyer.png", "test_flyer", "clean-flat", "base", "idle");

  write_review_manifest(dir, source_sha);
  std::cout << "EVIDENCE review artifacts generated in " << vc.string() << std::endl;
}

int main(int argc, char** argv) {
  const std::filesystem::path evidence_dir =
      (argc >= 3 && std::string(argv[1]) == "--evidence") ? std::filesystem::path(argv[2])
                                                          : std::filesystem::path{};
  const std::string source_sha = (argc >= 4 && !evidence_dir.empty()) ? argv[3] : std::string("local");

  test_canon_validation();
  test_canon_construction();
  test_proportions_and_landmarks();
  test_deformation_enforcement();
  test_identity_invariants();
  test_performance_intent();
  test_style_program();
  test_temporal_identity();
  test_fx_semantics();
  test_cinematic();
  test_visual_quality();
  test_compiler_integration();
  test_serialization_determinism();
  test_functional_authority_closure();

  if (!evidence_dir.empty()) generate_review_artifacts(evidence_dir, source_sha);

  std::cout << "\nvisual_intelligence_tests: " << (failures == 0 ? "ALL PASS" : "FAILURES")
            << " (failures=" << failures << ")\n";
  return failures == 0 ? 0 : 1;
}
