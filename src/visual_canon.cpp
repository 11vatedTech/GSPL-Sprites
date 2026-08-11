#include "gspl_sprites/palette.hpp"
#include "gspl_sprites/visual_canon.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

using namespace std::string_view_literals;

[[nodiscard]] bool is_nan_or_inf(double v) { return !std::isfinite(v); }

void add_diag(ValidationResult& r, std::string code, std::string msg) {
  r.diagnostics.push_back({std::move(code), std::move(msg)});
}

[[nodiscard]] std::string d2s(double v) {
  if (std::isinf(v)) return v > 0 ? "inf" : "-inf";
  char buf[32];
  const auto res = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::general, 8);
  return std::string(buf, res.ptr);
}

[[nodiscard]] bool parse_double(std::string_view s, double& out) {
  if (s == "inf") { out = std::numeric_limits<double>::infinity(); return true; }
  if (s == "-inf") { out = -std::numeric_limits<double>::infinity(); return true; }
  double v{};
  const auto res = std::from_chars(s.data(), s.data() + s.size(), v);
  if (res.ec != std::errc{} || res.ptr != s.data() + s.size()) return false;
  out = v;
  return true;
}

[[nodiscard]] bool parse_bool(std::string_view s) { return s == "true" || s == "1"; }

struct MeasureParts {
  std::string kind;
  std::vector<std::string> args;
};

[[nodiscard]] std::optional<MeasureParts> split_measure(std::string_view m) {
  MeasureParts out;
  std::size_t start = 0;
  while (start <= m.size()) {
    const auto colon = m.find(':', start);
    const std::string_view part =
        m.substr(start, colon == std::string_view::npos ? m.size() - start : colon - start);
    if (out.kind.empty()) out.kind = std::string(part);
    else out.args.push_back(std::string(part));
    if (colon == std::string_view::npos) break;
    start = colon + 1;
  }
  return out;
}

// Accumulated world transform of a part (position, accumulated scale, rotation).
struct WorldTransform {
  double x{}, y{};
  double scale_x{1.0};
  double scale_y{1.0};
  double rotation_degrees{};
};

[[nodiscard]] bool part_world_transform(const VisualMorphologyV2& morph, std::string_view id,
                                        WorldTransform& out) {
  const auto it = morph.parts.find(std::string(id));
  if (it == morph.parts.end()) return false;
  const VisualPart& part = it->second;
  if (part.parent.empty()) {
    out = {part.x, part.y, part.size_x, part.size_y, part.rotation_degrees};
    return true;
  }
  WorldTransform parent;
  if (!part_world_transform(morph, part.parent, parent)) return false;
  const double rad = parent.rotation_degrees * 3.14159265358979323846 / 180.0;
  const double c = std::cos(rad), s = std::sin(rad);
  out.x = parent.x + c * part.x - s * part.y;
  out.y = parent.y + s * part.x + c * part.y;
  out.scale_x = parent.scale_x * part.size_x;
  out.scale_y = parent.scale_y * part.size_y;
  out.rotation_degrees = parent.rotation_degrees + part.rotation_degrees;
  return true;
}

[[nodiscard]] bool measure_impl(const VisualCanon& canon, const VisualMorphologyV2& morph,
                                std::string_view measure, double& out) {
  const auto parts = split_measure(measure);
  if (!parts) return false;
  if (parts->kind == "size" && parts->args.size() == 2) {
    // Local size of the named part (relational proportion semantics: ratios
    // are authored between part-local sizes, never accumulated world scale).
    const auto pit = morph.parts.find(parts->args[0]);
    if (pit == morph.parts.end()) return false;
    if (parts->args[1] == "size_x") { out = pit->second.size_x; return true; }
    if (parts->args[1] == "size_y") { out = pit->second.size_y; return true; }
    return false;
  }
  if (parts->kind == "landmark_dist" && parts->args.size() == 2) {
    const auto a = resolve_landmark_position(canon, morph, parts->args[0]);
    const auto b = resolve_landmark_position(canon, morph, parts->args[1]);
    if (!a || !b) return false;
    out = std::sqrt((b->x - a->x) * (b->x - a->x) + (b->y - a->y) * (b->y - a->y));
    return true;
  }
  return false;
}

[[nodiscard]] bool measure_valid(const VisualCanon& canon, std::string_view measure) {
  const auto parts = split_measure(measure);
  if (!parts) return false;
  if (parts->kind == "size" && parts->args.size() == 2) {
    return canon.structures.count(parts->args[0]) != 0 &&
           (parts->args[1] == "size_x" || parts->args[1] == "size_y");
  }
  if (parts->kind == "landmark_dist" && parts->args.size() == 2) {
    return canon.landmarks.count(parts->args[0]) != 0 && canon.landmarks.count(parts->args[1]) != 0;
  }
  return false;
}

[[nodiscard]] std::vector<std::string> split_semicolon(const std::string& s) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= s.size()) {
    const auto sc = s.find(';', start);
    const std::string part = s.substr(start, sc == std::string::npos ? s.size() - start : sc - start);
    if (!part.empty()) out.push_back(part);
    if (sc == std::string::npos) break;
    start = sc + 1;
  }
  return out;
}

} // namespace

/* ── enum tables ── */

std::string_view structure_kind_name(StructureKind kind) noexcept {
  switch (kind) {
    case StructureKind::region: return "region";
    case StructureKind::element: return "element";
    case StructureKind::joint: return "joint";
    case StructureKind::mass: return "mass";
    case StructureKind::appendage: return "appendage";
  }
  return "element";
}

std::optional<StructureKind> structure_kind_from_name(std::string_view name) noexcept {
  if (name == "region") return StructureKind::region;
  if (name == "element") return StructureKind::element;
  if (name == "joint") return StructureKind::joint;
  if (name == "mass") return StructureKind::mass;
  if (name == "appendage") return StructureKind::appendage;
  return std::nullopt;
}

std::string_view relationship_kind_name(RelationshipKind kind) noexcept {
  switch (kind) {
    case RelationshipKind::parent: return "parent";
    case RelationshipKind::child: return "child";
    case RelationshipKind::attachment: return "attachment";
    case RelationshipKind::articulation: return "articulation";
    case RelationshipKind::adjacency: return "adjacency";
    case RelationshipKind::symmetry: return "symmetry";
    case RelationshipKind::correspondence: return "correspondence";
    case RelationshipKind::containment: return "containment";
    case RelationshipKind::overlap: return "overlap";
    case RelationshipKind::surface_ownership: return "surface_ownership";
  }
  return "adjacency";
}

std::optional<RelationshipKind> relationship_kind_from_name(std::string_view name) noexcept {
  if (name == "parent") return RelationshipKind::parent;
  if (name == "child") return RelationshipKind::child;
  if (name == "attachment") return RelationshipKind::attachment;
  if (name == "articulation") return RelationshipKind::articulation;
  if (name == "adjacency") return RelationshipKind::adjacency;
  if (name == "symmetry") return RelationshipKind::symmetry;
  if (name == "correspondence") return RelationshipKind::correspondence;
  if (name == "containment") return RelationshipKind::containment;
  if (name == "overlap") return RelationshipKind::overlap;
  if (name == "surface_ownership") return RelationshipKind::surface_ownership;
  return std::nullopt;
}

std::string_view silhouette_feature_kind_name(SilhouetteFeatureKind kind) noexcept {
  switch (kind) {
    case SilhouetteFeatureKind::mass: return "mass";
    case SilhouetteFeatureKind::anchor: return "anchor";
    case SilhouetteFeatureKind::protrusion: return "protrusion";
    case SilhouetteFeatureKind::concavity: return "concavity";
    case SilhouetteFeatureKind::negative_space: return "negative_space";
  }
  return "mass";
}

std::optional<SilhouetteFeatureKind> silhouette_feature_kind_from_name(std::string_view name) noexcept {
  if (name == "mass") return SilhouetteFeatureKind::mass;
  if (name == "anchor") return SilhouetteFeatureKind::anchor;
  if (name == "protrusion") return SilhouetteFeatureKind::protrusion;
  if (name == "concavity") return SilhouetteFeatureKind::concavity;
  if (name == "negative_space") return SilhouetteFeatureKind::negative_space;
  return std::nullopt;
}

std::string_view expressive_feature_kind_name(ExpressiveFeatureKind kind) noexcept {
  switch (kind) {
    case ExpressiveFeatureKind::eye: return "eye";
    case ExpressiveFeatureKind::brow: return "brow";
    case ExpressiveFeatureKind::mouth: return "mouth";
    case ExpressiveFeatureKind::visor: return "visor";
    case ExpressiveFeatureKind::antenna: return "antenna";
    case ExpressiveFeatureKind::light: return "light";
    case ExpressiveFeatureKind::panel: return "panel";
    case ExpressiveFeatureKind::ear: return "ear";
    case ExpressiveFeatureKind::tail: return "tail";
    case ExpressiveFeatureKind::custom: return "custom";
  }
  return "custom";
}

