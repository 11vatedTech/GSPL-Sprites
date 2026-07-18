#include "gspl_sprites/animation3d.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
bool stable_id(std::string_view value) {
  return !value.empty() && value.size() <= 128 &&
         std::ranges::all_of(value, [](unsigned char c) {
           return std::isalnum(c) || c == '.' || c == '_' || c == '-';
         });
}
void add(ValidationResult &result, std::string code, std::string message) {
  result.diagnostics.push_back({std::move(code), std::move(message)});
}
long double square(long double value) { return value * value; }
bool normalized(const std::array<std::int32_t, 4> &value) {
  const auto length = std::sqrt(square(value[0]) + square(value[1]) +
                                square(value[2]) + square(value[3]));
  return length >= 990'000.0L && length <= 1'010'000.0L;
}
template <class Key>
bool ordered_keys(const std::vector<Key> &keys, std::uint32_t duration) {
  if (keys.empty() || keys.front().tick != 0 || keys.back().tick > duration)
    return false;
  for (std::size_t i = 1; i < keys.size(); ++i)
    if (keys[i - 1].tick >= keys[i].tick)
      return false;
  return true;
}
void validate_retarget_skeleton(ValidationResult &result,
                                const Skeleton3d &skeleton,
                                std::string_view role) {
  std::map<std::string, std::optional<std::string>, std::less<>> parents;
  std::size_t roots{};
  if (!stable_id(skeleton.id) || skeleton.joints.empty() ||
      skeleton.joints.size() > 1'024)
    add(result, "SPRITE_3D_RETARGET_SKELETON_INVALID",
        std::string(role) + " skeleton identity or size is invalid");
  for (const auto &joint : skeleton.joints) {
    if (!stable_id(joint.id) ||
        (joint.parent_id && !stable_id(*joint.parent_id)) ||
        !normalized(joint.rotation_xyzw_ppm) ||
        !parents.emplace(joint.id, joint.parent_id).second)
      add(result, "SPRITE_3D_RETARGET_SKELETON_INVALID",
          std::string(role) + " skeleton contains an invalid joint");
    if (!joint.parent_id)
      ++roots;
  }
  if (roots != 1)
    add(result, "SPRITE_3D_RETARGET_SKELETON_ROOT_INVALID",
        std::string(role) + " skeleton must have exactly one root");
  for (const auto &[id, parent] : parents) {
    if (parent && !parents.contains(*parent)) {
      add(result, "SPRITE_3D_RETARGET_SKELETON_PARENT_INVALID",
          std::string(role) + " skeleton references an unknown parent");
      continue;
    }
    std::set<std::string> path;
    auto current = std::optional<std::string>{id};
    while (current) {
      if (!path.insert(*current).second) {
        add(result, "SPRITE_3D_RETARGET_SKELETON_CYCLE",
            std::string(role) + " skeleton contains a cycle");
        break;
      }
      const auto found = parents.find(*current);
      current = found == parents.end() ? std::nullopt : found->second;
    }
  }
}
} // namespace

ValidationResult
validate_animation_clip3d(const AnimationClip3d &clip,
                          const Projection3dDefinition &projection) {
  ValidationResult result;
  const auto projection_validation = validate_projection3d(projection);
  result.diagnostics.insert(result.diagnostics.end(),
                            projection_validation.diagnostics.begin(),
                            projection_validation.diagnostics.end());
  if (!stable_id(clip.id) || clip.ticks_per_second == 0 ||
      clip.ticks_per_second > 1'000 || clip.duration_ticks == 0 ||
      clip.duration_ticks > 86'400'000 || clip.joint_tracks.size() > 4'096 ||
      clip.morph_tracks.size() > 4'096 || clip.events.size() > 65'536)
    add(result, "SPRITE_3D_ANIMATION_INVALID",
        "animation identity, timing, or collection limits are invalid");
  std::set<std::string> joint_ids;
  if (projection.skeleton)
    for (const auto &joint : projection.skeleton->joints)
      joint_ids.insert(joint.id);
  std::set<std::string> tracked_joints;
  for (const auto &track : clip.joint_tracks) {
    if (!joint_ids.contains(track.joint_id) ||
        !tracked_joints.insert(track.joint_id).second ||
        track.keys.size() > 1'000'000 ||
        !ordered_keys(track.keys, clip.duration_ticks))
      add(result, "SPRITE_3D_JOINT_TRACK_INVALID",
          "joint animation track is unknown, duplicate, unordered, or "
          "unbounded");
    for (const auto &key : track.keys)
      if (!normalized(key.pose.rotation_xyzw_ppm) ||
          std::ranges::any_of(key.pose.scale_xyz_ppm, [](auto value) {
            return value <= 0 || value > 16'000'000;
          }))
        add(result, "SPRITE_3D_POSE_INVALID",
            "joint pose rotation or scale is invalid");
  }
  std::set<std::string> morph_ids;
  for (const auto &morph : projection.morph_targets)
    morph_ids.insert(morph.id);
  std::set<std::string> tracked_morphs;
  for (const auto &track : clip.morph_tracks) {
    if (!morph_ids.contains(track.morph_target_id) ||
        !tracked_morphs.insert(track.morph_target_id).second ||
        track.keys.size() > 1'000'000 ||
        !ordered_keys(track.keys, clip.duration_ticks))
      add(result, "SPRITE_3D_MORPH_TRACK_INVALID",
          "morph animation track is unknown, duplicate, unordered, or "
          "unbounded");
    for (const auto &key : track.keys)
      if (key.weight_per_million > 1'000'000)
        add(result, "SPRITE_3D_MORPH_WEIGHT_INVALID",
            "morph animation weight is out of range");
  }
  std::set<std::pair<std::uint32_t, std::string>> events;
  for (const auto &event : clip.events)
    if (!stable_id(event.id) || event.tick > clip.duration_ticks ||
        !events.emplace(event.tick, event.id).second)
      add(result, "SPRITE_3D_ANIMATION_EVENT_INVALID",
          "animation event is invalid or duplicate");
  return result;
}

