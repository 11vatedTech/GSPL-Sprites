#include "gspl_sprites/visual_morphology.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace gspl::sprites::visual {
namespace {

constexpr double kPi = 3.14159265358979323846264338327950288;

std::string escape_json(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) {
          out += "\\u00";
          const char* hex = "0123456789abcdef";
          out += hex[(c >> 4) & 0xF];
          out += hex[c & 0xF];
        } else {
          out += static_cast<char>(c);
        }
    }
  }
  return out;
}

[[nodiscard]] bool looks_like_hex_color(std::string_view s) {
  if (s.size() != 7 || s[0] != '#') return false;
  for (std::size_t i = 1; i < 7; ++i) {
    const char c = s[i];
    const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    if (!hex) return false;
  }
  return true;
}

bool has_cycle(const std::map<std::string, VisualPart, std::less<>>& parts) {
  // DFS from every part; a back edge to an in-stack node is a cycle.
  std::map<std::string, int, std::less<>> state;  // 0 unvisited, 1 in stack, 2 done
  for (const auto& [id, part] : parts) state[id] = 0;
  std::vector<std::string> stack;
  for (const auto& [id, part] : parts) {
    if (state[id] != 0) continue;
    stack.push_back(id);
    while (!stack.empty()) {
      const std::string cur = stack.back();
      if (state[cur] == 0) { state[cur] = 1; }
      const auto it = parts.find(cur);
      if (it == parts.end()) { stack.pop_back(); state[cur] = 2; continue; }
      const std::string& parent = it->second.parent;
      if (parent.empty()) { stack.pop_back(); state[cur] = 2; continue; }
      auto pit = parts.find(parent);
      if (pit == parts.end()) { stack.pop_back(); state[cur] = 2; continue; }
      if (state[parent] == 1) return true;   // cycle
      if (state[parent] == 0) { stack.push_back(parent); continue; }
      stack.pop_back();
      state[cur] = 2;
    }
  }
  return false;
}

} // namespace

std::string_view visual_shape_kind_name(VisualShapeKind kind) noexcept {
  switch (kind) {
    case VisualShapeKind::ellipse: return "ellipse";
    case VisualShapeKind::capsule: return "capsule";
    case VisualShapeKind::rounded_rect: return "rounded_rect";
    case VisualShapeKind::polygon: return "polygon";
    case VisualShapeKind::ring: return "ring";
    case VisualShapeKind::open_path: return "open_path";
    case VisualShapeKind::closed_path: return "closed_path";
  }
  return "unknown";
}

std::optional<VisualShapeKind> visual_shape_kind_from_name(std::string_view name) noexcept {
  for (int i = 0; i <= static_cast<int>(VisualShapeKind::closed_path); ++i) {
    const auto kind = static_cast<VisualShapeKind>(i);
    if (visual_shape_kind_name(kind) == name) return kind;
  }
  return std::nullopt;
}

