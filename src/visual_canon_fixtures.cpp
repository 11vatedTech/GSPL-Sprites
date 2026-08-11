#include "gspl_sprites/visual_canon_fixtures.hpp"

namespace gspl::sprites::visual {
namespace {

using namespace std::string_view_literals;

void set_canon(VisualCanon& c, std::string_view entity, std::string_view name,
               std::string_view rights) {
  c.schema = "gspl.visual-canon/0.1";
  c.entity_id = std::string(entity);
  c.name = std::string(name);
  c.rights_class = std::string(rights);
}

CanonStructure st(std::string id, std::string parent, std::string role,
                  std::string primitive, std::string layer, std::string material,
                  std::string color_role, double x, double y, double z,
                  double sx, double sy, double sz, double rot = 0.0, int z_order = 0) {
  CanonStructure s;
  s.id = std::move(id);
  s.kind = StructureKind::element;
  s.parent = std::move(parent);
  s.role = std::move(role);
  s.primitive = std::move(primitive);
  s.layer = std::move(layer);
  s.material_class = std::move(material);
  s.color_role = std::move(color_role);
  s.x = x; s.y = y; s.z = z;
  s.size_x = sx; s.size_y = sy; s.size_z = sz;
  s.rotation_degrees = rot;
  s.z_order = z_order;
  return s;
}

void add_proportion(VisualCanon& c, std::string id, std::string num, std::string den,
                    double preferred, double lo, double hi, double dlo, double dhi,
                    bool hard) {
  ProportionRule p;
  p.id = std::move(id);
  p.numerator = std::move(num);
  p.denominator = std::move(den);
  p.preferred = preferred;
  p.min = lo; p.max = hi;
  p.deformation_min = dlo; p.deformation_max = dhi;
  p.hard = hard;
  c.proportions.push_back(std::move(p));
}

void add_landmark(VisualCanon& c, std::string id, std::string role, std::string owner,
                  double ox, double oy, std::string symmetry = "", bool anchor = false,
                  bool required = false) {
  VisualLandmark lm;
  lm.id = std::move(id);
  lm.role = std::move(role);
  lm.owner = std::move(owner);
  lm.ox = ox; lm.oy = oy;
  lm.symmetry = std::move(symmetry);
  lm.silhouette_anchor = anchor;
  lm.required = required;
  c.landmarks.emplace(lm.id, std::move(lm));
}

void add_envelope(VisualCanon& c, std::string structure, double allowed,
                  double boundary, bool rigid, double action_scale) {
  DeformationEnvelope e;
  e.structure_id = std::move(structure);
  e.allowed_deviation = allowed;
  e.allowed_translation = allowed;
  e.allowed_rotation = allowed;
  e.allowed_scale = allowed;
  e.hard_translation = boundary;
  e.hard_rotation = boundary;
  e.hard_scale = boundary;
  e.rigid = rigid;
  e.action_scale = action_scale;
  c.deformation_envelopes.emplace(e.structure_id, std::move(e));
}

void add_invariant(VisualCanon& c, std::string id, IdentityInvariantKind kind,
                   std::vector<std::string> refs, double tolerance, bool hard,
                   std::string scope = "") {
  IdentityInvariant inv;
  inv.id = std::move(id);
  inv.kind = kind;
  inv.refs = std::move(refs);
  inv.tolerance = tolerance;
  inv.hard = hard;
  inv.severity = hard ? InvariantSeverity::hard : InvariantSeverity::soft;
  inv.scope = std::move(scope);
  c.identity_invariants.push_back(std::move(inv));
}

void add_envelope_typed(VisualCanon& c, std::string structure, double allowed_translation,
                        double allowed_rotation, double allowed_scale,
                        double hard_translation, double hard_rotation, double hard_scale,
                        bool rigid) {
  DeformationEnvelope e;
  e.structure_id = std::move(structure);
  e.allowed_deviation = std::max({allowed_translation, allowed_rotation, allowed_scale});
  e.allowed_translation = allowed_translation;
  e.allowed_rotation = allowed_rotation;
  e.allowed_scale = allowed_scale;
  e.hard_translation = hard_translation;
  e.hard_rotation = hard_rotation;
  e.hard_scale = hard_scale;
  e.rigid = rigid;
  c.deformation_envelopes.emplace(e.structure_id, std::move(e));
}

void add_construction(VisualCanon& c, std::string id, ConstructionConstraintKind kind,
                      std::string structure, std::string reference, ConstructionAxis axis,
                      double factor, double offset, bool hard = true, std::string scope = "") {
  ConstructionConstraint cc;
  cc.id = std::move(id);
  cc.kind = kind;
  cc.structure = std::move(structure);
  cc.reference = std::move(reference);
  cc.axis = axis;
  cc.factor = factor;
  cc.offset = offset;
  cc.hard = hard;
  cc.scope = std::move(scope);
  c.construction.push_back(std::move(cc));
}

} // namespace

VisualCanon make_voltfox_canon() {
  VisualCanon c;
  set_canon(c, "voltfox", "Voltfox", "original_user_creation");
  c.forms = {"base", "storm"};

  c.structures["torso"] = st("torso", "", "body-mass", "ellipse", "body", "fur", "primary",
                             0, 0, 0, 18, 11, 9, 0, 0);
  c.structures["chest"] = st("chest", "torso", "body-mass", "ellipse", "body", "fur", "secondary",
                             0, 5, 1, 12, 8, 6);
  c.structures["head"] = st("head", "torso", "facial-feature", "ellipse", "body", "fur", "primary",
                            0, 9, 2, 8, 7, 7);
  c.structures["muzzle"] = st("muzzle", "head", "facial-feature", "capsule", "facial", "skin",
                              "secondary", 0, 11.5, 0, 3.5, 2.2, 2.2);
  c.structures["nose"] = st("nose", "muzzle", "facial-feature", "ellipse", "facial", "skin",
                            "accent", 0, 13, 1, 1.3, 0.9, 1.2);
  c.structures["left_ear"] = st("left_ear", "head", "appendage", "capsule", "body", "fur",
                                "primary", -3.4, 10.6, 3, 2.6, 4.6, 1.8, -20, 3);
  c.structures["right_ear"] = st("right_ear", "head", "appendage", "capsule", "body", "fur",
                                 "primary", 3.4, 10.6, 3, 2.6, 4.6, 1.8, 20, 3);
  c.structures["inner_ear_left"] = st("inner_ear_left", "left_ear", "facial-feature", "ellipse",
                                      "facial", "fur", "secondary", 0, 0.5, 1, 1.2, 2.2, 1);
  c.structures["inner_ear_right"] = st("inner_ear_right", "right_ear", "facial-feature", "ellipse",
                                       "facial", "fur", "secondary", 0, 0.5, 1, 1.2, 2.2, 1);
  c.structures["left_eye"] = st("left_eye", "head", "eye", "ellipse", "facial", "glass", "eye",
                                -2.2, 10, 5, 1.6, 1.6, 1, 0, 5);
  c.structures["right_eye"] = st("right_eye", "head", "eye", "ellipse", "facial", "glass", "eye",
                                 2.2, 10, 5, 1.6, 1.6, 1, 0, 5);
  c.structures["left_leg"] = st("left_leg", "torso", "limb", "capsule", "body", "fur", "primary",
                                -4.5, -6, 0, 3.2, 8, 3.2, 8);
  c.structures["right_leg"] = st("right_leg", "torso", "limb", "capsule", "body", "fur", "primary",
                                 4.5, -6, 0, 3.2, 8, 3.2, -8);
  c.structures["left_hind"] = st("left_hind", "torso", "limb", "capsule", "body", "fur", "primary",
                                 -6, -7, -1, 3.6, 9, 3.2, -10, -1);
  c.structures["right_hind"] = st("right_hind", "torso", "limb", "capsule", "body", "fur", "primary",
                                  6, -7, -1, 3.6, 9, 3.2, 10, -1);
  c.structures["left_foot"] = st("left_foot", "left_leg", "appendage", "capsule", "body", "fur",
                                 "shadow", 0, -4, 1, 2.2, 1.8, 1.4);
  c.structures["right_foot"] = st("right_foot", "right_leg", "appendage", "capsule", "body", "fur",
                                  "shadow", 0, -4, 1, 2.2, 1.8, 1.4);
  c.structures["left_hind_foot"] = st("left_hind_foot", "left_hind", "appendage", "capsule", "body",
                                      "fur", "shadow", 0, -4.5, 1, 2.4, 1.9, 1.4);
  c.structures["right_hind_foot"] = st("right_hind_foot", "right_hind", "appendage", "capsule",
                                       "body", "fur", "shadow", 0, -4.5, 1, 2.4, 1.9, 1.4);
  c.structures["tail_base"] = st("tail_base", "torso", "appendage", "capsule", "rear_appendages",
                                 "fur", "primary", -8, -0.5, 0, 3, 4, 2.8, -15);
  c.structures["tail_mid"] = st("tail_mid", "tail_base", "appendage", "capsule", "rear_appendages",
                                "fur", "primary", -2.5, -1, 1, 2.8, 3.8, 2.4, -12);
  c.structures["tail_tip"] = st("tail_tip", "tail_mid", "appendage", "capsule", "rear_appendages",
                                "fur", "secondary", -2.5, -1.2, 2, 2.4, 3.6, 2.2, -10);

  // Markings (semantic; geometry constructed deterministically by the canon).
  MarkingBinding m1; m1.id = "torso_stripes"; m1.kind = MarkingKind::stripe;
  m1.structure_ref = "torso"; m1.color_role = "shadow"; m1.opacity = 0.45; m1.scale = 0.8;
  m1.intent = "identity";
  c.markings.push_back(m1);
  MarkingBinding m2; m2.id = "muzzle_spot_left"; m2.kind = MarkingKind::spot;
  m2.structure_ref = "muzzle"; m2.color_role = "secondary"; m2.opacity = 0.85; m2.scale = 0.5;
  c.markings.push_back(m2);
  MarkingBinding m3; m3.id = "muzzle_spot_right"; m3.kind = MarkingKind::spot;
  m3.structure_ref = "muzzle"; m3.color_role = "secondary"; m3.opacity = 0.85; m3.scale = 0.5;
  c.markings.push_back(m3);
  MarkingBinding m4; m4.id = "tail_tip_mark"; m4.kind = MarkingKind::fur_marking;
  m4.structure_ref = "tail_tip"; m4.color_role = "primary"; m4.opacity = 0.5; m4.scale = 0.6;
  c.markings.push_back(m4);
  MarkingBinding m5; m5.id = "storm_circuit"; m5.kind = MarkingKind::circuit;
  m5.structure_ref = "torso"; m5.color_role = "emission"; m5.opacity = 0.85; m5.scale = 1.2;
  m5.form_id = "storm"; m5.intent = "energy";
  c.markings.push_back(m5);
  MarkingBinding m6; m6.id = "storm_arcs"; m6.kind = MarkingKind::electrical;
  m6.structure_ref = "tail_mid"; m6.color_role = "emission"; m6.opacity = 0.8; m6.scale = 1.0;
  m6.form_id = "storm"; m6.intent = "energy";
  c.markings.push_back(m6);

  // Landmarks.
  add_landmark(c, "eye_center.left", "eye_center", "left_eye", 0, 0, "eye_center.right", false, true);
  add_landmark(c, "eye_center.right", "eye_center", "right_eye", 0, 0, "eye_center.left", false, true);
  add_landmark(c, "nose_tip", "nose_tip", "nose", 0, 0.5);
  add_landmark(c, "ear_tip.left", "ear_tip", "left_ear", 0, 2.3, "ear_tip.right", true);
  add_landmark(c, "ear_tip.right", "ear_tip", "right_ear", 0, 2.3, "ear_tip.left", true);
  c.landmarks.at("ear_tip.right").ordering_after = {"ear_tip.left"};
  add_landmark(c, "tail_base_lm", "tail_base", "tail_base", 0, 0, "", false, true);
  add_landmark(c, "tail_tip_lm", "tail_tip", "tail_tip", 0, -1.8, "", false, true);
  // The tail extends backward (negative x): the base is the rightmost tail
  // landmark, so it must be ordered after (to the right of) the tip.
  c.landmarks.at("tail_base_lm").ordering_after = {"tail_tip_lm"};
  add_landmark(c, "shoulder.left", "shoulder", "left_leg", 0, 3, "shoulder.right");
  add_landmark(c, "shoulder.right", "shoulder", "right_leg", 0, 3, "shoulder.left");
  add_landmark(c, "hip.left", "hip", "left_hind", 0, 3.5, "hip.right");
  add_landmark(c, "hip.right", "hip", "right_hind", 0, 3.5, "hip.left");

  // Relational construction constraints (GENERATIVE): geometry is derived
  // from relationships, not copied from absolute coordinates. Absolute
  // values remain projection hints for unconstrained axes.
  // head width = torso width * ratio (this IS head_to_body, generative).
  add_construction(c, "gen_head_width", ConstructionConstraintKind::size_ratio,
                   "head", "torso", ConstructionAxis::x, 8.0 / 18.0, 0.0);
  // head height anchored to torso top chain: head.y = torso.y + torso.size_y * 0.82.
  add_construction(c, "gen_head_height", ConstructionConstraintKind::size_ratio,
                   "head", "torso", ConstructionAxis::y, 7.0 / 11.0, 0.0);
  // Muzzle chain: muzzle.y = head.y + head.size_y * 0.36 (snout forward).
  add_construction(c, "gen_muzzle_chain", ConstructionConstraintKind::chain,
                   "muzzle", "head", ConstructionAxis::y, 0.36, 0.0);
  // Ears: symmetry across the head-local x plane.
  add_construction(c, "gen_ear_symmetry", ConstructionConstraintKind::symmetry,
                   "right_ear", "left_ear", ConstructionAxis::x, 1.0, 0.0);
  add_construction(c, "gen_ear_orient", ConstructionConstraintKind::orientation,
                   "right_ear", "left_ear", ConstructionAxis::x, -1.0, 0.0);
  // Eyes: symmetry across the head-local x plane.
  add_construction(c, "gen_eye_symmetry", ConstructionConstraintKind::symmetry,
                   "right_eye", "left_eye", ConstructionAxis::x, 1.0, 0.0);
  // Legs: symmetry across the torso-local x plane.
  add_construction(c, "gen_leg_symmetry", ConstructionConstraintKind::symmetry,
                   "right_leg", "left_leg", ConstructionAxis::x, 1.0, 0.0);
  add_construction(c, "gen_leg_orient", ConstructionConstraintKind::orientation,
                   "right_leg", "left_leg", ConstructionAxis::x, -1.0, 0.0);
  // Hind legs: symmetry.
  add_construction(c, "gen_hind_symmetry", ConstructionConstraintKind::symmetry,
                   "right_hind", "left_hind", ConstructionAxis::x, 1.0, 0.0);
  // Tail chain: tail_mid extends from tail_base, tail_tip from tail_mid.
  add_construction(c, "gen_tail_mid", ConstructionConstraintKind::chain,
                   "tail_mid", "tail_base", ConstructionAxis::x, 0.5, 0.0);
  add_construction(c, "gen_tail_tip", ConstructionConstraintKind::chain,
                   "tail_tip", "tail_mid", ConstructionAxis::x, 0.5, 0.0);

  // Relational proportions (raster-unit invariant ratios). The `derived`
  // field makes the proportion GENERATIVE: changing `preferred` changes
  // constructed geometry (causal, mutation-tested).
  ProportionRule ph;
  ph.id = "head_to_body";
  ph.numerator = make_size_measure("head", "size_x");
  ph.denominator = make_size_measure("torso", "size_x");
  ph.preferred = 8.0 / 18.0;
  ph.min = 0.39; ph.max = 0.50;
  ph.deformation_min = 0.32; ph.deformation_max = 0.56;
  ph.hard = true;
  ph.derived = make_size_measure("head", "size_x");   // generative
  ph.derived_hard = true;
  c.proportions.push_back(std::move(ph));
  add_proportion(c, "eye_spacing", make_landmark_dist_measure("eye_center.left", "eye_center.right"),
                 make_size_measure("head", "size_x"), 0.55, 0.48, 0.62, 0.38, 0.70, true);
  ProportionRule pl;
  pl.id = "leg_to_body";
  pl.numerator = make_size_measure("left_leg", "size_y");
  pl.denominator = make_size_measure("torso", "size_y");
  pl.preferred = 8.0 / 11.0;
  pl.min = 0.64; pl.max = 0.82;
  pl.deformation_min = 0.5; pl.deformation_max = 0.95;
  pl.hard = true;
  pl.derived = make_size_measure("left_leg", "size_y");
  pl.derived_hard = true;
  c.proportions.push_back(std::move(pl));
  add_proportion(c, "tail_length", make_landmark_dist_measure("tail_base_lm", "tail_tip_lm"),
                 make_size_measure("torso", "size_x"), 0.27, 0.20, 0.35, 0.14, 0.42, false);
  add_proportion(c, "muzzle_to_head", make_size_measure("muzzle", "size_y"),
                 make_size_measure("head", "size_y"), 0.31, 0.24, 0.40, 0.18, 0.48, false);

  // Silhouette features.
  SilhouetteFeature sf1; sf1.id = "sil_torso"; sf1.kind = SilhouetteFeatureKind::mass;
  sf1.structure_ref = "torso"; sf1.priority = 10; sf1.min_contribution = 0.25;
  c.silhouette_features.push_back(sf1);
  SilhouetteFeature sf2; sf2.id = "sil_head"; sf2.kind = SilhouetteFeatureKind::mass;
  sf2.structure_ref = "head"; sf2.priority = 8;
  c.silhouette_features.push_back(sf2);
  SilhouetteFeature sf3; sf3.id = "sil_ear_l"; sf3.kind = SilhouetteFeatureKind::protrusion;
  sf3.structure_ref = "left_ear"; sf3.priority = 7; sf3.min_separation = 2.0;
  c.silhouette_features.push_back(sf3);
  SilhouetteFeature sf4; sf4.id = "sil_ear_r"; sf4.kind = SilhouetteFeatureKind::protrusion;
  sf4.structure_ref = "right_ear"; sf4.priority = 7; sf4.min_separation = 2.0;
  c.silhouette_features.push_back(sf4);
  SilhouetteFeature sf5; sf5.id = "sil_tail"; sf5.kind = SilhouetteFeatureKind::protrusion;
  sf5.structure_ref = "tail_tip"; sf5.priority = 7; sf5.min_separation = 2.0;
  c.silhouette_features.push_back(sf5);
  SilhouetteFeature sf6; sf6.id = "sil_muzzle"; sf6.kind = SilhouetteFeatureKind::protrusion;
  sf6.structure_ref = "muzzle"; sf6.priority = 5;
  c.silhouette_features.push_back(sf6);

  // Surfaces / materials / colors.
  SurfaceRegion sur1; sur1.id = "surface.head"; sur1.structure_ref = "head"; sur1.role = "fur_region";
  SurfaceRegion sur2; sur2.id = "surface.muzzle"; sur2.structure_ref = "muzzle"; sur2.role = "skin_region";
  SurfaceRegion sur3; sur3.id = "surface.eye_l"; sur3.structure_ref = "left_eye"; sur3.role = "eye_surface";
  SurfaceRegion sur4; sur4.id = "surface.torso"; sur4.structure_ref = "torso"; sur4.role = "fur_region";
  c.surfaces = {sur1, sur2, sur3, sur4};

  MaterialBinding mat1; mat1.id = "mat.torso"; mat1.structure_ref = "torso"; mat1.material_class = "fur";
  MaterialBinding mat2; mat2.id = "mat.head"; mat2.structure_ref = "head"; mat2.material_class = "fur";
  MaterialBinding mat3; mat3.id = "mat.muzzle"; mat3.structure_ref = "muzzle"; mat3.material_class = "skin";
  MaterialBinding mat4; mat4.id = "mat.eyes"; mat4.structure_ref = "left_eye"; mat4.material_class = "glass";
  MaterialBinding mat5; mat5.id = "mat.tail"; mat5.structure_ref = "tail_tip"; mat5.material_class = "fur";
  MaterialBinding mat6; mat6.id = "mat.legs"; mat6.structure_ref = "left_leg"; mat6.material_class = "fur";
  c.materials = {mat1, mat2, mat3, mat4, mat5, mat6};

  ColorRegion col1; col1.id = "color.torso"; col1.structure_ref = "torso"; col1.color_role = "primary";
  ColorRegion col2; col2.id = "color.chest"; col2.structure_ref = "chest"; col2.color_role = "secondary";
  ColorRegion col3; col3.id = "color.eyes"; col3.structure_ref = "left_eye"; col3.color_role = "eye";
  ColorRegion col4; col4.id = "color.tail_tip"; col4.structure_ref = "tail_tip"; col4.color_role = "secondary";
  ColorRegion col5; col5.id = "color.paws"; col5.structure_ref = "left_foot"; col5.color_role = "shadow";
  c.color_regions = {col1, col2, col3, col4, col5};

  // Expressive regions (generic framework; robots/flowers use other kinds).
  ExpressiveRegion face;
  face.id = "face"; face.structure_ref = "head";
  ExpressiveFeature fe1; fe1.id = "feature.eye_left"; fe1.kind = ExpressiveFeatureKind::eye;
  fe1.region_id = "face"; fe1.gaze = 0.0; fe1.aperture = 1.0; fe1.intensity = 0.25;
  ExpressiveFeature fe2; fe2.id = "feature.eye_right"; fe2.kind = ExpressiveFeatureKind::eye;
  fe2.region_id = "face"; fe2.gaze = 0.0; fe2.aperture = 1.0; fe2.intensity = 0.25;
  ExpressiveFeature fe3; fe3.id = "feature.mouth"; fe3.kind = ExpressiveFeatureKind::mouth;
  fe3.region_id = "face"; fe3.aperture = 0.4;
  ExpressiveFeature fe4; fe4.id = "feature.brow_left"; fe4.kind = ExpressiveFeatureKind::brow;
  fe4.region_id = "face";
  ExpressiveFeature fe5; fe5.id = "feature.brow_right"; fe5.kind = ExpressiveFeatureKind::brow;
  fe5.region_id = "face";
  face.features = {fe1, fe2, fe3, fe4, fe5};
  c.facial_regions.push_back(face);

  AttachmentPoint at1; at1.id = "attach.tail_root"; at1.structure_ref = "tail_base";
  at1.socket_id = "tail_root"; at1.role = "tail";
  AttachmentPoint at2; at2.id = "attach.ear_left"; at2.structure_ref = "left_ear";
  at2.socket_id = "ear_socket"; at2.role = "ear";
  c.attachments = {at1, at2};

  // Deformation envelopes.
  add_envelope(c, "torso", 1.6, 4.0, false, 1.6);
  add_envelope(c, "chest", 1.4, 3.2, false, 1.4);
  add_envelope(c, "head", 1.6, 3.6, false, 1.5);
  add_envelope(c, "muzzle", 1.1, 2.6, false, 1.5);
  add_envelope(c, "left_ear", 2.2, 5.0, false, 1.5);
  add_envelope(c, "right_ear", 2.2, 5.0, false, 1.5);
  add_envelope(c, "left_eye", 0.7, 1.4, true, 1.0);
  add_envelope(c, "right_eye", 0.7, 1.4, true, 1.0);
  add_envelope(c, "left_leg", 3.2, 7.0, false, 1.6);
  add_envelope(c, "right_leg", 3.2, 7.0, false, 1.6);
  add_envelope(c, "tail_tip", 4.2, 9.0, false, 1.8);

  // Resolution features.
  VisualFeature rf1; rf1.id = "res.eyes"; rf1.structure_ref = "left_eye";
  rf1.semantic_priority = 100; rf1.recognition_importance = 1.0; rf1.min_resolution = 32;
  VisualFeature rf2; rf2.id = "res.ears"; rf2.structure_ref = "left_ear";
  rf2.semantic_priority = 60; rf2.recognition_importance = 0.8; rf2.min_resolution = 24;
  VisualFeature rf3; rf3.id = "res.tail"; rf3.structure_ref = "tail_tip";
  rf3.semantic_priority = 40; rf3.recognition_importance = 0.6; rf3.min_resolution = 16;
  VisualFeature rf4; rf4.id = "res.nose"; rf4.structure_ref = "nose";
  rf4.semantic_priority = 50; rf4.recognition_importance = 0.7; rf4.min_resolution = 24;
  c.resolution_features = {rf1, rf2, rf3, rf4};

  // Identity invariants.
  add_invariant(c, "inv.head_proportion", IdentityInvariantKind::proportion, {"head_to_body"}, 0.05, true);
  add_invariant(c, "inv.eye_spacing", IdentityInvariantKind::proportion, {"eye_spacing"}, 0.05, true);
  add_invariant(c, "inv.leg_proportion", IdentityInvariantKind::proportion, {"leg_to_body"}, 0.05, true);
  add_invariant(c, "inv.eyes_exist", IdentityInvariantKind::landmark_existence,
                {"eye_center.left", "eye_center.right"}, 0.0, true);
  add_invariant(c, "inv.nose_exist", IdentityInvariantKind::landmark_existence, {"nose_tip"}, 0.0, true);
  add_invariant(c, "inv.ear_order", IdentityInvariantKind::landmark_ordering, {"ear_tip.right"}, 0.0, true);
  add_invariant(c, "inv.tail_order", IdentityInvariantKind::landmark_ordering, {"tail_base_lm"}, 0.0, true);
  add_invariant(c, "inv.torso_markings", IdentityInvariantKind::marking_topology, {"torso_stripes"}, 0.0, true);
  add_invariant(c, "inv.storm_markings", IdentityInvariantKind::marking_topology, {"storm_circuit"}, 0.0, true, "form:storm");
  add_invariant(c, "inv.eye_material", IdentityInvariantKind::material_truth, {"left_eye"}, 0.0, true);
  add_invariant(c, "inv.torso_boundary", IdentityInvariantKind::hard_boundary, {"torso"}, 0.0, true);

  c.base_palette["primary"] = "#b4753a";
  c.base_palette["secondary"] = "#f5ecd8";
  c.base_palette["accent"] = "#ff8a3c";
  c.base_palette["eye"] = "#3f7cc0";
  c.base_palette["emission"] = "#9be8ff";
  c.base_palette["shadow"] = "#5a3a22";
  c.base_palette["highlight"] = "#ffd9a0";
  c.base_palette["outline"] = "#2e1c0e";
  c.base_palette["warning"] = "#ff5c38";
  c.base_palette["effect"] = "#9be8ff";
  c.form_palettes["storm"]["primary"] = "#2a1450";
  c.form_palettes["storm"]["secondary"] = "#8f7ad6";
  c.form_palettes["storm"]["accent"] = "#6ee7ff";
  c.form_palettes["storm"]["eye"] = "#d8c8ff";
  c.form_palettes["storm"]["emission"] = "#b18cff";
  c.form_palettes["storm"]["shadow"] = "#120a24";
  c.form_palettes["storm"]["highlight"] = "#c9b8ff";
  c.form_palettes["storm"]["outline"] = "#0a0518";
  return c;
}

VisualCanon make_humanoid_canon() {
  VisualCanon c;
  set_canon(c, "test_humanoid", "Test Humanoid", "original_user_creation");
  c.forms = {"base"};

  c.structures["pelvis"] = st("pelvis", "", "body-mass", "rounded_rect", "body", "cloth", "primary",
                              0, -5, 0, 12, 7, 8);
  c.structures["torso"] = st("torso", "pelvis", "body-mass", "rounded_rect", "body", "cloth", "primary",
                             0, 5, 1, 15, 15, 10);
  c.structures["neck"] = st("neck", "torso", "joint", "capsule", "body", "skin", "secondary",
                            0, 10, 3, 2.2, 2.6, 2.2);
  c.structures["head"] = st("head", "neck", "facial-feature", "ellipse", "body", "skin", "secondary",
                            0, 13.5, 4, 9, 9, 9);
  c.structures["hair"] = st("hair", "head", "appendage", "capsule", "body", "fur", "shadow",
                            0, 15.5, 3, 8, 4, 4);
  c.structures["left_arm"] = st("left_arm", "torso", "limb", "capsule", "body", "cloth", "primary",
                                -8, 2, 2, 4, 10, 4, 8);
  c.structures["right_arm"] = st("right_arm", "torso", "limb", "capsule", "body", "cloth", "primary",
                                 8, 2, 2, 4, 10, 4, -8);
  c.structures["left_hand"] = st("left_hand", "left_arm", "appendage", "ellipse", "body", "skin",
                                 "secondary", 0, -5, 3, 3.5, 3, 2.5);
  c.structures["right_hand"] = st("right_hand", "right_arm", "appendage", "ellipse", "body", "skin",
                                  "secondary", 0, -5, 3, 3.5, 3, 2.5);
  c.structures["left_leg"] = st("left_leg", "pelvis", "limb", "capsule", "body", "cloth", "shadow",
                                -3.5, -9, 1, 4, 9, 4);
  c.structures["right_leg"] = st("right_leg", "pelvis", "limb", "capsule", "body", "cloth", "shadow",
                                 3.5, -9, 1, 4, 9, 4);
  c.structures["left_foot"] = st("left_foot", "left_leg", "appendage", "capsule", "body", "cloth",
                                 "shadow", 0, -4.5, 2, 3, 2, 2);
  c.structures["right_foot"] = st("right_foot", "right_leg", "appendage", "capsule", "body", "cloth",
                                  "shadow", 0, -4.5, 2, 3, 2, 2);
  c.structures["left_eye"] = st("left_eye", "head", "eye", "ellipse", "facial", "glass", "eye",
                                -2, 15.5, 5, 1.5, 1.5, 1);
  c.structures["right_eye"] = st("right_eye", "head", "eye", "ellipse", "facial", "glass", "eye",
                                 2, 15.5, 5, 1.5, 1.5, 1);
  c.structures["mouth"] = st("mouth", "head", "facial-feature", "capsule", "facial", "skin",
                             "accent", 0, 11.5, 5, 2.4, 0.8, 1);

  MarkingBinding m1; m1.id = "shirt_stripes"; m1.kind = MarkingKind::stripe;
  m1.structure_ref = "torso"; m1.color_role = "secondary"; m1.opacity = 0.5; m1.scale = 0.7;
  c.markings.push_back(m1);

  add_landmark(c, "eye_center.left", "eye_center", "left_eye", 0, 0, "eye_center.right", false, true);
  add_landmark(c, "eye_center.right", "eye_center", "right_eye", 0, 0, "eye_center.left", false, true);
  add_landmark(c, "chin", "chin", "head", 0, -4);
  add_landmark(c, "elbow.left", "elbow", "left_arm", 0, -4, "elbow.right");
  add_landmark(c, "elbow.right", "elbow", "right_arm", 0, -4, "elbow.left");
  add_landmark(c, "knee.left", "knee", "left_leg", 0, -4, "knee.right");
  add_landmark(c, "knee.right", "knee", "right_leg", 0, -4, "knee.left");

  add_proportion(c, "head_to_torso", make_size_measure("head", "size_x"),
                 make_size_measure("torso", "size_x"), 0.6, 0.53, 0.67, 0.45, 0.75, true);
  add_proportion(c, "eye_spacing", make_landmark_dist_measure("eye_center.left", "eye_center.right"),
                 make_size_measure("head", "size_x"), 0.44, 0.38, 0.52, 0.3, 0.6, true);
  add_proportion(c, "arm_len", make_size_measure("left_arm", "size_y"),
                 make_size_measure("torso", "size_y"), 0.67, 0.58, 0.76, 0.48, 0.86, false);
  add_proportion(c, "leg_len", make_size_measure("left_leg", "size_y"),
                 make_size_measure("torso", "size_y"), 0.6, 0.52, 0.68, 0.42, 0.78, false);

  SilhouetteFeature sf1; sf1.id = "sil_torso"; sf1.kind = SilhouetteFeatureKind::mass;
  sf1.structure_ref = "torso"; sf1.priority = 10; sf1.min_contribution = 0.25;
  SilhouetteFeature sf2; sf2.id = "sil_head"; sf2.kind = SilhouetteFeatureKind::mass;
  sf2.structure_ref = "head"; sf2.priority = 9;
  SilhouetteFeature sf3; sf3.id = "sil_arm_l"; sf3.kind = SilhouetteFeatureKind::protrusion;
  sf3.structure_ref = "left_arm"; sf3.priority = 6; sf3.min_separation = 2.0;
  SilhouetteFeature sf4; sf4.id = "sil_arm_r"; sf4.kind = SilhouetteFeatureKind::protrusion;
  sf4.structure_ref = "right_arm"; sf4.priority = 6; sf4.min_separation = 2.0;
  c.silhouette_features = {sf1, sf2, sf3, sf4};

  SurfaceRegion sur1; sur1.id = "surface.head"; sur1.structure_ref = "head"; sur1.role = "skin_region";
  SurfaceRegion sur2; sur2.id = "surface.torso"; sur2.structure_ref = "torso"; sur2.role = "cloth_region";
  c.surfaces = {sur1, sur2};

  MaterialBinding mat1; mat1.id = "mat.torso"; mat1.structure_ref = "torso"; mat1.material_class = "cloth";
  MaterialBinding mat2; mat2.id = "mat.head"; mat2.structure_ref = "head"; mat2.material_class = "skin";
  MaterialBinding mat3; mat3.id = "mat.eyes"; mat3.structure_ref = "left_eye"; mat3.material_class = "glass";
  MaterialBinding mat4; mat4.id = "mat.hair"; mat4.structure_ref = "hair"; mat4.material_class = "fur";
  c.materials = {mat1, mat2, mat3, mat4};

  ColorRegion col1; col1.id = "color.torso"; col1.structure_ref = "torso"; col1.color_role = "primary";
  ColorRegion col2; col2.id = "color.legs"; col2.structure_ref = "left_leg"; col2.color_role = "shadow";
  ColorRegion col3; col3.id = "color.eyes"; col3.structure_ref = "left_eye"; col3.color_role = "eye";
  c.color_regions = {col1, col2, col3};

  ExpressiveRegion face;
  face.id = "face"; face.structure_ref = "head";
  ExpressiveFeature fe1; fe1.id = "feature.eye_left"; fe1.kind = ExpressiveFeatureKind::eye;
  fe1.region_id = "face"; fe1.gaze = 0.0; fe1.aperture = 1.0; fe1.intensity = 0.2;
  ExpressiveFeature fe2; fe2.id = "feature.eye_right"; fe2.kind = ExpressiveFeatureKind::eye;
  fe2.region_id = "face"; fe2.gaze = 0.0; fe2.aperture = 1.0; fe2.intensity = 0.2;
  ExpressiveFeature fe3; fe3.id = "feature.mouth"; fe3.kind = ExpressiveFeatureKind::mouth;
  fe3.region_id = "face"; fe3.aperture = 0.3;
  face.features = {fe1, fe2, fe3};
  c.facial_regions.push_back(face);

  add_envelope(c, "head", 1.6, 3.6, false, 1.5);
  add_envelope(c, "left_arm", 3.2, 7.0, false, 1.6);
  add_envelope(c, "right_arm", 3.2, 7.0, false, 1.6);
  add_envelope(c, "left_leg", 2.8, 6.0, false, 1.5);
  add_envelope(c, "right_leg", 2.8, 6.0, false, 1.5);
  add_envelope(c, "left_eye", 0.7, 1.4, true, 1.0);
  add_envelope(c, "right_eye", 0.7, 1.4, true, 1.0);

  VisualFeature rf1; rf1.id = "res.eyes"; rf1.structure_ref = "left_eye";
  rf1.semantic_priority = 100; rf1.recognition_importance = 1.0; rf1.min_resolution = 32;
  VisualFeature rf2; rf2.id = "res.head"; rf2.structure_ref = "head";
  rf2.semantic_priority = 70; rf2.recognition_importance = 0.9; rf2.min_resolution = 24;
  c.resolution_features = {rf1, rf2};

  add_invariant(c, "inv.head_proportion", IdentityInvariantKind::proportion, {"head_to_torso"}, 0.05, true);
  add_invariant(c, "inv.eye_spacing", IdentityInvariantKind::proportion, {"eye_spacing"}, 0.05, true);
  add_invariant(c, "inv.eyes_exist", IdentityInvariantKind::landmark_existence,
                {"eye_center.left", "eye_center.right"}, 0.0, true);
  add_invariant(c, "inv.shirt_markings", IdentityInvariantKind::marking_topology, {"shirt_stripes"}, 0.0, true);
  add_invariant(c, "inv.eye_material", IdentityInvariantKind::material_truth, {"left_eye"}, 0.0, true);

  c.base_palette["primary"] = "#c0504a";
  c.base_palette["secondary"] = "#e8b98a";
  c.base_palette["accent"] = "#f2c14e";
  c.base_palette["eye"] = "#1c1c28";
  c.base_palette["emission"] = "#ffe9a8";
  c.base_palette["shadow"] = "#40507a";
  c.base_palette["highlight"] = "#ffe9c4";
  c.base_palette["outline"] = "#2a1a14";
  c.base_palette["warning"] = "#e8542a";
  c.base_palette["effect"] = "#ffd27a";
  return c;
}

VisualCanon make_mech_canon() {
  VisualCanon c;
  set_canon(c, "test_mech", "Test Mech", "original_user_creation");
  c.forms = {"base"};

  c.structures["chassis"] = st("chassis", "", "body-mass", "rounded_rect", "body", "metal", "primary",
                               0, 0, 0, 17, 11, 10);
  c.structures["turret"] = st("turret", "chassis", "armor-panel", "capsule", "body", "metal", "primary",
                              0, 5.5, 5, 10, 4.5, 7);
  c.structures["barrel"] = st("barrel", "turret", "weapon", "capsule", "front_appendages", "metal",
                              "shadow", 0, 4, 7, 3, 6.5, 3);
  c.structures["left_tread"] = st("left_tread", "chassis", "limb", "capsule", "rear_appendages", "metal",
                                  "shadow", -8, -7.5, 1, 5.5, 4.5, 5);
  c.structures["right_tread"] = st("right_tread", "chassis", "limb", "capsule", "rear_appendages", "metal",
                                   "shadow", 8, -7.5, 1, 5.5, 4.5, 5);
  c.structures["core_light"] = st("core_light", "chassis", "energy", "ellipse", "emission", "energy",
                                  "warning", 0, -1, 9, 3.2, 3.2, 1.2, 0, 10);
  c.structures["visor"] = st("visor", "turret", "eye", "capsule", "facial", "glass", "emission",
                             0, 2, 9, 7, 1.8, 1.4, 0, 9);
  c.structures["left_antenna"] = st("left_antenna", "chassis", "appendage", "capsule", "rear_appendages",
                                    "metal", "secondary", -3.5, 7, 8, 1, 4.5, 1, -12);
  c.structures["right_antenna"] = st("right_antenna", "chassis", "appendage", "capsule", "rear_appendages",
                                     "metal", "secondary", 3.5, 7, 8, 1, 4.5, 1, 12);
  c.structures["armor_left"] = st("armor_left", "chassis", "armor-panel", "rounded_rect", "body",
                                  "metal", "secondary", -4.5, 3.5, 4, 5.5, 4.5, 3, 8);
  c.structures["armor_right"] = st("armor_right", "chassis", "armor-panel", "rounded_rect", "body",
                                   "metal", "secondary", 4.5, 3.5, 4, 5.5, 4.5, 3, -8);

  MarkingBinding m1; m1.id = "chassis_circuit"; m1.kind = MarkingKind::circuit;
  m1.structure_ref = "chassis"; m1.color_role = "effect"; m1.opacity = 0.7; m1.scale = 1.0;
  m1.intent = "energy";
  MarkingBinding m2; m2.id = "tread_hazard_l"; m2.kind = MarkingKind::band;
  m2.structure_ref = "left_tread"; m2.color_role = "warning"; m2.opacity = 0.8; m2.scale = 0.6;
  MarkingBinding m3; m3.id = "tread_hazard_r"; m3.kind = MarkingKind::band;
  m3.structure_ref = "right_tread"; m3.color_role = "warning"; m3.opacity = 0.8; m3.scale = 0.6;
  c.markings = {m1, m2, m3};

  add_landmark(c, "visor_center", "visor_center", "visor", 0, 0, "", false, true);
  add_landmark(c, "core_center", "core_center", "core_light", 0, 0, "", false, true);
  add_landmark(c, "barrel_tip", "barrel_tip", "barrel", 0, 3.2, "", true);
  add_landmark(c, "wheel_center.left", "wheel_center", "left_tread", 0, 0, "wheel_center.right");
  add_landmark(c, "wheel_center.right", "wheel_center", "right_tread", 0, 0, "wheel_center.left");
  add_landmark(c, "antenna_tip.left", "antenna_tip", "left_antenna", 0, 2.2, "antenna_tip.right");
  add_landmark(c, "antenna_tip.right", "antenna_tip", "right_antenna", 0, 2.2, "antenna_tip.left");

  add_proportion(c, "turret_to_chassis", make_size_measure("turret", "size_x"),
                 make_size_measure("chassis", "size_x"), 0.59, 0.52, 0.66, 0.44, 0.74, true);
  add_proportion(c, "barrel_len", make_size_measure("barrel", "size_y"),
                 make_size_measure("chassis", "size_y"), 0.59, 0.5, 0.68, 0.4, 0.78, false);
  add_proportion(c, "tread_span", make_landmark_dist_measure("wheel_center.left", "wheel_center.right"),
                 make_size_measure("chassis", "size_x"), 0.94, 0.84, 1.04, 0.74, 1.14, false);

  SilhouetteFeature sf1; sf1.id = "sil_chassis"; sf1.kind = SilhouetteFeatureKind::mass;
  sf1.structure_ref = "chassis"; sf1.priority = 10; sf1.min_contribution = 0.25;
  SilhouetteFeature sf2; sf2.id = "sil_turret"; sf2.kind = SilhouetteFeatureKind::mass;
  sf2.structure_ref = "turret"; sf2.priority = 8;
  SilhouetteFeature sf3; sf3.id = "sil_barrel"; sf3.kind = SilhouetteFeatureKind::protrusion;
  sf3.structure_ref = "barrel"; sf3.priority = 6; sf3.min_separation = 2.0;
  SilhouetteFeature sf4; sf4.id = "sil_antenna_l"; sf4.kind = SilhouetteFeatureKind::protrusion;
  sf4.structure_ref = "left_antenna"; sf4.priority = 5;
  SilhouetteFeature sf5; sf5.id = "sil_antenna_r"; sf5.kind = SilhouetteFeatureKind::protrusion;
  sf5.structure_ref = "right_antenna"; sf5.priority = 5;
  c.silhouette_features = {sf1, sf2, sf3, sf4, sf5};

  SurfaceRegion sur1; sur1.id = "surface.chassis"; sur1.structure_ref = "chassis"; sur1.role = "hard_surface";
  SurfaceRegion sur2; sur2.id = "surface.visor"; sur2.structure_ref = "visor"; sur2.role = "eye_surface";
  c.surfaces = {sur1, sur2};

  MaterialBinding mat1; mat1.id = "mat.chassis"; mat1.structure_ref = "chassis"; mat1.material_class = "metal";
  MaterialBinding mat2; mat2.id = "mat.turret"; mat2.structure_ref = "turret"; mat2.material_class = "metal";
  MaterialBinding mat3; mat3.id = "mat.visor"; mat3.structure_ref = "visor"; mat3.material_class = "glass";
  MaterialBinding mat4; mat4.id = "mat.core"; mat4.structure_ref = "core_light"; mat4.material_class = "energy";
  c.materials = {mat1, mat2, mat3, mat4};

  ColorRegion col1; col1.id = "color.chassis"; col1.structure_ref = "chassis"; col1.color_role = "primary";
  ColorRegion col2; col2.id = "color.armor"; col2.structure_ref = "armor_left"; col2.color_role = "secondary";
  ColorRegion col3; col3.id = "color.core"; col3.structure_ref = "core_light"; col3.color_role = "warning";
  ColorRegion col4; col4.id = "color.visor"; col4.structure_ref = "visor"; col4.color_role = "emission";
  c.color_regions = {col1, col2, col3, col4};

  ExpressiveRegion face;
  face.id = "face"; face.structure_ref = "visor";
  ExpressiveFeature fe1; fe1.id = "feature.visor"; fe1.kind = ExpressiveFeatureKind::visor;
  fe1.region_id = "face"; fe1.aperture = 1.0; fe1.intensity = 0.8;
  ExpressiveFeature fe2; fe2.id = "feature.antenna_l"; fe2.kind = ExpressiveFeatureKind::antenna;
  fe2.region_id = "face"; fe2.rotation = -12.0;
  ExpressiveFeature fe3; fe3.id = "feature.antenna_r"; fe3.kind = ExpressiveFeatureKind::antenna;
  fe3.region_id = "face"; fe3.rotation = 12.0;
  face.features = {fe1, fe2, fe3};
  c.facial_regions.push_back(face);

  add_envelope(c, "chassis", 1.0, 2.6, true, 1.0);
  add_envelope(c, "turret", 1.6, 3.6, false, 1.2);
  add_envelope(c, "barrel", 1.6, 4.0, false, 1.3);
  add_envelope(c, "left_tread", 1.0, 2.6, false, 1.0);
  add_envelope(c, "right_tread", 1.0, 2.6, false, 1.0);
  add_envelope(c, "left_antenna", 3.0, 7.0, false, 1.5);
  add_envelope(c, "right_antenna", 3.0, 7.0, false, 1.5);

  VisualFeature rf1; rf1.id = "res.visor"; rf1.structure_ref = "visor";
  rf1.semantic_priority = 100; rf1.recognition_importance = 1.0; rf1.min_resolution = 32;
  VisualFeature rf2; rf2.id = "res.antenna"; rf2.structure_ref = "left_antenna";
  rf2.semantic_priority = 50; rf2.recognition_importance = 0.7; rf2.min_resolution = 16;
  VisualFeature rf3; rf3.id = "res.core"; rf3.structure_ref = "core_light";
  rf3.semantic_priority = 70; rf3.recognition_importance = 0.85; rf3.min_resolution = 24;
  c.resolution_features = {rf1, rf2, rf3};

  add_invariant(c, "inv.turret_proportion", IdentityInvariantKind::proportion, {"turret_to_chassis"}, 0.05, true);
  add_invariant(c, "inv.visor_exist", IdentityInvariantKind::landmark_existence, {"visor_center"}, 0.0, true);
  add_invariant(c, "inv.chassis_markings", IdentityInvariantKind::marking_topology, {"chassis_circuit"}, 0.0, true);
  add_invariant(c, "inv.visor_material", IdentityInvariantKind::material_truth, {"visor"}, 0.0, true);
  add_invariant(c, "inv.chassis_rigid", IdentityInvariantKind::hard_boundary, {"chassis"}, 0.0, true);

  c.base_palette["primary"] = "#4a6a8a";
  c.base_palette["secondary"] = "#7a9aba";
  c.base_palette["accent"] = "#ff5c38";
  c.base_palette["eye"] = "#66e0ff";
  c.base_palette["emission"] = "#66e0ff";
  c.base_palette["shadow"] = "#22303e";
  c.base_palette["highlight"] = "#c8e8ff";
  c.base_palette["outline"] = "#141c24";
  c.base_palette["warning"] = "#ff5c38";
  c.base_palette["effect"] = "#66e0ff";
  return c;
}

VisualCanon make_flyer_canon() {
  VisualCanon c;
  set_canon(c, "test_flyer", "Test Flyer", "original_user_creation");
  c.forms = {"base"};

  c.structures["body"] = st("body", "", "body-mass", "ellipse", "body", "skin", "primary",
                            0, 0, 0, 10, 7, 7);
  c.structures["head"] = st("head", "body", "facial-feature", "ellipse", "body", "skin", "primary",
                            0, 5.5, 3, 5.5, 5, 5);
  c.structures["beak"] = st("beak", "head", "facial-feature", "capsule", "facial", "skin", "warning",
                            0, 8.5, 2, 1.8, 2.2, 1.6);
  c.structures["left_wing_upper"] = st("left_wing_upper", "body", "appendage", "capsule",
                                       "rear_appendages", "skin", "secondary", -6, 2, 1, 7.5, 2.8, 2,
                                       -16);
  c.structures["right_wing_upper"] = st("right_wing_upper", "body", "appendage", "capsule",
                                        "rear_appendages", "skin", "secondary", 6, 2, 1, 7.5, 2.8, 2,
                                        16);
  c.structures["left_wing_lower"] = st("left_wing_lower", "left_wing_upper", "appendage", "capsule",
                                       "rear_appendages", "skin", "secondary", -4, -1, 1, 5, 2.2, 1.6,
                                       -10);
  c.structures["right_wing_lower"] = st("right_wing_lower", "right_wing_upper", "appendage", "capsule",
                                        "rear_appendages", "skin", "secondary", 4, -1, 1, 5, 2.2, 1.6,
                                        10);
  c.structures["tail_base"] = st("tail_base", "body", "appendage", "capsule", "rear_appendages", "skin",
                                 "primary", 0, -4.5, 1, 2.2, 3.2, 2);
  c.structures["tail_fan"] = st("tail_fan", "tail_base", "appendage", "capsule", "rear_appendages", "skin",
                                "accent", 0, -2, 2, 4, 1.6, 1.6, -8);
  c.structures["left_eye"] = st("left_eye", "head", "eye", "ellipse", "facial", "glass", "eye",
                                -1.8, 6.5, 5, 1.4, 1.4, 1);
  c.structures["right_eye"] = st("right_eye", "head", "eye", "ellipse", "facial", "glass", "eye",
                                 1.8, 6.5, 5, 1.4, 1.4, 1);
  c.structures["left_wing_light"] = st("left_wing_light", "left_wing_upper", "energy", "ellipse",
                                       "emission", "energy", "effect", -3, 0, 4, 1.1, 1.1, 0.8, 0, 8);
  c.structures["right_wing_light"] = st("right_wing_light", "right_wing_upper", "energy", "ellipse",
                                        "emission", "energy", "effect", 3, 0, 4, 1.1, 1.1, 0.8, 0, 8);

  MarkingBinding m1; m1.id = "wing_stripes_l"; m1.kind = MarkingKind::stripe;
  m1.structure_ref = "left_wing_upper"; m1.color_role = "primary"; m1.opacity = 0.5; m1.scale = 0.6;
  MarkingBinding m2; m2.id = "wing_stripes_r"; m2.kind = MarkingKind::stripe;
  m2.structure_ref = "right_wing_upper"; m2.color_role = "primary"; m2.opacity = 0.5; m2.scale = 0.6;
  MarkingBinding m3; m3.id = "body_spots"; m3.kind = MarkingKind::spot;
  m3.structure_ref = "body"; m3.color_role = "accent"; m3.opacity = 0.7; m3.scale = 0.6;
  c.markings = {m1, m2, m3};

  add_landmark(c, "eye_center.left", "eye_center", "left_eye", 0, 0, "eye_center.right", false, true);
  add_landmark(c, "eye_center.right", "eye_center", "right_eye", 0, 0, "eye_center.left", false, true);
  add_landmark(c, "beak_tip", "beak_tip", "beak", 0, 1.1, "", true);
  add_landmark(c, "wing_tip.left", "wing_tip", "left_wing_lower", 0, -1.1, "wing_tip.right", true);
  add_landmark(c, "wing_tip.right", "wing_tip", "right_wing_lower", 0, -1.1, "wing_tip.left", true);
  add_landmark(c, "tail_fan_tip", "tail_fan_tip", "tail_fan", 0, -0.8);

  add_proportion(c, "head_to_body", make_size_measure("head", "size_x"),
                 make_size_measure("body", "size_x"), 0.55, 0.48, 0.62, 0.4, 0.7, true);
  add_proportion(c, "eye_spacing", make_landmark_dist_measure("eye_center.left", "eye_center.right"),
                 make_size_measure("head", "size_x"), 0.65, 0.57, 0.74, 0.47, 0.84, true);
  add_proportion(c, "wing_span", make_landmark_dist_measure("wing_tip.left", "wing_tip.right"),
                 make_size_measure("body", "size_x"), 2.2, 1.9, 2.5, 1.6, 2.8, false);
  add_proportion(c, "beak_len", make_size_measure("beak", "size_y"),
                 make_size_measure("head", "size_y"), 0.44, 0.35, 0.55, 0.25, 0.65, false);

  SilhouetteFeature sf1; sf1.id = "sil_body"; sf1.kind = SilhouetteFeatureKind::mass;
  sf1.structure_ref = "body"; sf1.priority = 10; sf1.min_contribution = 0.2;
  SilhouetteFeature sf2; sf2.id = "sil_head"; sf2.kind = SilhouetteFeatureKind::mass;
  sf2.structure_ref = "head"; sf2.priority = 8;
  SilhouetteFeature sf3; sf3.id = "sil_wing_l"; sf3.kind = SilhouetteFeatureKind::protrusion;
  sf3.structure_ref = "left_wing_lower"; sf3.priority = 8; sf3.min_separation = 2.0;
  SilhouetteFeature sf4; sf4.id = "sil_wing_r"; sf4.kind = SilhouetteFeatureKind::protrusion;
  sf4.structure_ref = "right_wing_lower"; sf4.priority = 8; sf4.min_separation = 2.0;
  SilhouetteFeature sf5; sf5.id = "sil_beak"; sf5.kind = SilhouetteFeatureKind::protrusion;
  sf5.structure_ref = "beak"; sf5.priority = 6;
  c.silhouette_features = {sf1, sf2, sf3, sf4, sf5};

  SurfaceRegion sur1; sur1.id = "surface.body"; sur1.structure_ref = "body"; sur1.role = "feather_region";
  SurfaceRegion sur2; sur2.id = "surface.wing_l"; sur2.structure_ref = "left_wing_upper"; sur2.role = "feather_region";
  c.surfaces = {sur1, sur2};

  MaterialBinding mat1; mat1.id = "mat.body"; mat1.structure_ref = "body"; mat1.material_class = "skin";
  MaterialBinding mat2; mat2.id = "mat.wings"; mat2.structure_ref = "left_wing_upper"; mat2.material_class = "skin";
  MaterialBinding mat3; mat3.id = "mat.eyes"; mat3.structure_ref = "left_eye"; mat3.material_class = "glass";
  MaterialBinding mat4; mat4.id = "mat.lights"; mat4.structure_ref = "left_wing_light"; mat4.material_class = "energy";
  c.materials = {mat1, mat2, mat3, mat4};

  ColorRegion col1; col1.id = "color.body"; col1.structure_ref = "body"; col1.color_role = "primary";
  ColorRegion col2; col2.id = "color.wings"; col2.structure_ref = "left_wing_upper"; col2.color_role = "secondary";
  ColorRegion col3; col3.id = "color.eyes"; col3.structure_ref = "left_eye"; col3.color_role = "eye";
  ColorRegion col4; col4.id = "color.tail"; col4.structure_ref = "tail_fan"; col4.color_role = "accent";
  c.color_regions = {col1, col2, col3, col4};

  ExpressiveRegion face;
  face.id = "face"; face.structure_ref = "head";
  ExpressiveFeature fe1; fe1.id = "feature.eye_left"; fe1.kind = ExpressiveFeatureKind::eye;
  fe1.region_id = "face"; fe1.gaze = 0.0; fe1.aperture = 1.0; fe1.intensity = 0.3;
  ExpressiveFeature fe2; fe2.id = "feature.eye_right"; fe2.kind = ExpressiveFeatureKind::eye;
  fe2.region_id = "face"; fe2.gaze = 0.0; fe2.aperture = 1.0; fe2.intensity = 0.3;
  face.features = {fe1, fe2};
  c.facial_regions.push_back(face);

  add_envelope(c, "head", 1.6, 3.6, false, 1.5);
  add_envelope(c, "left_wing_upper", 2.8, 6.0, false, 1.6);
  add_envelope(c, "right_wing_upper", 2.8, 6.0, false, 1.6);
  add_envelope(c, "left_eye", 0.7, 1.4, true, 1.0);
  add_envelope(c, "right_eye", 0.7, 1.4, true, 1.0);
  add_envelope(c, "tail_base", 2.2, 5.0, false, 1.5);

  VisualFeature rf1; rf1.id = "res.eyes"; rf1.structure_ref = "left_eye";
  rf1.semantic_priority = 100; rf1.recognition_importance = 1.0; rf1.min_resolution = 32;
  VisualFeature rf2; rf2.id = "res.wings"; rf2.structure_ref = "left_wing_upper";
  rf2.semantic_priority = 80; rf2.recognition_importance = 0.9; rf2.min_resolution = 24;
  VisualFeature rf3; rf3.id = "res.beak"; rf3.structure_ref = "beak";
  rf3.semantic_priority = 60; rf3.recognition_importance = 0.75; rf3.min_resolution = 16;
  c.resolution_features = {rf1, rf2, rf3};

  add_invariant(c, "inv.head_proportion", IdentityInvariantKind::proportion, {"head_to_body"}, 0.05, true);
  add_invariant(c, "inv.eye_spacing", IdentityInvariantKind::proportion, {"eye_spacing"}, 0.05, true);
  add_invariant(c, "inv.eyes_exist", IdentityInvariantKind::landmark_existence,
                {"eye_center.left", "eye_center.right"}, 0.0, true);
  add_invariant(c, "inv.wing_markings", IdentityInvariantKind::marking_topology, {"wing_stripes_l"}, 0.0, true);
  add_invariant(c, "inv.eye_material", IdentityInvariantKind::material_truth, {"left_eye"}, 0.0, true);

  c.base_palette["primary"] = "#7a4ac0";
  c.base_palette["secondary"] = "#5a2a90";
  c.base_palette["accent"] = "#3ce0b0";
  c.base_palette["eye"] = "#3ce0b0";
  c.base_palette["emission"] = "#3ce0b0";
  c.base_palette["shadow"] = "#2a1440";
  c.base_palette["highlight"] = "#c9b8ff";
  c.base_palette["outline"] = "#1a0a28";
  c.base_palette["warning"] = "#ffb03c";
  c.base_palette["effect"] = "#3ce0b0";
  return c;
}

} // namespace gspl::sprites::visual