std::optional<ExpressiveFeatureKind> expressive_feature_kind_from_name(std::string_view name) noexcept {
  if (name == "eye") return ExpressiveFeatureKind::eye;
  if (name == "brow") return ExpressiveFeatureKind::brow;
  if (name == "mouth") return ExpressiveFeatureKind::mouth;
  if (name == "visor") return ExpressiveFeatureKind::visor;
  if (name == "antenna") return ExpressiveFeatureKind::antenna;
  if (name == "light") return ExpressiveFeatureKind::light;
  if (name == "panel") return ExpressiveFeatureKind::panel;
  if (name == "ear") return ExpressiveFeatureKind::ear;
  if (name == "tail") return ExpressiveFeatureKind::tail;
  if (name == "custom") return ExpressiveFeatureKind::custom;
  return std::nullopt;
}

std::string_view identity_invariant_kind_name(IdentityInvariantKind kind) noexcept {
  switch (kind) {
    case IdentityInvariantKind::proportion: return "proportion";
    case IdentityInvariantKind::landmark_ordering: return "landmark_ordering";
    case IdentityInvariantKind::landmark_existence: return "landmark_existence";
    case IdentityInvariantKind::silhouette_anchor: return "silhouette_anchor";
    case IdentityInvariantKind::attachment: return "attachment";
    case IdentityInvariantKind::color_role_topology: return "color_role_topology";
    case IdentityInvariantKind::material_truth: return "material_truth";
    case IdentityInvariantKind::marking_topology: return "marking_topology";
    case IdentityInvariantKind::hard_boundary: return "hard_boundary";
  }
  return "proportion";
}

std::optional<IdentityInvariantKind> identity_invariant_kind_from_name(std::string_view name) noexcept {
  if (name == "proportion") return IdentityInvariantKind::proportion;
  if (name == "landmark_ordering") return IdentityInvariantKind::landmark_ordering;
  if (name == "landmark_existence") return IdentityInvariantKind::landmark_existence;
  if (name == "silhouette_anchor") return IdentityInvariantKind::silhouette_anchor;
  if (name == "attachment") return IdentityInvariantKind::attachment;
  if (name == "color_role_topology") return IdentityInvariantKind::color_role_topology;
  if (name == "material_truth") return IdentityInvariantKind::material_truth;
  if (name == "marking_topology") return IdentityInvariantKind::marking_topology;
  if (name == "hard_boundary") return IdentityInvariantKind::hard_boundary;
  return std::nullopt;
}

/* ── measure helpers ── */

std::string make_landmark_dist_measure(std::string_view a, std::string_view b) {
  return "landmark_dist:" + std::string(a) + ":" + std::string(b);
}

std::string make_size_measure(std::string_view part, std::string_view axis) {
  return "size:" + std::string(part) + ":" + std::string(axis);
}

/* ── validation ── */