std::string
canonicalize_animation_clip3d(const AnimationClip3d &clip,
                              const Projection3dDefinition &projection) {
  const auto validation = validate_animation_clip3d(clip, projection);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto joints = clip.joint_tracks;
  auto morphs = clip.morph_tracks;
  auto events = clip.events;
  std::ranges::sort(joints, {}, &JointTrack3d::joint_id);
  std::ranges::sort(morphs, {}, &MorphTrack3d::morph_target_id);
  std::ranges::sort(events, [](const auto &a, const auto &b) {
    return std::tuple{a.tick, a.id} < std::tuple{b.tick, b.id};
  });
  std::ostringstream out;
  out << "schema=gspl.animation3d/0.1\nid=" << clip.id
      << "\nticks_per_second=" << clip.ticks_per_second
      << "\nduration_ticks=" << clip.duration_ticks
      << "\nlooping=" << (clip.looping ? 1 : 0) << '\n';
  for (const auto &track : joints)
    for (const auto &key : track.keys)
      out << "joint_key=" << track.joint_id << '|' << key.tick << '|'
          << key.pose.translation.x << '|' << key.pose.translation.y << '|'
          << key.pose.translation.z << '|' << key.pose.rotation_xyzw_ppm[0]
          << '|' << key.pose.rotation_xyzw_ppm[1] << '|'
          << key.pose.rotation_xyzw_ppm[2] << '|'
          << key.pose.rotation_xyzw_ppm[3] << '|' << key.pose.scale_xyz_ppm[0]
          << '|' << key.pose.scale_xyz_ppm[1] << '|'
          << key.pose.scale_xyz_ppm[2] << '\n';
  for (const auto &track : morphs)
    for (const auto &key : track.keys)
      out << "morph_key=" << track.morph_target_id << '|' << key.tick << '|'
          << key.weight_per_million << '\n';
  for (const auto &event : events)
    out << "event=" << event.id << '|' << event.tick << '\n';
  return out.str();
}

ValidationResult validate_retarget_map3d(const RetargetMap3d &mapping,
                                         const Skeleton3d &source,
                                         const Skeleton3d &target,
                                         bool require_complete_target) {
  ValidationResult result;
  validate_retarget_skeleton(result, source, "source");
  validate_retarget_skeleton(result, target, "target");
  if (!stable_id(mapping.id) || mapping.source_skeleton_id != source.id ||
      mapping.target_skeleton_id != target.id ||
      mapping.translation_scale_per_million == 0 ||
      mapping.translation_scale_per_million > 16'000'000 ||
      mapping.joints.size() > 4'096)
    add(result, "SPRITE_3D_RETARGET_INVALID",
        "retarget map identity, skeletons, scale, or limits are invalid");
  std::map<std::string, std::optional<std::string>, std::less<>> source_parents,
      target_parents;
  for (const auto &joint : source.joints)
    source_parents.emplace(joint.id, joint.parent_id);
  for (const auto &joint : target.joints)
    target_parents.emplace(joint.id, joint.parent_id);
  std::map<std::string, std::string, std::less<>> mapped;
  std::set<std::string> targets;
  for (const auto &joint : mapping.joints)
    if (!source_parents.contains(joint.source_joint_id) ||
        !target_parents.contains(joint.target_joint_id) ||
        !mapped.emplace(joint.source_joint_id, joint.target_joint_id).second ||
        !targets.insert(joint.target_joint_id).second)
      add(result, "SPRITE_3D_RETARGET_JOINT_INVALID",
          "retarget joint is unknown, duplicated, or many-to-one");
  for (const auto &[source_id, target_id] : mapped) {
    const auto &source_parent = source_parents.at(source_id);
    const auto &target_parent = target_parents.at(target_id);
    if (source_parent.has_value() != target_parent.has_value() ||
        (source_parent && (!mapped.contains(*source_parent) ||
                           mapped.at(*source_parent) != *target_parent)))
      add(result, "SPRITE_3D_RETARGET_HIERARCHY_INVALID",
          "retarget mapping does not preserve parent hierarchy");
  }
  if (require_complete_target && targets.size() != target.joints.size())
    add(result, "SPRITE_3D_RETARGET_INCOMPLETE",
        "retarget mapping does not cover the target skeleton");
  return result;
}
} // namespace gspl::sprites
