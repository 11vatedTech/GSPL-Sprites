#pragma once

#include "gspl_sprites/projection3d.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace gspl::sprites {

struct JointPose3d {
  Vector3Micrometers translation;
  std::array<std::int32_t, 4> rotation_xyzw_ppm{0, 0, 0, 1'000'000};
  std::array<std::int32_t, 3> scale_xyz_ppm{1'000'000, 1'000'000, 1'000'000};
};
struct JointKeyframe3d {
  std::uint32_t tick{};
  JointPose3d pose;
};
struct JointTrack3d {
  std::string joint_id;
  std::vector<JointKeyframe3d> keys;
};
struct MorphKeyframe3d {
  std::uint32_t tick{};
  std::uint32_t weight_per_million{};
};
struct MorphTrack3d {
  std::string morph_target_id;
  std::vector<MorphKeyframe3d> keys;
};
struct AnimationEvent3d {
  std::string id;
  std::uint32_t tick{};
};

struct AnimationClip3d {
  std::string id;
  std::uint32_t ticks_per_second{60};
  std::uint32_t duration_ticks{1};
  bool looping{};
  std::vector<JointTrack3d> joint_tracks;
  std::vector<MorphTrack3d> morph_tracks;
  std::vector<AnimationEvent3d> events;
};

struct RetargetJointMapping3d {
  std::string source_joint_id;
  std::string target_joint_id;
};
struct RetargetMap3d {
  std::string id;
  std::string source_skeleton_id;
  std::string target_skeleton_id;
  std::uint32_t translation_scale_per_million{1'000'000};
  std::vector<RetargetJointMapping3d> joints;
};

[[nodiscard]] ValidationResult
validate_animation_clip3d(const AnimationClip3d &clip,
                          const Projection3dDefinition &projection);
[[nodiscard]] std::string
canonicalize_animation_clip3d(const AnimationClip3d &clip,
                              const Projection3dDefinition &projection);
[[nodiscard]] ValidationResult
validate_retarget_map3d(const RetargetMap3d &mapping, const Skeleton3d &source,
                        const Skeleton3d &target,
                        bool require_complete_target = true);

} // namespace gspl::sprites