ValidationResult validate_visual_canon(const VisualCanon& canon, const CanonLimits& limits) {
  ValidationResult r;
  if (canon.entity_id.empty()) add_diag(r, "CANON_ENTITY_EMPTY", "canon entity_id is empty");
  if (canon.forms.empty()) add_diag(r, "CANON_FORMS_EMPTY", "canon declares no forms");
  if (canon.structures.size() > limits.max_structures)
    add_diag(r, "CANON_STRUCTURE_LIMIT", "too many structures");
  if (canon.proportions.size() > limits.max_proportions)
    add_diag(r, "CANON_PROPORTION_LIMIT", "too many proportion rules");
  if (canon.landmarks.size() > limits.max_landmarks)
    add_diag(r, "CANON_LANDMARK_LIMIT", "too many landmarks");
  if (canon.identity_invariants.size() > limits.max_invariants)
    add_diag(r, "CANON_INVARIANT_LIMIT", "too many invariants");
  if (canon.markings.size() > limits.max_markings)
    add_diag(r, "CANON_MARKING_LIMIT", "too many markings");

  // Structures: refs, cycles, finite geometry, role spelling.
  for (const auto& [id, s] : canon.structures) {
    if (id != s.id) add_diag(r, "CANON_STRUCTURE_ID_MISMATCH", "structure key != id: " + id);
    if (s.role.empty()) add_diag(r, "CANON_STRUCTURE_NO_ROLE", "structure has no role: " + id);
    if (!s.parent.empty() && !canon.structures.count(s.parent))
      add_diag(r, "CANON_STRUCTURE_DANGLING_PARENT", "dangling parent '" + s.parent + "' of " + id);
    if (!visual_shape_kind_from_name(s.primitive))
      add_diag(r, "CANON_STRUCTURE_BAD_PRIMITIVE", "unknown primitive '" + s.primitive + "' on " + id);
    if (!layer_from_name(s.layer))
      add_diag(r, "CANON_STRUCTURE_BAD_LAYER", "unknown layer '" + s.layer + "' on " + id);
    if (!color_role_from_name(s.color_role))
      add_diag(r, "CANON_STRUCTURE_BAD_COLOR_ROLE", "unknown color role '" + s.color_role + "' on " + id);
    if (is_nan_or_inf(s.x) || is_nan_or_inf(s.y) || is_nan_or_inf(s.z) ||
        is_nan_or_inf(s.size_x) || is_nan_or_inf(s.size_y) || is_nan_or_inf(s.size_z) ||
        is_nan_or_inf(s.rotation_degrees))
      add_diag(r, "CANON_STRUCTURE_NONFINITE", "nonfinite geometry on " + id);
    if (s.size_x <= 0.0 || s.size_y <= 0.0 || s.size_z <= 0.0)
      add_diag(r, "CANON_STRUCTURE_BAD_SIZE", "nonpositive size on " + id);
    for (const auto& rel : s.relationships) {
      if (rel.target.empty() || !canon.structures.count(rel.target))
        add_diag(r, "CANON_STRUCTURE_DANGLING_RELATIONSHIP",
                 "dangling relationship on " + id + " -> " + rel.target);
    }
  }
  // Cycle detection over parent edges.
  {
    std::map<std::string, int, std::less<>> state;
    bool cycle = false;
    for (const auto& [id, s] : canon.structures) {
      std::string cur = id;
      while (!cur.empty()) {
        auto st = state.find(cur);
        if (st != state.end() && st->second == 1) { cycle = true; break; }
        state[cur] = 1;
        const auto it = canon.structures.find(cur);
        if (it == canon.structures.end()) break;
        cur = it->second.parent;
      }
      if (cycle) { add_diag(r, "CANON_STRUCTURE_CYCLE", "structural cycle involving " + id); break; }
      for (auto& [k, v] : state) v = 2;
    }
  }

  // Proportions.
  for (const auto& p : canon.proportions) {
    if (p.id.empty()) add_diag(r, "CANON_PROPORTION_NO_ID", "proportion without id");
    if (!measure_valid(canon, p.numerator) || !measure_valid(canon, p.denominator))
      add_diag(r, "CANON_PROPORTION_BAD_MEASURE", "bad measure in proportion '" + p.id + "'");
    if (is_nan_or_inf(p.preferred) || is_nan_or_inf(p.min) || is_nan_or_inf(p.max) ||
        is_nan_or_inf(p.deformation_min) || is_nan_or_inf(p.deformation_max))
      add_diag(r, "CANON_PROPORTION_NONFINITE", "nonfinite bound in proportion '" + p.id + "'");
    if (!(p.min <= p.preferred && p.preferred <= p.max))
      add_diag(r, "CANON_PROPORTION_INVERTED_RANGE",
               "preferred outside [min,max] in '" + p.id + "'");
    if (!(p.deformation_min <= p.min && p.max <= p.deformation_max))
      add_diag(r, "CANON_PROPORTION_BAD_DEFORMATION_RANGE",
               "deformation range must contain [min,max] in '" + p.id + "'");
  }

  // Landmarks.
  for (const auto& [id, lm] : canon.landmarks) {
    if (!canon.structures.count(lm.owner))
      add_diag(r, "CANON_LANDMARK_DANGLING_OWNER", "landmark '" + id + "' owner missing");
    if (!lm.symmetry.empty() && !canon.landmarks.count(lm.symmetry))
      add_diag(r, "CANON_LANDMARK_DANGLING_SYMMETRY", "landmark '" + id + "' symmetry missing");
    if (is_nan_or_inf(lm.ox) || is_nan_or_inf(lm.oy) || is_nan_or_inf(lm.oz))
      add_diag(r, "CANON_LANDMARK_NONFINITE", "nonfinite offset on landmark '" + id + "'");
    for (const auto& o : lm.ordering_after)
      if (!canon.landmarks.count(o))
        add_diag(r, "CANON_LANDMARK_DANGLING_ORDER", "landmark '" + id + "' ordering ref missing: " + o);
  }

  // Silhouette features.
  for (const auto& f : canon.silhouette_features) {
    if (!canon.structures.count(f.structure_ref))
      add_diag(r, "CANON_SILHOUETTE_DANGLING", "silhouette feature '" + f.id + "' structure missing");
    if (is_nan_or_inf(f.min_contribution) || is_nan_or_inf(f.min_separation) ||
        f.min_contribution < 0.0 || f.min_contribution > 1.0)
      add_diag(r, "CANON_SILHOUETTE_BAD_BOUND", "bad bound on silhouette feature '" + f.id + "'");
  }

  // Surfaces, materials, colors.
  for (const auto& s : canon.surfaces)
    if (!canon.structures.count(s.structure_ref))
      add_diag(r, "CANON_SURFACE_DANGLING", "surface '" + s.id + "' structure missing");
  for (const auto& m : canon.materials) {
    if (!canon.structures.count(m.structure_ref))
      add_diag(r, "CANON_MATERIAL_DANGLING", "material '" + m.id + "' structure missing");
    if (m.material_class.empty())
      add_diag(r, "CANON_MATERIAL_NO_CLASS", "material '" + m.id + "' has no class");
  }
  for (const auto& c : canon.color_regions) {
    if (!canon.structures.count(c.structure_ref))
      add_diag(r, "CANON_COLOR_DANGLING", "color region '" + c.id + "' structure missing");
    if (!color_role_from_name(c.color_role))
      add_diag(r, "CANON_COLOR_BAD_ROLE", "color region '" + c.id + "' bad role");
    if (!c.hex.empty() && !parse_hex_color(c.hex))
      add_diag(r, "CANON_COLOR_BAD_HEX", "color region '" + c.id + "' bad hex");
  }

  // Facial regions.
  for (const auto& reg : canon.facial_regions) {
    if (!canon.structures.count(reg.structure_ref))
      add_diag(r, "CANON_FACIAL_DANGLING", "facial region '" + reg.id + "' structure missing");
    for (const auto& f : reg.features) {
      if (f.region_id != reg.id)
        add_diag(r, "CANON_FACIAL_FEATURE_REGION", "feature '" + f.id + "' region mismatch");
      if (is_nan_or_inf(f.gaze) || is_nan_or_inf(f.aperture) || is_nan_or_inf(f.intensity) ||
          is_nan_or_inf(f.rotation) || is_nan_or_inf(f.squash))
        add_diag(r, "CANON_FACIAL_FEATURE_NONFINITE", "feature '" + f.id + "' nonfinite");
    }
  }

  // Markings.
  for (const auto& m : canon.markings) {
    if (!canon.structures.count(m.structure_ref))
      add_diag(r, "CANON_MARKING_DANGLING", "marking '" + m.id + "' structure missing");
    if (is_nan_or_inf(m.opacity) || m.opacity < 0.0 || m.opacity > 1.0)
      add_diag(r, "CANON_MARKING_BAD_OPACITY", "marking '" + m.id + "' bad opacity");
  }

  // Attachments.
  for (const auto& a : canon.attachments)
    if (!canon.structures.count(a.structure_ref))
      add_diag(r, "CANON_ATTACHMENT_DANGLING", "attachment '" + a.id + "' structure missing");

  // Deformation envelopes.
  for (const auto& [id, e] : canon.deformation_envelopes) {
    if (!canon.structures.count(e.structure_id))
      add_diag(r, "CANON_ENVELOPE_DANGLING", "envelope '" + id + "' structure missing");
    if (is_nan_or_inf(e.canonical_value) || is_nan_or_inf(e.allowed_deviation) ||
        is_nan_or_inf(e.action_scale) || is_nan_or_inf(e.expression_scale) ||
        is_nan_or_inf(e.style_scale) || is_nan_or_inf(e.transformation_scale) ||
        is_nan_or_inf(e.hard_boundary))
      add_diag(r, "CANON_ENVELOPE_NONFINITE", "envelope '" + id + "' nonfinite");
    if (e.allowed_deviation < 0.0)
      add_diag(r, "CANON_ENVELOPE_NEGATIVE_ALLOWED", "envelope '" + id + "' negative allowed");
    if (e.action_scale < 0.0 || e.expression_scale < 0.0 || e.style_scale < 0.0 ||
        e.transformation_scale < 0.0)
      add_diag(r, "CANON_ENVELOPE_NEGATIVE_SCALE", "envelope '" + id + "' negative scale");
    if (!(e.allowed_deviation <= e.hard_boundary))
      add_diag(r, "CANON_ENVELOPE_BOUNDARY_ORDER",
               "envelope '" + id + "' allowed must not exceed hard boundary");
  }

  // Resolution features.
  for (const auto& f : canon.resolution_features) {
    if (!canon.structures.count(f.structure_ref))
      add_diag(r, "CANON_RESOLUTION_DANGLING", "resolution feature '" + f.id + "' structure missing");
    if (is_nan_or_inf(f.recognition_importance) || f.recognition_importance < 0.0)
      add_diag(r, "CANON_RESOLUTION_BAD_IMPORTANCE", "resolution feature '" + f.id + "' bad importance");
  }

  // Identity invariants.
  for (const auto& inv : canon.identity_invariants) {
    if (inv.refs.empty())
      add_diag(r, "CANON_INVARIANT_NO_REFS", "invariant '" + inv.id + "' has no refs");
    if (is_nan_or_inf(inv.tolerance) || inv.tolerance < 0.0)
      add_diag(r, "CANON_INVARIANT_BAD_TOLERANCE", "invariant '" + inv.id + "' bad tolerance");
    for (const auto& ref : inv.refs) {
      switch (inv.kind) {
        case IdentityInvariantKind::proportion:
          if (std::none_of(canon.proportions.begin(), canon.proportions.end(),
                           [&](const ProportionRule& p) { return p.id == ref; }))
            add_diag(r, "CANON_INVARIANT_DANGLING_REF", "invariant '" + inv.id + "' ref '" + ref + "'");
          break;
        case IdentityInvariantKind::landmark_ordering:
        case IdentityInvariantKind::landmark_existence:
          if (!canon.landmarks.count(ref))
            add_diag(r, "CANON_INVARIANT_DANGLING_REF", "invariant '" + inv.id + "' landmark '" + ref + "'");
          break;
        case IdentityInvariantKind::marking_topology:
          if (std::none_of(canon.markings.begin(), canon.markings.end(),
                           [&](const MarkingBinding& m) { return m.id == ref; }))
            add_diag(r, "CANON_INVARIANT_DANGLING_REF", "invariant '" + inv.id + "' marking '" + ref + "'");
          break;
        default:
          if (!canon.structures.count(ref))
            add_diag(r, "CANON_INVARIANT_DANGLING_REF", "invariant '" + inv.id + "' structure '" + ref + "'");
          break;
      }
    }
  }

  // Palette hex validation.
  for (const auto& [role, hex] : canon.base_palette)
    if (!parse_hex_color(hex))
      add_diag(r, "CANON_PALETTE_BAD_HEX", "base palette role '" + role + "' bad hex");
  for (const auto& [form, pal] : canon.form_palettes)
    for (const auto& [role, hex] : pal)
      if (!parse_hex_color(hex))
        add_diag(r, "CANON_PALETTE_BAD_HEX", "form palette " + form + "/" + role + " bad hex");

  return r;
}

/* ── canonical serialization (deterministic line grammar) ── */

static void emit_kv(std::string& out, const std::string& key, std::string_view value) {
  out += key;
  out += '=';
  out.append(value);
  out += '\n';
}

static void emit_bool(std::string& out, const std::string& key, bool v) {
  emit_kv(out, key, v ? "true" : "false");
}

static void emit_join(std::string& out, const std::string& key, const std::vector<std::string>& items) {
  std::string joined;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (i) joined += ';';
    joined += items[i];
  }
  emit_kv(out, key, joined);
}

