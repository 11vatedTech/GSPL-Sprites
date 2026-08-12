#include "gspl_sprites/core.hpp"
#include "gspl_sprites/performance_intent.hpp"
#include "gspl_sprites/visual_canon.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace gspl::sprites::visual {

std::string_view performance_phase_name(PerformancePhase phase) noexcept {
  switch (phase) {
    case PerformancePhase::anticipation: return "anticipation";
    case PerformancePhase::initiation: return "initiation";
    case PerformancePhase::extension: return "extension";
    case PerformancePhase::contact: return "contact";
    case PerformancePhase::impact: return "impact";
    case PerformancePhase::recoil: return "recoil";
    case PerformancePhase::follow_through: return "follow_through";
    case PerformancePhase::recovery: return "recovery";
    case PerformancePhase::settle: return "settle";
  }
  return "extension";
}

std::optional<PerformancePhase> performance_phase_from_name(std::string_view name) noexcept {
  if (name == "anticipation") return PerformancePhase::anticipation;
  if (name == "initiation") return PerformancePhase::initiation;
  if (name == "extension") return PerformancePhase::extension;
  if (name == "contact") return PerformancePhase::contact;
  if (name == "impact") return PerformancePhase::impact;
  if (name == "recoil") return PerformancePhase::recoil;
  if (name == "follow_through") return PerformancePhase::follow_through;
  if (name == "recovery") return PerformancePhase::recovery;
  if (name == "settle") return PerformancePhase::settle;
  return std::nullopt;
}

std::string_view balance_intent_name(BalanceIntent intent) noexcept {
  switch (intent) {
    case BalanceIntent::planted: return "planted";
    case BalanceIntent::shifted: return "shifted";
    case BalanceIntent::recoil: return "recoil";
    case BalanceIntent::intentional_imbalance: return "intentional_imbalance";
  }
  return "planted";
}

std::optional<BalanceIntent> balance_intent_from_name(std::string_view name) noexcept {
  if (name == "planted") return BalanceIntent::planted;
  if (name == "shifted") return BalanceIntent::shifted;
  if (name == "recoil") return BalanceIntent::recoil;
  if (name == "intentional_imbalance") return BalanceIntent::intentional_imbalance;
  return std::nullopt;
}

namespace {

[[nodiscard]] std::string d2s(double v) {
  char buf[32];
  const auto res = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::general, 8);
  return std::string(buf, res.ptr);
}

[[nodiscard]] std::string v2s(const Vec2& v) {
  return d2s(v.x) + "," + d2s(v.y);
}

void add_diag(ValidationResult& r, std::string code, std::string msg,
              DiagnosticSeverity severity = DiagnosticSeverity::error) {
  r.diagnostics.push_back({std::move(code), std::move(msg), severity});
}

[[nodiscard]] double clamp_abs(double v, double bound) {
  const double a = std::abs(v);
  return a > bound ? (v < 0 ? -bound : bound) : v;
}

[[nodiscard]] Vec2 normalized_or(Vec2 v, Vec2 fallback) {
  return v.length_sq() > 0.0 ? v.normalized() : fallback;
}

/* Structural role helpers over canon roles (extensible, data-driven). */
[[nodiscard]] bool is_support_role(std::string_view role) {
  return role == "limb" || role == "leg" || role == "foot" || role == "paw" ||
         role == "tread" || role == "wheel" || role == "appendage";
}
[[nodiscard]] bool is_root_mass_role(std::string_view role) {
  return role == "body-mass" || role == "torso" || role == "chassis" || role == "body";
}

} // namespace

/* ── deterministic pose/acting solver ── */

