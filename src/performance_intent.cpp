#include "gspl_sprites/core.hpp"
#include "gspl_sprites/performance_intent.hpp"

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

void add_diag(ValidationResult& r, std::string code, std::string msg) {
  r.diagnostics.push_back({std::move(code), std::move(msg)});
}

} // namespace

PerformanceState pose_to_performance_state(const PerformanceIntent& intent,
                                           const KeyPose* key_pose) {
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
  if (key_pose) {
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