std::string canonicalize_visual_canon(const VisualCanon& canon) {
  std::string out;
  emit_kv(out, "schema", canon.schema.empty() ? "gspl.visual-canon/0.1" : canon.schema);
  emit_kv(out, "entity_id", canon.entity_id);
  emit_kv(out, "name", canon.name);
  emit_kv(out, "rights_class", canon.rights_class);
  emit_kv(out, "provenance", canon.provenance);
  for (const auto& f : canon.forms) emit_kv(out, "form", f);
  for (const auto& [role, hex] : canon.base_palette) emit_kv(out, "base_palette." + role, hex);
  for (const auto& [form, pal] : canon.form_palettes)
    for (const auto& [role, hex] : pal) emit_kv(out, "form_palette." + form + "." + role, hex);

  for (const auto& [id, s] : canon.structures) {
    const std::string p = "structure." + id + ".";
    emit_kv(out, p + "kind", structure_kind_name(s.kind));
    emit_kv(out, p + "parent", s.parent);
    emit_kv(out, p + "role", s.role);
    emit_kv(out, p + "primitive", s.primitive);
    emit_kv(out, p + "layer", s.layer);
    emit_kv(out, p + "material_class", s.material_class);
    emit_kv(out, p + "color_role", s.color_role);
    emit_kv(out, p + "x", d2s(s.x));
    emit_kv(out, p + "y", d2s(s.y));
    emit_kv(out, p + "z", d2s(s.z));
    emit_kv(out, p + "size_x", d2s(s.size_x));
    emit_kv(out, p + "size_y", d2s(s.size_y));
    emit_kv(out, p + "size_z", d2s(s.size_z));
    emit_kv(out, p + "rotation", d2s(s.rotation_degrees));
    emit_kv(out, p + "z_order", std::to_string(s.z_order));
    emit_bool(out, p + "emissive", s.emissive);
    emit_bool(out, p + "silhouette_contribution", s.silhouette_contribution);
    emit_kv(out, p + "bone_id", s.bone_id);
    emit_kv(out, p + "socket_id", s.socket_id);
    emit_kv(out, p + "projection_behavior", s.projection_behavior);
    emit_kv(out, p + "render_group", s.render_group);
    for (const auto& rel : s.relationships)
      emit_kv(out, p + "rel." + std::string(relationship_kind_name(rel.kind)), rel.target);
  }

  for (const auto& p : canon.proportions) {
    const std::string k = "proportion." + p.id + ".";
    emit_kv(out, k + "numerator", p.numerator);
    emit_kv(out, k + "denominator", p.denominator);
    emit_kv(out, k + "preferred", d2s(p.preferred));
    emit_kv(out, k + "min", d2s(p.min));
    emit_kv(out, k + "max", d2s(p.max));
    emit_kv(out, k + "deformation_min", d2s(p.deformation_min));
    emit_kv(out, k + "deformation_max", d2s(p.deformation_max));
    emit_bool(out, k + "hard", p.hard);
    emit_kv(out, k + "scope", p.scope);
  }

  for (const auto& [id, lm] : canon.landmarks) {
    const std::string k = "landmark." + id + ".";
    emit_kv(out, k + "role", lm.role);
    emit_kv(out, k + "owner", lm.owner);
    emit_kv(out, k + "ox", d2s(lm.ox));
    emit_kv(out, k + "oy", d2s(lm.oy));
    emit_kv(out, k + "oz", d2s(lm.oz));
    emit_kv(out, k + "symmetry", lm.symmetry);
    emit_bool(out, k + "silhouette_anchor", lm.silhouette_anchor);
    emit_bool(out, k + "required", lm.required);
    emit_kv(out, k + "deformability", d2s(lm.deformability));
    emit_join(out, k + "ordering_after", lm.ordering_after);
  }

  for (const auto& f : canon.silhouette_features) {
    const std::string k = "silhouette." + f.id + ".";
    emit_kv(out, k + "kind", silhouette_feature_kind_name(f.kind));
    emit_kv(out, k + "priority", std::to_string(f.priority));
    emit_kv(out, k + "structure", f.structure_ref);
    emit_kv(out, k + "min_contribution", d2s(f.min_contribution));
    emit_kv(out, k + "min_separation", d2s(f.min_separation));
  }

  for (const auto& s : canon.surfaces) {
    const std::string k = "surface." + s.id + ".";
    emit_kv(out, k + "structure", s.structure_ref);
    emit_kv(out, k + "role", s.role);
  }

  for (const auto& m : canon.materials) {
    const std::string k = "material." + m.id + ".";
    emit_kv(out, k + "structure", m.structure_ref);
    emit_kv(out, k + "class", m.material_class);
    emit_kv(out, k + "id", m.material_id);
  }

  for (const auto& c : canon.color_regions) {
    const std::string k = "color." + c.id + ".";
    emit_kv(out, k + "structure", c.structure_ref);
    emit_kv(out, k + "role", c.color_role);
    emit_kv(out, k + "form", c.form_id);
    emit_kv(out, k + "hex", c.hex);
  }

  for (const auto& reg : canon.facial_regions) {
    const std::string k = "region." + reg.id + ".";
    emit_kv(out, k + "structure", reg.structure_ref);
    std::vector<std::string> fids;
    for (const auto& f : reg.features) fids.push_back(f.id);
    emit_join(out, k + "features", fids);
    for (const auto& f : reg.features) {
      const std::string fk = "feature." + f.id + ".";
      emit_kv(out, fk + "kind", expressive_feature_kind_name(f.kind));
      emit_kv(out, fk + "region", f.region_id);
      emit_kv(out, fk + "gaze", d2s(f.gaze));
      emit_kv(out, fk + "aperture", d2s(f.aperture));
      emit_kv(out, fk + "intensity", d2s(f.intensity));
      emit_kv(out, fk + "rotation", d2s(f.rotation));
      emit_kv(out, fk + "squash", d2s(f.squash));
    }
  }

  for (const auto& m : canon.markings) {
    const std::string k = "marking." + m.id + ".";
    emit_kv(out, k + "kind", marking_kind_name(m.kind));
    emit_kv(out, k + "structure", m.structure_ref);
    emit_kv(out, k + "role", m.color_role);
    emit_kv(out, k + "opacity", d2s(m.opacity));
    emit_kv(out, k + "scale", d2s(m.scale));
    emit_kv(out, k + "form", m.form_id);
    emit_kv(out, k + "intent", m.intent);
  }

  for (const auto& a : canon.attachments) {
    const std::string k = "attachment." + a.id + ".";
    emit_kv(out, k + "structure", a.structure_ref);
    emit_kv(out, k + "socket", a.socket_id);
    emit_kv(out, k + "role", a.role);
  }

  for (const auto& [id, e] : canon.deformation_envelopes) {
    const std::string k = "envelope." + id + ".";
    emit_kv(out, k + "structure", e.structure_id);
    emit_kv(out, k + "canonical", d2s(e.canonical_value));
    emit_kv(out, k + "allowed", d2s(e.allowed_deviation));
    emit_kv(out, k + "action", d2s(e.action_scale));
    emit_kv(out, k + "expression", d2s(e.expression_scale));
    emit_kv(out, k + "style", d2s(e.style_scale));
    emit_kv(out, k + "transformation", d2s(e.transformation_scale));
    emit_kv(out, k + "boundary", d2s(e.hard_boundary));
    emit_bool(out, k + "rigid", e.rigid);
    emit_bool(out, k + "volume", e.volume_preserving);
  }

  for (const auto& f : canon.resolution_features) {
    const std::string k = "feature_res." + f.id + ".";
    emit_kv(out, k + "structure", f.structure_ref);
    emit_kv(out, k + "priority", std::to_string(f.semantic_priority));
    emit_kv(out, k + "importance", d2s(f.recognition_importance));
    emit_kv(out, k + "min_res", std::to_string(f.min_resolution));
    emit_kv(out, k + "substitution", f.substitution_rule);
    emit_kv(out, k + "merge", f.merge_rule);
    emit_kv(out, k + "omit", f.omission_rule);
  }

  for (const auto& inv : canon.identity_invariants) {
    const std::string k = "invariant." + inv.id + ".";
    emit_kv(out, k + "kind", identity_invariant_kind_name(inv.kind));
    emit_join(out, k + "refs", inv.refs);
    emit_kv(out, k + "tolerance", d2s(inv.tolerance));
    emit_bool(out, k + "hard", inv.hard);
    emit_kv(out, k + "scope", inv.scope);
  }

  return out;
}

/* ── parser (canonical grammar; bounded; fails closed) ── */