ValidationResult validate_visual_morphology(const VisualMorphologyV2& morph, std::uint32_t max_parts) {
  ValidationResult result;
  auto add = [&](std::string code, const std::string& msg) {
    result.diagnostics.push_back({std::move(code), msg});
  };

  if (morph.parts.size() > max_parts) {
    add("VISUAL_PART_LIMIT_EXCEEDED",
        "visual part count " + std::to_string(morph.parts.size()) + " exceeds max " + std::to_string(max_parts));
  }
  if (morph.schema != "gspl.visual-morphology/0.1") {
    add("VISUAL_MORPHOLOGY_SCHEMA", "unexpected schema '" + morph.schema + "'");
  }

  std::vector<std::string> part_ids;
  part_ids.reserve(morph.parts.size());
  for (const auto& [id, part] : morph.parts) {
    part_ids.push_back(id);
    if (id.empty()) add("VISUAL_PART_EMPTY_ID", "a part has an empty id");
    if (part.id != id) add("VISUAL_PART_ID_MISMATCH", "part '" + part.id + "' has an id field that differs from its map key '" + id + "'");
    if (part.parent.empty()) continue;
    if (!morph.parts.count(part.parent)) {
      add("VISUAL_PARENT_NOT_FOUND", "part '" + id + "' references unknown parent '" + part.parent + "'");
    }
  }
  if (has_cycle(morph.parts)) {
    add("VISUAL_PARENT_CYCLE", "visual morphology contains a parent cycle");
  }

  for (const auto& [id, part] : morph.parts) {
    const auto finite_ok = [&](double v) { return std::isfinite(v); };
    if (!finite_ok(part.x) || !finite_ok(part.y) || !finite_ok(part.z) ||
        !finite_ok(part.size_x) || !finite_ok(part.size_y) || !finite_ok(part.size_z) ||
        !finite_ok(part.rotation_degrees)) {
      add("VISUAL_PART_NONFINITE", "part '" + id + "' has non-finite transform");
    }
    if (part.size_x < 0.0 || part.size_y < 0.0 || part.size_z < 0.0) {
      add("VISUAL_PART_NEGATIVE_SIZE", "part '" + id + "' has a negative size");
    }
    if (!(part.opacity >= 0.0 && part.opacity <= 1.0)) {
      add("VISUAL_PART_OPACITY", "part '" + id + "' opacity outside [0,1]");
    }
    for (const auto& v : part.polygon) {
      if (!finite_ok(v.x) || !finite_ok(v.y)) {
        add("VISUAL_PART_NONFINITE", "part '" + id + "' polygon is non-finite");
        break;
      }
    }
    for (const auto& w : part.stroke_widths) {
      if (!finite_ok(w) || w < 0.0) {
        add("VISUAL_PART_STROKE_WIDTH", "part '" + id + "' has an invalid stroke width");
        break;
      }
    }
    for (const auto& seg : part.path.segments) {
      const auto& p = seg.p;
      const auto& c1 = seg.c1;
      const auto& c2 = seg.c2;
      if (!finite_ok(p.x) || !finite_ok(p.y) || !finite_ok(c1.x) || !finite_ok(c1.y) ||
          !finite_ok(c2.x) || !finite_ok(c2.y)) {
        add("VISUAL_PART_NONFINITE", "part '" + id + "' path is non-finite");
        break;
      }
    }
    if (part.shape == VisualShapeKind::polygon && part.polygon.size() < 3) {
      add("VISUAL_POLYGON_TOO_SMALL", "part '" + id + "' polygon needs at least 3 vertices");
    }
    for (const auto& mid : part.marking_ids) {
      const bool found = std::any_of(morph.markings.begin(), morph.markings.end(),
                                     [&](const Marking& m) { return m.id == mid; });
      if (!found) add("VISUAL_MARKING_REF_NOT_FOUND", "part '" + id + "' references unknown marking '" + mid + "'");
    }
  }

  const auto mark_result = validate_markings(morph.markings, part_ids);
  for (auto& d : mark_result.diagnostics) result.diagnostics.push_back(std::move(d));

  for (const std::string& layer : morph.layer_order) {
    if (!layer_from_name(layer)) {
      add("VISUAL_UNKNOWN_LAYER", "unknown layer name '" + layer + "' in layer_order");
    }
  }
  return result;
}

