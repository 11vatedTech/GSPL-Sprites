#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/visual_geometry.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites::visual {

/* ── Cinematic Representation ──
 * Future-facing architecture for camera framing, shot scale, screen
 * direction, perspective, focal hierarchy, camera movement, impact
 * staging, depth and composition. The Visual Canon and Performance
 * architectures are designed so this layer can be added without schema
 * destruction; this spec defines the deterministic representation.
 * See docs/architecture/VISUAL_PERFORMANCE_ARCHITECTURE.md. */

enum class ShotScale { extreme_close_up, close_up, medium, full, wide, establishing };
[[nodiscard]] std::string_view shot_scale_name(ShotScale scale) noexcept;
[[nodiscard]] std::optional<ShotScale> shot_scale_from_name(std::string_view name) noexcept;

enum class CameraMovement { static_, pan, track, dolly, tilt, orbit, handheld, push_in, pull_out };
[[nodiscard]] std::string_view camera_movement_name(CameraMovement move) noexcept;
[[nodiscard]] std::optional<CameraMovement> camera_movement_from_name(std::string_view name) noexcept;

struct CameraSpec {
  Vec2 position;
  double zoom{1.0};                 // canvas units per world unit
  double rotation_degrees{0.0};
  ShotScale shot_scale{ShotScale::medium};
  CameraMovement movement{CameraMovement::static_};
  double focal_priority{0.5};       // 0 = environment, 1 = subject
  double screen_direction{0.0};     // signed world angle of action axis
  double perspective_ratio{0.0};    // 0 = orthographic, 1 = strong perspective
  double parallax_layers{1.0};      // future depth layers factor
  double depth_scale{1.0};          // future z-aware scale
};

struct ImpactStaging {
  Vec2 impact_position;             // where impact lands on screen (0..1 normalized)
  double impact_strength{0.0};      // 0..1
  double freeze_frames{0.0};        // hit-stop duration multiplier
  double shake_intensity{0.0};      // camera shake 0..1
  std::string staged_landmark;      // landmark id the staging anchors to
};

struct CinematicSpec {
  std::string schema{"gspl.cinematic/0.1"};
  std::string entity_id;
  CameraSpec camera;
  std::vector<ImpactStaging> impact_staging;
  std::vector<std::string> focal_hierarchy;  // landmark ids, most important first
};

struct CinematicLimits {
  std::uint32_t max_staging{8};
  std::uint32_t max_focal_hierarchy{32};
};

[[nodiscard]] ValidationResult validate_cinematic(const CinematicSpec& spec, const CinematicLimits& limits = {});
[[nodiscard]] std::string canonicalize_cinematic(const CinematicSpec& spec);
[[nodiscard]] std::string cinematic_identity(const CinematicSpec& spec);

} // namespace gspl::sprites::visual