namespace {

struct KeyValue {
  std::vector<std::string> path;  // dotted key
  std::string value;
};

[[nodiscard]] bool parse_kv_lines(std::string_view text, std::vector<KeyValue>& out,
                                  std::uint32_t max_lines, std::string& error) {
  std::uint32_t lines = 0;
  std::size_t start = 0;
  while (start <= text.size()) {
    const auto nl = text.find('\n', start);
    std::string_view line = text.substr(start, nl == std::string_view::npos ? text.size() - start : nl - start);
    if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
    if (++lines > max_lines) { error = "too many lines"; return false; }
    if (!line.empty()) {
      const auto eq = line.find('=');
      if (eq == std::string_view::npos) { error = "line without '='"; return false; }
      const std::string_view key = line.substr(0, eq);
      KeyValue kv;
      std::size_t ks = 0;
      while (ks <= key.size()) {
        const auto dot = key.find('.', ks);
        kv.path.push_back(std::string(key.substr(ks, dot == std::string_view::npos ? key.size() - ks : dot - ks)));
        if (dot == std::string_view::npos) break;
        ks = dot + 1;
      }
      kv.value = std::string(line.substr(eq + 1));
      out.push_back(std::move(kv));
    }
    if (nl == std::string_view::npos) break;
    start = nl + 1;
  }
  return true;
}

} // namespace

std::optional<VisualCanon> parse_visual_canon(std::string_view text, const CanonLimits& limits) {
  std::vector<KeyValue> kvs;
  std::string error;
  if (!parse_kv_lines(text, kvs, 8192, error)) return std::nullopt;

  VisualCanon canon;
  bool ok = true;
  const auto fail = [&]() { ok = false; };

  for (const auto& kv : kvs) {
    if (kv.path.empty()) { fail(); break; }
    const std::string& sec = kv.path[0];

    // Canonical ids may contain '.' (e.g. "eye_center.left", "mat.torso",
    // "inv.eye_spacing"), so for those sections the id is the join of the
    // middle path segments and the field is the last segment. Other sections
    // use path[1] as id and path[2] as field. Scalar sections (schema,
    // entity_id, name, rights_class, provenance, form) are single-segment.
    const bool dotted_id =
        sec == "landmark" || sec == "surface" || sec == "material" ||
        sec == "color" || sec == "feature" || sec == "attachment" ||
        sec == "feature_res" || sec == "invariant";
    std::string id;
    std::string field;
    if (kv.path.size() == 1) {
      // Scalar section; no id/field.
    } else if (dotted_id) {
      if (kv.path.size() < 3) { fail(); break; }
      for (std::size_t i = 1; i + 1 < kv.path.size(); ++i) {
        if (i > 1) id += '.';
        id += kv.path[i];
      }
      field = kv.path.back();
    } else {
      id = kv.path[1];
      field = kv.path.size() >= 3 ? kv.path[2] : std::string{};
    }

    if (sec == "schema") canon.schema = kv.value;
    else if (sec == "entity_id") canon.entity_id = kv.value;
    else if (sec == "name") canon.name = kv.value;
    else if (sec == "rights_class") canon.rights_class = kv.value;
    else if (sec == "provenance") canon.provenance = kv.value;
    else if (sec == "form") canon.forms.push_back(kv.value);
    else if (sec == "base_palette" && kv.path.size() == 2) canon.base_palette[id] = kv.value;
    else if (sec == "form_palette" && kv.path.size() >= 3) canon.form_palettes[id][kv.path[2]] = kv.value;
    else if (sec == "structure") {
      auto& s = canon.structures[id];
      s.id = id;
      if (kv.path.size() < 3) { fail(); break; }
      double dv{};
      if (field == "kind") { const auto v = structure_kind_from_name(kv.value); if (!v) fail(); else s.kind = *v; }
      else if (field == "parent") s.parent = kv.value;
      else if (field == "role") s.role = kv.value;
      else if (field == "primitive") s.primitive = kv.value;
      else if (field == "layer") s.layer = kv.value;
      else if (field == "material_class") s.material_class = kv.value;
      else if (field == "color_role") s.color_role = kv.value;
      else if (field == "x" && parse_double(kv.value, dv)) s.x = dv;
      else if (field == "y" && parse_double(kv.value, dv)) s.y = dv;
      else if (field == "z" && parse_double(kv.value, dv)) s.z = dv;
      else if (field == "size_x" && parse_double(kv.value, dv)) s.size_x = dv;
      else if (field == "size_y" && parse_double(kv.value, dv)) s.size_y = dv;
      else if (field == "size_z" && parse_double(kv.value, dv)) s.size_z = dv;
      else if (field == "rotation" && parse_double(kv.value, dv)) s.rotation_degrees = dv;
      else if (field == "z_order") s.z_order = static_cast<std::int32_t>(std::strtol(kv.value.c_str(), nullptr, 10));
      else if (field == "emissive") s.emissive = parse_bool(kv.value);
      else if (field == "silhouette_contribution") s.silhouette_contribution = parse_bool(kv.value);
      else if (field == "bone_id") s.bone_id = kv.value;
      else if (field == "socket_id") s.socket_id = kv.value;
      else if (field == "projection_behavior") s.projection_behavior = kv.value;
      else if (field == "render_group") s.render_group = kv.value;
      else if (field == "rel" && kv.path.size() >= 4) {
        const auto rk = relationship_kind_from_name(kv.path[3]);
        if (!rk) { fail(); } else s.relationships.push_back({*rk, kv.value});
      }
      else fail();
    }
    else if (sec == "proportion") {
      if (canon.proportions.size() >= limits.max_proportions) { fail(); break; }
      auto it = std::find_if(canon.proportions.begin(), canon.proportions.end(),
                             [&](const ProportionRule& p) { return p.id == id; });
      if (it == canon.proportions.end()) { canon.proportions.push_back({}); it = canon.proportions.end() - 1; it->id = id; }
      double dv{};
      if (field == "numerator") it->numerator = kv.value;
      else if (field == "denominator") it->denominator = kv.value;
      else if (field == "preferred" && parse_double(kv.value, dv)) it->preferred = dv;
      else if (field == "min" && parse_double(kv.value, dv)) it->min = dv;
      else if (field == "max" && parse_double(kv.value, dv)) it->max = dv;
      else if (field == "deformation_min" && parse_double(kv.value, dv)) it->deformation_min = dv;
      else if (field == "deformation_max" && parse_double(kv.value, dv)) it->deformation_max = dv;
      else if (field == "hard") it->hard = parse_bool(kv.value);
      else if (field == "scope") it->scope = kv.value;
      else fail();
    }
    else if (sec == "landmark") {
      auto& lm = canon.landmarks[id];
      lm.id = id;
      double dv{};
      if (field == "role") lm.role = kv.value;
      else if (field == "owner") lm.owner = kv.value;
      else if (field == "ox" && parse_double(kv.value, dv)) lm.ox = dv;
      else if (field == "oy" && parse_double(kv.value, dv)) lm.oy = dv;
      else if (field == "oz" && parse_double(kv.value, dv)) lm.oz = dv;
      else if (field == "symmetry") lm.symmetry = kv.value;
      else if (field == "silhouette_anchor") lm.silhouette_anchor = parse_bool(kv.value);
      else if (field == "required") lm.required = parse_bool(kv.value);
      else if (field == "deformability" && parse_double(kv.value, dv)) lm.deformability = dv;
      else if (field == "ordering_after") lm.ordering_after = split_semicolon(kv.value);
      else fail();
    }
    else if (sec == "silhouette") {
      if (canon.silhouette_features.size() >= limits.max_features) { fail(); break; }
      auto it = std::find_if(canon.silhouette_features.begin(), canon.silhouette_features.end(),
                             [&](const SilhouetteFeature& f) { return f.id == id; });
      if (it == canon.silhouette_features.end()) { canon.silhouette_features.push_back({}); it = canon.silhouette_features.end() - 1; it->id = id; }
      double dv{};
      if (field == "kind") { const auto v = silhouette_feature_kind_from_name(kv.value); if (!v) fail(); else it->kind = *v; }
      else if (field == "priority") it->priority = static_cast<std::uint32_t>(std::strtoul(kv.value.c_str(), nullptr, 10));
      else if (field == "structure") it->structure_ref = kv.value;
      else if (field == "min_contribution" && parse_double(kv.value, dv)) it->min_contribution = dv;
      else if (field == "min_separation" && parse_double(kv.value, dv)) it->min_separation = dv;
      else fail();
    }
    else if (sec == "surface") {
      if (canon.surfaces.size() >= limits.max_features) { fail(); break; }
      auto it = std::find_if(canon.surfaces.begin(), canon.surfaces.end(),
                             [&](const SurfaceRegion& s) { return s.id == id; });
      if (it == canon.surfaces.end()) { canon.surfaces.push_back({}); it = canon.surfaces.end() - 1; it->id = id; }
      if (field == "structure") it->structure_ref = kv.value;
      else if (field == "role") it->role = kv.value;
      else fail();
    }
    else if (sec == "material") {
      if (canon.materials.size() >= limits.max_features) { fail(); break; }
      auto it = std::find_if(canon.materials.begin(), canon.materials.end(),
                             [&](const MaterialBinding& m) { return m.id == id; });
      if (it == canon.materials.end()) { canon.materials.push_back({}); it = canon.materials.end() - 1; it->id = id; }
      if (field == "structure") it->structure_ref = kv.value;
      else if (field == "class") it->material_class = kv.value;
      else if (field == "id") it->material_id = kv.value;
      else fail();
    }
    else if (sec == "color") {
      if (canon.color_regions.size() >= limits.max_features) { fail(); break; }
      auto it = std::find_if(canon.color_regions.begin(), canon.color_regions.end(),
                             [&](const ColorRegion& c) { return c.id == id; });
      if (it == canon.color_regions.end()) { canon.color_regions.push_back({}); it = canon.color_regions.end() - 1; it->id = id; }
      if (field == "structure") it->structure_ref = kv.value;
      else if (field == "role") it->color_role = kv.value;
      else if (field == "form") it->form_id = kv.value;
      else if (field == "hex") it->hex = kv.value;
      else fail();
    }
    else if (sec == "region") {
      auto it = std::find_if(canon.facial_regions.begin(), canon.facial_regions.end(),
                             [&](const ExpressiveRegion& r) { return r.id == id; });
      if (it == canon.facial_regions.end()) { canon.facial_regions.push_back({}); it = canon.facial_regions.end() - 1; it->id = id; }
      if (field == "structure") it->structure_ref = kv.value;
      else if (field == "features") {
        for (auto& f : split_semicolon(kv.value))
          it->features.push_back({f, ExpressiveFeatureKind::custom, f, 0.0, 1.0, 0.0, 0.0, 1.0});
      }
      else fail();
    }
    else if (sec == "feature") {
      ExpressiveFeature* feat = nullptr;
      for (auto& r : canon.facial_regions)
        for (auto& f : r.features)
          if (f.id == id) feat = &f;
      if (!feat) { fail(); break; }
      double dv{};
      if (field == "kind") { const auto v = expressive_feature_kind_from_name(kv.value); if (!v) fail(); else feat->kind = *v; }
      else if (field == "region") feat->region_id = kv.value;
      else if (field == "gaze" && parse_double(kv.value, dv)) feat->gaze = dv;
      else if (field == "aperture" && parse_double(kv.value, dv)) feat->aperture = dv;
      else if (field == "intensity" && parse_double(kv.value, dv)) feat->intensity = dv;
      else if (field == "rotation" && parse_double(kv.value, dv)) feat->rotation = dv;
      else if (field == "squash" && parse_double(kv.value, dv)) feat->squash = dv;
      else fail();
    }
    else if (sec == "marking") {
      if (canon.markings.size() >= limits.max_markings) { fail(); break; }
      auto it = std::find_if(canon.markings.begin(), canon.markings.end(),
                             [&](const MarkingBinding& m) { return m.id == id; });
      if (it == canon.markings.end()) { canon.markings.push_back({}); it = canon.markings.end() - 1; it->id = id; }
      double dv{};
      if (field == "kind") { const auto v = marking_kind_from_name(kv.value); if (!v) fail(); else it->kind = *v; }
      else if (field == "structure") it->structure_ref = kv.value;
      else if (field == "role") it->color_role = kv.value;
      else if (field == "opacity" && parse_double(kv.value, dv)) it->opacity = dv;
      else if (field == "scale" && parse_double(kv.value, dv)) it->scale = dv;
      else if (field == "form") it->form_id = kv.value;
      else if (field == "intent") it->intent = kv.value;
      else fail();
    }
    else if (sec == "attachment") {
      if (canon.attachments.size() >= limits.max_features) { fail(); break; }
      auto it = std::find_if(canon.attachments.begin(), canon.attachments.end(),
                             [&](const AttachmentPoint& a) { return a.id == id; });
      if (it == canon.attachments.end()) { canon.attachments.push_back({}); it = canon.attachments.end() - 1; it->id = id; }
      if (field == "structure") it->structure_ref = kv.value;
      else if (field == "socket") it->socket_id = kv.value;
      else if (field == "role") it->role = kv.value;
      else fail();
    }
    else if (sec == "envelope") {
      auto& e = canon.deformation_envelopes[id];
      e.structure_id = id;
      double dv{};
      if (field == "structure") e.structure_id = kv.value;
      else if (field == "canonical" && parse_double(kv.value, dv)) e.canonical_value = dv;
      else if (field == "allowed" && parse_double(kv.value, dv)) e.allowed_deviation = dv;
      else if (field == "action" && parse_double(kv.value, dv)) e.action_scale = dv;
      else if (field == "expression" && parse_double(kv.value, dv)) e.expression_scale = dv;
      else if (field == "style" && parse_double(kv.value, dv)) e.style_scale = dv;
      else if (field == "transformation" && parse_double(kv.value, dv)) e.transformation_scale = dv;
      else if (field == "boundary" && parse_double(kv.value, dv)) e.hard_boundary = dv;
      else if (field == "rigid") e.rigid = parse_bool(kv.value);
      else if (field == "volume") e.volume_preserving = parse_bool(kv.value);
      else fail();
    }
    else if (sec == "feature_res") {
      if (canon.resolution_features.size() >= limits.max_features) { fail(); break; }
      auto it = std::find_if(canon.resolution_features.begin(), canon.resolution_features.end(),
                             [&](const VisualFeature& f) { return f.id == id; });
      if (it == canon.resolution_features.end()) { canon.resolution_features.push_back({}); it = canon.resolution_features.end() - 1; it->id = id; }
      double dv{};
      if (field == "structure") it->structure_ref = kv.value;
      else if (field == "priority") it->semantic_priority = static_cast<std::uint32_t>(std::strtoul(kv.value.c_str(), nullptr, 10));
      else if (field == "importance" && parse_double(kv.value, dv)) it->recognition_importance = dv;
      else if (field == "min_res") it->min_resolution = static_cast<std::uint32_t>(std::strtoul(kv.value.c_str(), nullptr, 10));
      else if (field == "substitution") it->substitution_rule = kv.value;
      else if (field == "merge") it->merge_rule = kv.value;
      else if (field == "omit") it->omission_rule = kv.value;
      else fail();
    }
    else if (sec == "invariant") {
      if (canon.identity_invariants.size() >= limits.max_invariants) { fail(); break; }
      auto it = std::find_if(canon.identity_invariants.begin(), canon.identity_invariants.end(),
                             [&](const IdentityInvariant& i) { return i.id == id; });
      if (it == canon.identity_invariants.end()) { canon.identity_invariants.push_back({}); it = canon.identity_invariants.end() - 1; it->id = id; }
      double dv{};
      if (field == "kind") { const auto v = identity_invariant_kind_from_name(kv.value); if (!v) fail(); else it->kind = *v; }
      else if (field == "refs") it->refs = split_semicolon(kv.value);
      else if (field == "tolerance" && parse_double(kv.value, dv)) it->tolerance = dv;
      else if (field == "hard") it->hard = parse_bool(kv.value);
      else if (field == "scope") it->scope = kv.value;
      else fail();
    }
    else fail();
    if (!ok) break;
  }

  if (!ok) return std::nullopt;
  if (canon.schema.empty()) canon.schema = "gspl.visual-canon/0.1";
  if (!validate_visual_canon(canon, limits).ok()) return std::nullopt;
  return canon;
}

