#pragma once

#include "gspl_sprites/common.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gspl::sprites {

struct Transform2d {
  double x{};
  double y{};
  double rotation_degrees{};
  double scale_x{1.0};
  double scale_y{1.0};
};

struct JointLimit { double minimum_degrees{-180.0}; double maximum_degrees{180.0}; };
struct BoneDefinition {
  std::string id;
  std::optional<std::string> parent_id;
  Transform2d rest;
  double length{};
  JointLimit limit;
};
struct SocketDefinition { std::string id; std::string bone_id; Transform2d local; };
struct RigDefinition { std::string id; std::vector<BoneDefinition> bones; std::vector<SocketDefinition> sockets; };
[[nodiscard]] ValidationResult validate_rig(const RigDefinition& rig);

struct BoneInfluence { std::string bone_id; double weight{}; };
struct SkinnedVertex { double x{}; double y{}; std::vector<BoneInfluence> influences; };
[[nodiscard]] ValidationResult validate_skin(std::span<const SkinnedVertex> vertices,
                                             const RigDefinition& rig);

struct BoneKeyframe { std::uint32_t tick{}; Transform2d transform; };
struct BoneTrack { std::string bone_id; std::vector<BoneKeyframe> keys; };
struct SkeletalClip {
  std::string id;
  std::uint32_t duration_ticks{};
  bool looping{};
  std::vector<BoneTrack> tracks;
  std::vector<std::pair<std::string, std::uint32_t>> events;
};
[[nodiscard]] ValidationResult validate_skeletal_clip(const SkeletalClip& clip,
                                                      const RigDefinition& rig);
[[nodiscard]] Transform2d sample_track(const BoneTrack& track, std::uint32_t tick);

enum class Comparison { equal, not_equal, less, less_equal, greater, greater_equal };
struct AnimationTransition {
  std::string target_state;
  std::string parameter;
  Comparison comparison{Comparison::equal};
  double threshold{};
  std::uint32_t minimum_state_ticks{};
  std::uint32_t blend_ticks{};
  std::uint32_t priority{};
};
struct AnimationState { std::string id; std::string clip_id; std::vector<AnimationTransition> transitions; };
struct AnimationStateGraph { std::string initial_state; std::vector<AnimationState> states; };
[[nodiscard]] ValidationResult validate_state_graph(const AnimationStateGraph& graph,
                                                    std::span<const SkeletalClip> clips);
[[nodiscard]] std::optional<std::string> select_transition(const AnimationState& state,
                                                           std::uint32_t state_ticks,
                                                           std::string_view parameter,
                                                           double value);

enum class CollisionKind { circle, axis_aligned_box };
struct CollisionShape {
  std::string id;
  CollisionKind kind{};
  std::string bone_id;
  double offset_x{};
  double offset_y{};
  double extent_x{};
  double extent_y{};
};
struct CollisionWindow { std::string shape_id; std::uint32_t start_tick{}; std::uint32_t end_tick{}; bool deals_damage{}; std::string ability_id; };
[[nodiscard]] ValidationResult validate_collision_contract(std::span<const CollisionShape> shapes,
                                                           std::span<const CollisionWindow> windows,
                                                           const RigDefinition& rig,
                                                           std::uint32_t ability_active_ticks);

[[nodiscard]] std::string canonicalize_rig(const RigDefinition& rig);
[[nodiscard]] std::string canonicalize_clips(std::span<const SkeletalClip> clips);
[[nodiscard]] std::string canonicalize_state_graph(const AnimationStateGraph& graph);
[[nodiscard]] std::string canonicalize_collisions(std::span<const CollisionShape> shapes,
                                                  std::span<const CollisionWindow> windows);

} // namespace gspl::sprites