PoseSolution solve_pose(const VisualCanon& canon, const PerformanceIntent& intent,
                        const KeyPose* key_pose) {
  PoseSolution out;
  const double commitment = std::clamp(intent.commitment, 0.0, 1.0);
  const double force = std::clamp(intent.force_magnitude, 0.0, 1.0);
  const Vec2 force_dir = normalized_or(intent.force_direction, Vec2{1.0, 0.0});
  out.gaze_direction = intent.gaze_direction.length_sq() > 0.0
      ? intent.gaze_direction.normalized() : force_dir;
  out.balance_offset_x = 0.0;
  out.balance_offset_y = 0.0;

  // Deterministic structure ordering: roots first, then children.
  std::vector<std::string> root_masses;
  std::vector<std::string> chains;  // non-root structures with support/root ancestor
  for (const auto& [id, s] : canon.structures) {
    if (is_root_mass_role(s.role)) root_masses.push_back(id);
    else if (!s.parent.empty()) chains.push_back(id);
  }
  std::sort(root_masses.begin(), root_masses.end());
  std::sort(chains.begin(), chains.end());

  // 1. Balance: shifted/recoil translate the root mass (CoM intent).
  double balance_shift = 0.0;
  switch (intent.balance) {
    case BalanceIntent::planted: balance_shift = 0.0; break;
    case BalanceIntent::shifted: balance_shift = 1.0; break;
    case BalanceIntent::recoil: balance_shift = -0.8; break;
    case BalanceIntent::intentional_imbalance: balance_shift = 0.5; break;
  }
  if (std::abs(balance_shift) > 0.0 && !root_masses.empty()) {
    const double shift = balance_shift * commitment * 3.0;
    PartMotion m;
    m.part_id = root_masses.front();
    m.dx = -force_dir.y * shift * 0.2;
    m.dy = shift * 0.6;
    out.balance_offset_x = m.dx;
    out.balance_offset_y = m.dy;
    out.motions.push_back(std::move(m));
  }

  // 2. Line of action + force phase on the dominant root mass: anticipation
  //    compresses along the force axis, extension/contact/impact extend.
  if (!root_masses.empty()) {
    const std::string& root = root_masses.front();
    const double phase_scale =
        (intent.phase == PerformancePhase::anticipation || intent.phase == PerformancePhase::recovery ||
         intent.phase == PerformancePhase::settle) ? -0.5
        : (intent.phase == PerformancePhase::extension || intent.phase == PerformancePhase::contact ||
           intent.phase == PerformancePhase::impact) ? 0.5 : 0.0;
    const double lean = phase_scale * force * commitment * 8.0;
    if (std::abs(lean) > 1e-9) {
      PartMotion m;
      m.part_id = root;
      m.dx = force_dir.x * lean * 0.3;
      m.dy = force_dir.y * lean * 0.3;
      // Rotation about the line of action (into the page) — lean.
      m.rotation_degrees = -force_dir.y * lean * 0.4;
      out.motions.push_back(std::move(m));
      out.applied_chains.push_back(root);
    }
  }

  // 3. Gaze: the canon's explicit gaze_driver structure (or first "head"-role
  //    structure if unset) rotates toward gaze direction. No heuristic.
  {
    const Vec2 g = normalized_or(out.gaze_direction, Vec2{1.0, 0.0});
    out.head_rotation_degrees = clamp_abs(g.y * 18.0 * commitment, 24.0);
    // Determine gaze driver: explicit canon field > role "head" > empty.
    std::string head_id = canon.gaze_driver;
    if (head_id.empty()) {
      for (const auto& [id, s] : canon.structures)
        if (s.role == "head") { head_id = id; break; }
    }
    if (!head_id.empty() && std::abs(out.head_rotation_degrees) > 1e-9) {
      PartMotion m;
      m.part_id = head_id;
      m.rotation_degrees = out.head_rotation_degrees;
      out.motions.push_back(std::move(m));
    }
  }

  // 4. Key pose: explicit motions are OVERRIDES applied after derived
  //    motions (authoritative when provided): KeyPose with the same part_id
  //    REPLACES any previously derived motion (override, not append).
  if (key_pose) {
    for (const auto& m : key_pose->motions) {
      // Remove any derived motion for the same part (override semantics).
      std::erase_if(out.motions, [&](const PartMotion& existing) {
        return existing.part_id == m.part_id;
      });
      out.motions.push_back(m);
    }
    if (!key_pose->emotion.empty()) { /* emotion flows via expression_intent */ }
  }
  std::sort(out.motions.begin(), out.motions.end(),
            [](const PartMotion& a, const PartMotion& b) {
              return a.part_id < b.part_id;
            });
  return out;
}