/* ── identity ── */

std::string visual_canon_identity(const VisualCanon& canon) {
  return gspl::sprites::sha256("canon|" + canon.entity_id + "|" + canonicalize_visual_canon(canon));
}

/* ── canon → morphology ── */

namespace {

[[nodiscard]] VisualShapeKind shape_from_primitive(std::string_view primitive) {
  if (const auto k = visual_shape_kind_from_name(primitive)) return *k;
  return VisualShapeKind::ellipse;
}

[[nodiscard]] VisualLayer layer_from_string(std::string_view name) {
  if (const auto l = layer_from_name(name)) return *l;
  return VisualLayer::body;
}

[[nodiscard]] ColorRole role_from_string(std::string_view name) {
  if (const auto r = color_role_from_name(name)) return *r;
  return ColorRole::primary;
}

} // namespace

VisualMorphologyV2 canon_to_morphology(const VisualCanon& canon, std::string_view form_id,
                                       double body_scale) {
  VisualMorphologyV2 out;
  out.schema = "gspl.visual-morphology/0.1";
  out.form_id = std::string(form_id);

  for (const auto& [id, s] : canon.structures) {
    VisualPart p;
    p.id = id;
    p.parent = s.parent;
    p.x = s.x * body_scale;
    p.y = s.y * body_scale;
    p.z = s.z;
    p.size_x = s.size_x * body_scale;
    p.size_y = s.size_y * body_scale;
    p.size_z = s.size_z;
    p.rotation_degrees = s.rotation_degrees;
    p.shape = shape_from_primitive(s.primitive);
    p.color_role = role_from_string(s.color_role);
    p.material_class = s.material_class;
    p.semantic_role = s.role;
    p.bone_id = s.bone_id;
    p.socket_id = s.socket_id;
    p.layer = layer_from_string(s.layer);
    p.z_order = s.z_order;
    p.opacity = 1.0;
    p.emissive = s.emissive;
    p.visible = true;
    p.silhouette_contribution = s.silhouette_contribution;
    p.projection_behavior = s.projection_behavior;
    p.render_group = s.render_group;
    out.parts.emplace(id, std::move(p));
  }

  // Material bindings: attach explicit material ids.
  for (const auto& m : canon.materials) {
    auto it = out.parts.find(m.structure_ref);
    if (it == out.parts.end()) continue;
    it->second.material_class = m.material_class;
    if (!m.material_id.empty()) it->second.material_id = m.material_id;
  }

  // Marking bindings -> Marking structs bound to canon structures. The canon
  // carries marking SEMANTICS (kind, surface, role, opacity, scale); geometry
  // is constructed deterministically per kind from the part's size (never
  // baked pixels, never entity-specific). Empty geometry is never emitted.
  const auto build_marking_geometry = [&](const MarkingBinding& mb, const CanonStructure& s,
                                          Marking& mk) {
    const double hw = std::max(0.5, s.size_x * 0.5);
    const double hy = std::max(0.5, s.size_y * 0.5);
    const std::string seed_text = gspl::sprites::sha256(
        "marking|" + mb.id + "|" + std::string(form_id));
    std::uint64_t seed = seed_text.empty()
        ? 1ull
        : (static_cast<std::uint64_t>(static_cast<unsigned char>(seed_text[0])) << 8);
    if (seed == 0) seed = 1;
    Lcg32 lcg(seed);
    switch (mb.kind) {
      case MarkingKind::stripe:
      case MarkingKind::band:
      case MarkingKind::scar:
      case MarkingKind::tattoo:
      case MarkingKind::fur_marking: {
        constexpr int kSeg = 4;
        for (int i = 0; i <= kSeg; ++i) {
          const double t = 2.0 * static_cast<double>(i) / static_cast<double>(kSeg) - 1.0;
          const double wiggle = (lcg.unit() * 2.0 - 1.0) * hy * 0.12;
          mk.path.push_back({t * hw * 0.85, wiggle});
        }
        mk.widths.assign(mk.path.size(), std::max(0.2, mb.scale * 0.5));
        mk.band_width = std::max(0.2, mb.scale * 0.5);
        break;
      }
      case MarkingKind::spot: {
        const int n = std::clamp(static_cast<int>(std::round(mb.scale * 6.0)), 1, 24);
        mk.spot_radius = std::max(0.15, hw * 0.08);
        for (int i = 0; i < n; ++i) {
          mk.spots.push_back({(lcg.unit() * 2.0 - 1.0) * hw * 0.7,
                              (lcg.unit() * 2.0 - 1.0) * hy * 0.6});
        }
        break;
      }
      case MarkingKind::electrical:
      case MarkingKind::circuit:
      case MarkingKind::vein: {
        constexpr int kSeg = 6;
        for (int i = 0; i <= kSeg; ++i) {
          const double t = 2.0 * static_cast<double>(i) / static_cast<double>(kSeg) - 1.0;
          const double amp = (lcg.unit() * 2.0 - 1.0) * hy * 0.55;
          mk.path.push_back({t * hw * 0.9, amp});
        }
        mk.widths.assign(mk.path.size(), std::max(0.15, mb.scale * 0.35));
        mk.band_width = std::max(0.15, mb.scale * 0.35);
        break;
      }
      case MarkingKind::procedural: {
        const int n = std::clamp(static_cast<int>(std::round(mb.scale * 8.0)), 1, 32);
        mk.spot_radius = std::max(0.1, hw * 0.05);
        mk.density = mb.scale;
        for (int i = 0; i < n; ++i) {
          mk.spots.push_back({(lcg.unit() * 2.0 - 1.0) * hw,
                              (lcg.unit() * 2.0 - 1.0) * hy});
        }
        break;
      }
      default:
        // symbol/gradient_region: no geometry yet (documented future capability).
        break;
    }
  };
  for (const auto& mb : canon.markings) {
    if (!mb.form_id.empty() && mb.form_id != form_id) continue;
    const auto sit = canon.structures.find(mb.structure_ref);
    if (sit == canon.structures.end()) continue;
    Marking mk;
    mk.id = mb.id;
    mk.kind = mb.kind;
    mk.part_id = mb.structure_ref;
    if (const auto role = color_role_from_name(mb.color_role)) mk.color_role = *role;
    mk.opacity = mb.opacity;
    mk.scale = mb.scale;
    mk.seed = [&]() -> std::uint64_t {
      const std::string h = gspl::sprites::sha256("marking-seed|" + mb.id);
      if (h.empty()) return 1ull;
      std::uint64_t v = static_cast<std::uint64_t>(static_cast<unsigned char>(h[0]));
      return v == 0 ? 1ull : v;
    }();
    build_marking_geometry(mb, sit->second, mk);
    out.markings.push_back(std::move(mk));
    auto pit = out.parts.find(mb.structure_ref);
    if (pit != out.parts.end()) pit->second.marking_ids.push_back(mb.id);
  }

  // Layer order: deterministic enum order over present layers.
  {
    std::set<std::string, std::less<>> present;
    for (const auto& [id, p] : out.parts) present.insert(std::string(layer_name(p.layer)));
    for (int li = 0; li <= static_cast<int>(VisualLayer::outline); ++li) {
      const auto l = static_cast<VisualLayer>(li);
      if (present.count(std::string(layer_name(l)))) out.layer_order.push_back(std::string(layer_name(l)));
    }
    for (const auto& name : present)
      if (!layer_from_name(name)) out.layer_order.push_back(name);
  }

  return out;
}