std::string canonicalize_visual_morphology(const VisualMorphologyV2& morph) {
  std::ostringstream out;
  out << "{\"schema\":\"" << escape_json(morph.schema) << "\",\"form\":\"" << escape_json(morph.form_id) << "\"";
  out << ",\"layer_order\":[";
  for (std::size_t i = 0; i < morph.layer_order.size(); ++i) {
    if (i) out << ",";
    out << "\"" << escape_json(morph.layer_order[i]) << "\"";
  }
  out << "]";
  out << ",\"parts\":[";
  bool first_part = true;
  for (const auto& [id, part] : morph.parts) {
    if (!first_part) out << ",";
    first_part = false;
    out << "{\"id\":\"" << escape_json(id)
        << "\",\"parent\":\"" << escape_json(part.parent)
        << "\",\"x\":" << part.x << ",\"y\":" << part.y << ",\"z\":" << part.z
        << ",\"sx\":" << part.size_x << ",\"sy\":" << part.size_y << ",\"sz\":" << part.size_z
        << ",\"rot\":" << part.rotation_degrees
        << ",\"shape\":\"" << escape_json(visual_shape_kind_name(part.shape))
        << "\",\"color\":\"" << escape_json(part.explicit_color)
        << "\",\"role\":\"" << escape_json(color_role_name(part.color_role))
        << "\",\"mat_class\":\"" << escape_json(part.material_class)
        << "\",\"mat_id\":\"" << escape_json(part.material_id)
        << "\",\"semantic_role\":\"" << escape_json(part.semantic_role)
        << "\",\"bone\":\"" << escape_json(part.bone_id)
        << "\",\"socket\":\"" << escape_json(part.socket_id)
        << "\",\"layer\":\"" << escape_json(layer_name(part.layer))
        << "\",\"z_order\":" << part.z_order
        << ",\"opacity\":" << part.opacity
        << ",\"emissive\":" << (part.emissive ? "true" : "false")
        << ",\"visible\":" << (part.visible ? "true" : "false")
        << ",\"silhouette\":" << (part.silhouette_contribution ? "true" : "false")
        << ",\"proj\":\"" << escape_json(part.projection_behavior)
        << "\",\"group\":\"" << escape_json(part.render_group)
        << "\",\"ring_inner\":" << part.ring_inner_scale;
    out << ",\"markings\":[";
    for (std::size_t i = 0; i < part.marking_ids.size(); ++i) {
      if (i) out << ",";
      out << "\"" << escape_json(part.marking_ids[i]) << "\"";
    }
    out << "]";
    out << ",\"polygon\":[";
    for (std::size_t i = 0; i < part.polygon.size(); ++i) {
      if (i) out << ",";
      out << part.polygon[i].x << "," << part.polygon[i].y;
    }
    out << "]";
    out << ",\"widths\":[";
    for (std::size_t i = 0; i < part.stroke_widths.size(); ++i) {
      if (i) out << ",";
      out << part.stroke_widths[i];
    }
    out << "]";
    out << ",\"path\":[";
    for (std::size_t i = 0; i < part.path.segments.size(); ++i) {
      if (i) out << ",";
      const PathSegment& seg = part.path.segments[i];
      out << static_cast<int>(seg.kind) << "," << seg.c1.x << "," << seg.c1.y
          << "," << seg.c2.x << "," << seg.c2.y << "," << seg.p.x << "," << seg.p.y;
    }
    out << "]}";
  }
  out << "]";
  out << ",\"markings\":" << canonicalize_markings(morph.markings);
  out << "}";
  return out.str();
}

std::string visual_morphology_identity(const VisualMorphologyV2& morph) {
  return gspl::sprites::sha256(canonicalize_visual_morphology(morph));
}