ValidationResult analyze_balance(const VisualCanon& canon, const PoseSolution& solution,
                                 const PerformanceIntent& intent) {
  ValidationResult r;
  // Support policy: only entities with support roles are ground-checked.
  bool has_support = false;
  for (const auto& [id, s] : canon.structures)
    if (is_support_role(s.role)) { has_support = true; break; }
  if (!has_support) return r;  // flyers/zero-gravity: policy says no check

  // Compute approximate CoM: root mass center + solved balance offset.
  double cx = 0.0, cy = 0.0, wsum = 0.0;
  for (const auto& [id, s] : canon.structures) {
    if (!is_root_mass_role(s.role) && s.role != "facial-feature") continue;
    const double w = std::max(0.5, s.size_x * s.size_y);
    cx += s.x * w; cy += s.y * w; wsum += w;
  }
  if (wsum <= 0.0) return r;
  cx /= wsum; cy /= wsum;
  cx += solution.balance_offset_x;
  cy += solution.balance_offset_y;

  // Support polygon: min/max x of support structures (planted ground).
  double min_x = std::numeric_limits<double>::max(), max_x = -std::numeric_limits<double>::max();
  for (const auto& [id, s] : canon.structures) {
    if (!is_support_role(s.role)) continue;
    min_x = std::min(min_x, s.x); max_x = std::max(max_x, s.x);
  }
  const double planted = (intent.balance == BalanceIntent::planted ||
                          intent.balance == BalanceIntent::shifted);
  if (planted && max_x <= min_x) {
    add_diag(r, "BALANCE_NO_SUPPORT", "planted pose has no plausible support polygon");
    return r;
  }
  if (planted && (cx < min_x || cx > max_x)) {
    add_diag(r, "BALANCE_OUTSIDE_SUPPORT",
             "center of mass x=" + d2s(cx) + " outside support polygon [" +
             d2s(min_x) + ", " + d2s(max_x) + "]");
  }
  return r;
}

PerformanceState pose_to_performance_state(const PerformanceIntent& intent,
                                           const KeyPose* key_pose,
                                           const VisualCanon* canon) {
  PerformanceState out;
  out.motion_phase = std::string(performance_phase_name(intent.phase));
  out.action_phase = intent.action;
  out.expression_intent = intent.emotion;
  out.impact = std::clamp(intent.force_magnitude, 0.0, 1.0);
  out.secondary_motion = 0.0;
  const Vec2 dir = intent.force_direction.length_sq() > 0.0
      ? intent.force_direction.normalized() : Vec2{1.0, 0.0};
  out.velocity_intent = dir * intent.force_magnitude;
  out.facing = (intent.gaze_direction.x < 0.0 || (intent.gaze_direction.x == 0.0 && dir.x < 0.0))
      ? "left" : "right";
  if (canon) {
    // Canon-aware: derive motions from semantics (the actual performance
    // reasoning path). Key pose explicit motions override derived ones.
    const PoseSolution solution = solve_pose(*canon, intent, key_pose);
    out.motions = solution.motions;
    out.velocity_intent = dir * intent.force_magnitude;
  } else if (key_pose) {
    for (const auto& m : key_pose->motions) out.motions.push_back(m);
    if (!key_pose->emotion.empty()) out.expression_intent = key_pose->emotion;
  }
  return out;
}

ValidationResult validate_performance_intent(const PerformanceIntent& intent) {
  ValidationResult r;
  if (intent.action.empty()) add_diag(r, "INTENT_NO_ACTION", "performance intent has no action");
  if (!(intent.commitment >= 0.0 && intent.commitment <= 1.0))
    add_diag(r, "INTENT_BAD_COMMITMENT", "commitment outside [0,1]");
  if (intent.force_magnitude < 0.0 || intent.force_magnitude > 1.0)
    add_diag(r, "INTENT_BAD_FORCE", "force magnitude outside [0,1]");
  if (std::isnan(intent.force_direction.x) || std::isnan(intent.force_direction.y) ||
      std::isnan(intent.gaze_direction.x) || std::isnan(intent.gaze_direction.y) ||
      std::isnan(intent.attention.x) || std::isnan(intent.attention.y))
    add_diag(r, "INTENT_NONFINITE", "nonfinite direction vector");
  if (intent.timing_cadence <= 0.0)
    add_diag(r, "INTENT_BAD_CADENCE", "timing cadence must be positive");
  return r;
}