/* ── default canon from morphology (compatibility) ── */

VisualCanon default_canon_from_morphology(const VisualMorphologyV2& morph, std::string_view entity_id) {
  VisualCanon canon;
  canon.schema = "gspl.visual-canon/0.1";
  canon.entity_id = std::string(entity_id);
  if (!morph.form_id.empty()) canon.forms.push_back(morph.form_id);
  for (const auto& [id, p] : morph.parts) {
    CanonStructure s;
    s.id = id;
    s.kind = StructureKind::element;
    s.parent = p.parent;
    s.role = p.semantic_role.empty() ? "element" : p.semantic_role;
    s.primitive = std::string(visual_shape_kind_name(p.shape));
    s.layer = std::string(layer_name(p.layer));
    s.material_class = p.material_class;
    s.color_role = std::string(color_role_name(p.color_role));
    s.x = p.x; s.y = p.y; s.z = p.z;
    s.size_x = p.size_x; s.size_y = p.size_y; s.size_z = p.size_z;
    s.rotation_degrees = p.rotation_degrees;
    s.z_order = p.z_order;
    s.emissive = p.emissive;
    s.silhouette_contribution = p.silhouette_contribution;
    s.bone_id = p.bone_id;
    s.socket_id = p.socket_id;
    s.projection_behavior = p.projection_behavior;
    s.render_group = p.render_group;
    canon.structures.emplace(id, std::move(s));
  }
  return canon;
}

/* ── measures and landmark resolution ── */

std::optional<Vec2> resolve_landmark_position(const VisualCanon& canon,
                                              const VisualMorphologyV2& morph,
                                              std::string_view landmark_id) {
  const auto lit = canon.landmarks.find(std::string(landmark_id));
  if (lit == canon.landmarks.end()) return std::nullopt;
  const VisualLandmark& lm = lit->second;
  WorldTransform t;
  if (!part_world_transform(morph, lm.owner, t)) return std::nullopt;
  const double rad = t.rotation_degrees * 3.14159265358979323846 / 180.0;
  const double c = std::cos(rad), s = std::sin(rad);
  return Vec2{t.x + c * lm.ox - s * lm.oy, t.y + s * lm.ox + c * lm.oy};
}

std::optional<double> measure_value(const VisualCanon& canon, const VisualMorphologyV2& morph,
                                    std::string_view measure) {
  double v{};
  if (!measure_impl(canon, morph, measure, v)) return std::nullopt;
  return v;
}