VisualMorphologyV2 lower_visual_morphology_v1(
    const std::map<std::string, MorphologyPart, std::less<>>& v1,
    std::string_view form_id) {
  VisualMorphologyV2 out;
  out.schema = "gspl.visual-morphology/0.1";
  out.form_id = std::string(form_id);

  const std::string_view kDefaultLayers[] = {
      "rear_effects", "rear_appendages", "body", "front_appendages", "facial",
      "markings", "shadows", "highlights", "front_effects", "emission", "outline"};
  for (const auto& l : kDefaultLayers) out.layer_order.emplace_back(l);

  auto role_contains = [](std::string_view role, std::string_view needle) {
    return role.find(needle) != std::string_view::npos;
  };

  for (const auto& [name, part] : v1) {
    VisualPart vp;
    vp.id = name;
    // v1 seed convention: empty parent OR the literal root sentinel.
    vp.parent = (part.parent == "root") ? std::string{} : part.parent;
    vp.x = part.x;
    vp.y = part.y;
    vp.z = part.z;
    vp.size_x = part.size_x;
    vp.size_y = part.size_y;
    vp.size_z = part.size_z;
    vp.rotation_degrees = part.rotation_degrees;
    vp.bone_id = part.bone_id;
    vp.semantic_role = part.semantic_role;
    vp.z_order = part.z_order;
    vp.emissive = part.emissive;

    // Shape mapping (v1 primitive vocabulary -> v2 shape kinds).
    if (part.primitive == "capsule") vp.shape = VisualShapeKind::capsule;
    else if (part.primitive == "triangle") {
      vp.shape = VisualShapeKind::polygon;
      vp.polygon = {Vec2{-1.0, -1.0}, Vec2{1.0, 0.0}, Vec2{-1.0, 1.0}};
    } else if (part.primitive == "segmented_curve") {
      vp.shape = VisualShapeKind::open_path;
      constexpr int kSegs = 8;
      vp.path.segments.clear();
      for (int i = 0; i < kSegs; ++i) {
        const double t = 2.0 * static_cast<double>(i) / static_cast<double>(kSegs - 1) - 1.0;
        const double y = std::sin(t * kPi) * 0.45;
        vp.path.segments.push_back({SegmentKind::line_to, {}, {}, {t, y}});
      }
      vp.stroke_widths = {0.9, 0.7, 0.55, 0.4, 0.3, 0.25, 0.2, 0.16};
    } else if (part.primitive == "aura_contour") {
      vp.shape = VisualShapeKind::ring;
      vp.ring_inner_scale = 0.55;
    } else {
      vp.shape = VisualShapeKind::ellipse;
    }

    // Color authority: explicit literal color wins; else semantic role.
    if (looks_like_hex_color(part.color)) {
      vp.explicit_color = part.color;
      vp.color_role = ColorRole::custom;
    } else if (part.emissive || part.electrical_marking) {
      vp.color_role = ColorRole::emission;
    } else if (role_contains(part.semantic_role, "eye")) {
      vp.color_role = ColorRole::eye;
    } else {
      vp.color_role = ColorRole::primary;
    }

    // Material class: keyword-driven, generic (not entity-specific).
    std::string role = part.semantic_role;
    if (part.electrical_marking || role_contains(role, "electrical") || role_contains(role, "lightning")) {
      vp.material_class = "electricity";
    } else if (part.emissive || role_contains(role, "aura") || role_contains(role, "energy")) {
      vp.material_class = "energy";
    } else if (role_contains(role, "eye")) {
      vp.material_class = "glass";
    } else if (role_contains(role, "metal") || role_contains(role, "armor") || role_contains(role, "machine")) {
      vp.material_class = "metal";
    } else if (role_contains(role, "fur") || role_contains(role, "hair") || role_contains(role, "tail")) {
      vp.material_class = "fur";
    } else if (role_contains(role, "cloth") || role_contains(role, "cape")) {
      vp.material_class = "cloth";
    } else {
      vp.material_class = "skin";
    }

    // Layer: generic role mapping.
    if (role_contains(role, "aura") || role_contains(role, "energy")) vp.layer = VisualLayer::rear_effects;
    else if (role_contains(role, "eye") || role_contains(role, "facial")) vp.layer = VisualLayer::facial;
    else if (role_contains(role, "tail") || role_contains(role, "wing")) vp.layer = VisualLayer::rear_appendages;
    else vp.layer = VisualLayer::body;

    if (vp.material_class == "energy" || vp.material_class == "electricity") vp.silhouette_contribution = false;

    // Electrical marking: bound to this semantic surface.
    if (part.electrical_marking) {
      Marking m;
      m.id = name + ".electrical";
      m.kind = MarkingKind::electrical;
      m.part_id = name;
      m.color = 0x9BE8FFFF;
      m.color_role = ColorRole::emission;
      m.opacity = 0.9;
      m.seed = 4242u;
      // Deterministic jagged path along the part's local x-axis.
      const double hw = (part.size_x > 0.0) ? part.size_x : 1.0;
      const double hy = (part.size_y > 0.0) ? part.size_y : 1.0;
      Lcg32 lcg(gspl::sprites::sha256(name).empty() ? 1ull : static_cast<std::uint64_t>(gspl::sprites::sha256(name)[0]));
      constexpr int kS = 6;
      for (int i = 0; i <= kS; ++i) {
        const double t = 2.0 * static_cast<double>(i) / static_cast<double>(kS) - 1.0;
        const double amp = (lcg.unit() * 2.0 - 1.0) * hy * 0.6;
        m.path.push_back({t * hw, amp});
      }
      m.widths.assign(m.path.size(), 0.35);
      m.band_width = 0.5;
      out.markings.push_back(std::move(m));
      vp.marking_ids.push_back(name + ".electrical");
    }

    out.parts.emplace(name, std::move(vp));
  }
  return out;
}

} // namespace gspl::sprites::visual
