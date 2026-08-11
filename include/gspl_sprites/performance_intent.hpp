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

/* ── Deterministic Pose/Acting Solver ──
 * Derives part transforms from SEMANTIC intent (not copied explicit
 * motions): line of action, force direction/magnitude, phase, balance,
 * commitment and gaze. The solver is structural-hierarchy driven and
 * entity-agnostic; explicit KeyPose::motions remain as OVERRIDES applied
 * after derived motions. This is what makes PerformanceIntent causal.
 * See docs/architecture/VISUAL_PERFORMANCE_ARCHITECTURE.md. */
struct VisualCanon;

/* Solver result: derived motions + resolved semantic facts. */
struct PoseSolution {
  std::vector<PartMotion> motions;      // derived + key-pose overrides
  double balance_offset_x{0.0};         // solved CoM shift (world units)
  double balance_offset_y{0.0};
  Vec2 gaze_direction;                  // resolved gaze (world)
  double head_rotation_degrees{0.0};    // derived head/gaze orientation
  std::vector<std::string> applied_chains;  // structural chains affected
};

/* Solve a pose from intent semantics against a canon. Explicit key pose
 * motions are applied last (override), preserving determinism. */
[[nodiscard]] PoseSolution solve_pose(const VisualCanon& canon,
                                      const PerformanceIntent& intent,
                                      const KeyPose* key_pose = nullptr);

/* Balance/support reasoning: projected center of mass vs support contacts.
 * Returns diagnostics when a supposedly planted pose has no plausible
 * support. Support policy is data-driven (support roles from canon). */
[[nodiscard]] ValidationResult analyze_balance(const VisualCanon& canon,
                                               const PoseSolution& solution,
                                               const PerformanceIntent& intent);

/* Deterministic lowering: intent + key pose → PerformanceState (the
 * existing compiler input). facing derives from force/gaze; phase and
 * action flow into motion/action phase fields. Uses solve_pose when a
 * canon is provided; otherwise carries explicit key pose motions. */
[[nodiscard]] PerformanceState pose_to_performance_state(const PerformanceIntent& intent,
                                                         const KeyPose* key_pose = nullptr,
                                                         const VisualCanon* canon = nullptr);
[[nodiscard]] ValidationResult validate_performance_intent(const PerformanceIntent& intent);
[[nodiscard]] ValidationResult validate_key_pose(const KeyPose& pose);
[[nodiscard]] std::string canonicalize_performance_intent(const PerformanceIntent& intent);
[[nodiscard]] std::string canonicalize_key_pose(const KeyPose& pose);
[[nodiscard]] std::string performance_intent_identity(const PerformanceIntent& intent);
[[nodiscard]] std::string key_pose_identity(const KeyPose& pose);

} // namespace gspl::sprites::visual