std::optional<double> proportion_value(const VisualCanon& canon, const VisualMorphologyV2& morph,
                                       std::string_view proportion_id) {
  const auto it = std::find_if(canon.proportions.begin(), canon.proportions.end(),
                               [&](const ProportionRule& p) { return p.id == proportion_id; });
  if (it == canon.proportions.end()) return std::nullopt;
  double num{}, den{};
  if (!measure_impl(canon, morph, it->numerator, num)) return std::nullopt;
  if (!measure_impl(canon, morph, it->denominator, den)) return std::nullopt;
  if (den == 0.0) return std::nullopt;
  return num / den;
}

/* ── deformation enforcement ── */

ValidationResult enforce_deformation_envelopes(const VisualCanon& canon, PerformanceState& perf,
                                               std::string_view action_phase, double expression,
                                               double style_factor, double transformation_factor) {
  ValidationResult r;
  for (auto& m : perf.motions) {
    const auto eit = canon.deformation_envelopes.find(m.part_id);
    if (eit == canon.deformation_envelopes.end()) continue;
    const DeformationEnvelope& e = eit->second;
    double scale = 1.0;
    if (!action_phase.empty()) scale *= e.action_scale;
    if (expression != 0.0) scale *= e.expression_scale;
    scale *= style_factor * transformation_factor;
    const double allowed = e.allowed_deviation * scale;

    const auto clamp_mag = [](double v, double bound) -> double {
      const double a = std::abs(v);
      return a > bound ? (v < 0 ? -bound : bound) : v;
    };
    const double trans = std::sqrt(m.dx * m.dx + m.dy * m.dy);
    const double rot = std::abs(m.rotation_degrees);
    const double scale_dev = std::max(std::abs(m.scale_x - 1.0), std::abs(m.scale_y - 1.0));
    const double requested = std::max({trans, rot, scale_dev});

    if (requested > e.hard_boundary) {
      // Identity boundary exceeded: fail closed, do not silently clamp.
      add_diag(r, "DEFORMATION_HARD_VIOLATION",
               "structure '" + m.part_id + "' requested deviation " + d2s(requested) +
               " exceeds hard boundary " + d2s(e.hard_boundary));
      continue;
    }
    if (e.rigid && requested > allowed) {
      add_diag(r, "DEFORMATION_RIGID_VIOLATION",
               "rigid structure '" + m.part_id + "' requested deviation " + d2s(requested) +
               " exceeds allowed " + d2s(allowed));
      continue;
    }
    if (requested > allowed) {
      add_diag(r, "DEFORMATION_CLAMPED",
               "structure '" + m.part_id + "' deviation " + d2s(requested) +
               " clamped to allowed " + d2s(allowed));
      if (trans > allowed) {
        const double f = allowed / trans;
        m.dx *= f;
        m.dy *= f;
      }
      if (rot > allowed) m.rotation_degrees = clamp_mag(m.rotation_degrees, allowed);
      if (scale_dev > allowed) {
        const double f = allowed / scale_dev;
        m.scale_x = 1.0 + (m.scale_x - 1.0) * f;
        m.scale_y = 1.0 + (m.scale_y - 1.0) * f;
      }
    }
  }
  return r;
}

/* ── identity invariant checks ── */

ValidationResult check_identity_invariants(const VisualCanon& canon, const VisualMorphologyV2& morph) {
  ValidationResult r;
  for (const auto& inv : canon.identity_invariants) {
    if (!inv.scope.empty() && inv.scope.rfind("form:", 0) == 0 && inv.scope != "form:" + morph.form_id)
      continue;  // scope-restricted invariant not applicable to this form
    const std::string prefix = "INVARIANT_" + inv.id + "_";
    switch (inv.kind) {
      case IdentityInvariantKind::proportion: {
        for (const auto& ref : inv.refs) {
          const auto v = proportion_value(canon, morph, ref);
          if (!v) {
            add_diag(r, prefix + "UNMEASURABLE", "proportion '" + ref + "' unmeasurable");
            continue;
          }
          const auto pit = std::find_if(canon.proportions.begin(), canon.proportions.end(),
                                        [&](const ProportionRule& p) { return p.id == ref; });
          if (pit == canon.proportions.end()) continue;
          const bool ok = inv.tolerance > 0.0
              ? std::abs(*v - pit->preferred) <= inv.tolerance
              : (*v >= pit->min && *v <= pit->max);
          if (!ok)
            add_diag(r, prefix + "DRIFT",
                     "proportion '" + ref + "' value " + d2s(*v) + " outside invariant bound");
        }
        break;
      }
      case IdentityInvariantKind::landmark_existence: {
        for (const auto& ref : inv.refs) {
          if (!canon.landmarks.count(ref)) continue;
          if (!resolve_landmark_position(canon, morph, ref))
            add_diag(r, prefix + "MISSING", "required landmark '" + ref + "' not resolvable");
        }
        break;
      }
      case IdentityInvariantKind::landmark_ordering: {
        for (const auto& ref : inv.refs) {
          const auto lit = canon.landmarks.find(ref);
          if (lit == canon.landmarks.end()) continue;
          for (const auto& before : lit->second.ordering_after) {
            const auto a = resolve_landmark_position(canon, morph, before);
            const auto b = resolve_landmark_position(canon, morph, ref);
            if (a && b && a->x > b->x)
              add_diag(r, prefix + "ORDER",
                       "landmark '" + ref + "' precedes '" + before + "' (ordering violated)");
          }
        }
        break;
      }
      case IdentityInvariantKind::silhouette_anchor: {
        for (const auto& ref : inv.refs) {
          bool anchored = false;
          for (const auto& [lid, lm] : canon.landmarks)
            if (lm.owner == ref && lm.silhouette_anchor) anchored = true;
          for (const auto& f : canon.silhouette_features)
            if (f.structure_ref == ref && f.kind == SilhouetteFeatureKind::anchor) anchored = true;
          if (!anchored && !canon.structures.count(ref))
            add_diag(r, prefix + "MISSING", "silhouette anchor structure '" + ref + "' missing");
        }
        break;
      }
      case IdentityInvariantKind::attachment: {
        for (const auto& ref : inv.refs) {
          const auto sit = canon.structures.find(ref);
          if (sit == canon.structures.end()) continue;
          bool attached = !sit->second.parent.empty();
          for (const auto& rel : sit->second.relationships)
            if (rel.kind == RelationshipKind::attachment) attached = true;
          if (!attached)
            add_diag(r, prefix + "UNATTACHED", "structure '" + ref + "' has no attachment");
        }
        break;
      }
      case IdentityInvariantKind::color_role_topology: {
        for (const auto& ref : inv.refs) {
          const auto sit = canon.structures.find(ref);
          if (sit == canon.structures.end()) continue;
          if (!color_role_from_name(sit->second.color_role))
            add_diag(r, prefix + "ROLE", "structure '" + ref + "' has invalid color role");
          const auto pit = morph.parts.find(ref);
          if (pit != morph.parts.end() &&
              std::string(color_role_name(pit->second.color_role)) != sit->second.color_role)
            add_diag(r, prefix + "ROLE_DRIFT", "structure '" + ref + "' color role drifted in morphology");
        }
        break;
      }
      case IdentityInvariantKind::material_truth: {
        for (const auto& ref : inv.refs) {
          const auto sit = canon.structures.find(ref);
          if (sit == canon.structures.end()) continue;
          if (sit->second.material_class.empty())
            add_diag(r, prefix + "MATERIAL", "structure '" + ref + "' has no material");
          const auto pit = morph.parts.find(ref);
          if (pit != morph.parts.end() && pit->second.material_class != sit->second.material_class)
            add_diag(r, prefix + "MATERIAL_DRIFT", "structure '" + ref + "' material drifted in morphology");
        }
        break;
      }
      case IdentityInvariantKind::marking_topology: {
        for (const auto& ref : inv.refs) {
          bool found = false;
          for (const auto& mk : morph.markings) if (mk.id == ref) found = true;
          if (!found)
            add_diag(r, prefix + "MARKING", "marking '" + ref + "' missing in morphology");
        }
        break;
      }
      case IdentityInvariantKind::hard_boundary: {
        for (const auto& ref : inv.refs) {
          const auto eit = canon.deformation_envelopes.find(ref);
          if (eit == canon.deformation_envelopes.end() || !std::isfinite(eit->second.hard_boundary))
            add_diag(r, prefix + "BOUNDARY", "structure '" + ref + "' has no finite hard boundary");
        }
        break;
      }
    }
  }
  return r;
}

} // namespace gspl::sprites::visual