ValidationResult validate_key_pose(const KeyPose& pose) {
  ValidationResult r;
  if (pose.id.empty()) add_diag(r, "KEYPOSE_NO_ID", "key pose has no id");
  if (!(pose.commitment >= 0.0 && pose.commitment <= 1.0))
    add_diag(r, "KEYPOSE_BAD_COMMITMENT", "commitment outside [0,1]");
  if (pose.force_magnitude < 0.0 || pose.force_magnitude > 1.0)
    add_diag(r, "KEYPOSE_BAD_FORCE", "force magnitude outside [0,1]");
  for (const auto& v : pose.support_contacts)
    if (std::isnan(v.x) || std::isnan(v.y))
      add_diag(r, "KEYPOSE_NONFINITE", "nonfinite support contact");
  for (const auto& m : pose.motions)
    if (m.part_id.empty()) add_diag(r, "KEYPOSE_MOTION_NO_PART", "part motion without part id");
  return r;
}

std::string canonicalize_performance_intent(const PerformanceIntent& intent) {
  std::string out;
  out += "schema=" + (intent.schema.empty() ? std::string("gspl.performance-intent/0.1") : intent.schema) + "\n";
  out += "action=" + intent.action + "\n";
  out += "phase=" + std::string(performance_phase_name(intent.phase)) + "\n";
  out += "commitment=" + d2s(intent.commitment) + "\n";
  out += "force_direction=" + v2s(intent.force_direction) + "\n";
  out += "force_magnitude=" + d2s(intent.force_magnitude) + "\n";
  out += "emotion=" + intent.emotion + "\n";
  out += "balance=" + std::string(balance_intent_name(intent.balance)) + "\n";
  out += "gaze_target=" + intent.gaze_target + "\n";
  out += "gaze_direction=" + v2s(intent.gaze_direction) + "\n";
  out += "attention=" + v2s(intent.attention) + "\n";
  out += "timing_cadence=" + d2s(intent.timing_cadence) + "\n";
  return out;
}

std::string canonicalize_key_pose(const KeyPose& pose) {
  std::string out;
  out += "id=" + pose.id + "\n";
  out += "action=" + pose.action + "\n";
  out += "phase=" + std::string(performance_phase_name(pose.phase)) + "\n";
  out += "commitment=" + d2s(pose.commitment) + "\n";
  out += "line_of_action_direction=" + v2s(pose.line_of_action_direction) + "\n";
  out += "line_of_action_curvature=" + d2s(pose.line_of_action_curvature) + "\n";
  out += "center_of_mass=" + v2s(pose.center_of_mass) + "\n";
  for (const auto& c : pose.support_contacts) out += "support_contact=" + v2s(c) + "\n";
  out += "balance=" + std::string(balance_intent_name(pose.balance)) + "\n";
  out += "gaze_direction=" + v2s(pose.gaze_direction) + "\n";
  out += "force_direction=" + v2s(pose.force_direction) + "\n";
  out += "force_magnitude=" + d2s(pose.force_magnitude) + "\n";
  out += "emotion=" + pose.emotion + "\n";
  for (const auto& g : pose.silhouette_goal) out += "silhouette_goal=" + g + "\n";
  for (const auto& m : pose.motions) {
    out += "motion=" + m.part_id + ";" + d2s(m.dx) + ";" + d2s(m.dy) + ";" +
           d2s(m.rotation_degrees) + ";" + d2s(m.scale_x) + ";" + d2s(m.scale_y) + "\n";
  }
  return out;
}

std::string performance_intent_identity(const PerformanceIntent& intent) {
  return gspl::sprites::sha256("intent|" + canonicalize_performance_intent(intent));
}

std::string key_pose_identity(const KeyPose& pose) {
  return gspl::sprites::sha256("keypose|" + canonicalize_key_pose(pose));
}

} // namespace gspl::sprites::visual
