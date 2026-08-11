#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/performance.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Performance Intent ──
 * The semantic layer above raw animation frames: what an entity is doing
 * and feeling, such that pose, deformation, expression, FX and timing can
 * all be driven from one typed intent. Never animation truth by itself;
 * the runtime's authoritative state is the source of intents. See
 * docs/architecture/VISUAL_PERFORMANCE_ARCHITECTURE.md. */

enum class PerformancePhase {
  anticipation, initiation, extension, contact, impact,
  recoil, follow_through, recovery, settle
};
[[nodiscard]] std::string_view performance_phase_name(PerformancePhase phase) noexcept;
[[nodiscard]] std::optional<PerformancePhase> performance_phase_from_name(std::string_view name) noexcept;

enum class BalanceIntent { planted, shifted, recoil, intentional_imbalance };
[[nodiscard]] std::string_view balance_intent_name(BalanceIntent intent) noexcept;
[[nodiscard]] std::optional<BalanceIntent> balance_intent_from_name(std::string_view name) noexcept;

struct PerformanceIntent {
  std::string schema{"gspl.performance-intent/0.1"};
  std::string action;                              // attack, idle, jump, transform, ...
  PerformancePhase phase{PerformancePhase::extension};
  double commitment{1.0};                          // 0..1
  Vec2 force_direction;                            // unit direction (normalized on use)
  double force_magnitude{0.0};                     // 0..1
  std::string emotion;                             // extensible
  BalanceIntent balance{BalanceIntent::planted};
  std::string gaze_target;                         // landmark id or "" (world direction below)
  Vec2 gaze_direction;
  Vec2 attention;
  double timing_cadence{1.0};                      // spacing/timing multiplier
};

struct KeyPose {
  std::string id;
  std::string action;
  PerformancePhase phase{PerformancePhase::extension};
  double commitment{1.0};
  Vec2 line_of_action_direction;                   // dominant gesture axis
  double line_of_action_curvature{0.0};            // -1..1
  Vec2 center_of_mass;
  std::vector<Vec2> support_contacts;
  BalanceIntent balance{BalanceIntent::planted};
  Vec2 gaze_direction;
  Vec2 force_direction;
  double force_magnitude{0.0};
  std::string emotion;
  std::vector<std::string> silhouette_goal;        // priority landmark ids for this pose
  std::vector<PartMotion> motions;                 // explicit part motions (deterministic)
};

/* Deterministic lowering: intent + key pose → PerformanceState (the
 * existing compiler input). facing derives from force/gaze; phase and
 * action flow into motion/action phase fields. */
[[nodiscard]] PerformanceState pose_to_performance_state(const PerformanceIntent& intent,
                                                         const KeyPose* key_pose = nullptr);
[[nodiscard]] ValidationResult validate_performance_intent(const PerformanceIntent& intent);
[[nodiscard]] ValidationResult validate_key_pose(const KeyPose& pose);
[[nodiscard]] std::string canonicalize_performance_intent(const PerformanceIntent& intent);
[[nodiscard]] std::string canonicalize_key_pose(const KeyPose& pose);
[[nodiscard]] std::string performance_intent_identity(const PerformanceIntent& intent);
[[nodiscard]] std::string key_pose_identity(const KeyPose& pose);

} // namespace gspl::sprites::visual
