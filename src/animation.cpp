#include "gspl_sprites/animation.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <set>

namespace gspl::sprites {
namespace {
bool finite(const Transform2d& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.rotation_degrees) && std::isfinite(value.scale_x) && std::isfinite(value.scale_y) && value.scale_x > 0.0 && value.scale_y > 0.0;
}

double interpolate_angle(double from, double to, double amount) {
  auto delta = std::fmod(to - from, 360.0);
  if (delta > 180.0) delta -= 360.0;
  if (delta < -180.0) delta += 360.0;
  return from + delta * amount;
}

bool compare(double value, Comparison comparison, double threshold) {
  switch (comparison) {
    case Comparison::equal: return value == threshold;
    case Comparison::not_equal: return value != threshold;
    case Comparison::less: return value < threshold;
    case Comparison::less_equal: return value <= threshold;
    case Comparison::greater: return value > threshold;
    case Comparison::greater_equal: return value >= threshold;
  }
  return false;
}
}

ValidationResult validate_rig(const RigDefinition& rig) {
  ValidationResult result; auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  if (rig.id.empty() || rig.id.size() > 128) add("SPRITE_RIG_ID_INVALID", "rig id must contain 1..128 bytes");
  if (rig.bones.empty() || rig.bones.size() > 256) add("SPRITE_RIG_BONE_COUNT", "rig must contain 1..256 bones");
  std::map<std::string, const BoneDefinition*, std::less<>> bones; std::size_t roots = 0;
  for (const auto& bone : rig.bones) {
    if (bone.id.empty() || bone.id.size() > 128 || !bones.emplace(bone.id, &bone).second) add("SPRITE_RIG_BONE_ID", "bone id is empty, too long, or duplicated");
    if (!bone.parent_id) ++roots;
    if (!finite(bone.rest) || !std::isfinite(bone.length) || bone.length < 0.0 || !std::isfinite(bone.limit.minimum_degrees) || !std::isfinite(bone.limit.maximum_degrees) || bone.limit.minimum_degrees > bone.limit.maximum_degrees) add("SPRITE_RIG_BONE_VALUE", "bone transform, length, or limits are invalid: " + bone.id);
  }
  if (roots != 1) add("SPRITE_RIG_ROOT_COUNT", "rig must contain exactly one root bone");
  for (const auto& bone : rig.bones) if (bone.parent_id && (!bones.contains(*bone.parent_id) || *bone.parent_id == bone.id)) add("SPRITE_RIG_PARENT_INVALID", "bone parent is missing or self-referential: " + bone.id);
  for (const auto& bone : rig.bones) {
    std::set<std::string_view> path; const BoneDefinition* cursor = &bone;
    while (cursor != nullptr && cursor->parent_id) {
      if (!path.insert(cursor->id).second) { add("SPRITE_RIG_CYCLE", "bone hierarchy contains a cycle at: " + bone.id); break; }
      const auto parent = bones.find(*cursor->parent_id); cursor = parent == bones.end() ? nullptr : parent->second;
    }
  }
  std::set<std::string> sockets; for (const auto& socket : rig.sockets) if (socket.id.empty() || !sockets.insert(socket.id).second || !bones.contains(socket.bone_id) || !finite(socket.local)) add("SPRITE_RIG_SOCKET_INVALID", "socket is duplicated, malformed, or references an absent bone");
  return result;
}

ValidationResult validate_skin(std::span<const SkinnedVertex> vertices, const RigDefinition& rig) {
  ValidationResult result; auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  if (vertices.empty() || vertices.size() > 4'000'000) add("SPRITE_SKIN_VERTEX_COUNT", "skin must contain 1..4,000,000 vertices");
  std::set<std::string_view> bones; for (const auto& bone : rig.bones) bones.insert(bone.id);
  for (const auto& vertex : vertices) {
    if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || vertex.influences.empty() || vertex.influences.size() > 4) { add("SPRITE_SKIN_VERTEX_INVALID", "vertex position or influence count is invalid"); continue; }
    double sum = 0.0; std::set<std::string_view> used;
    for (const auto& influence : vertex.influences) { if (!bones.contains(influence.bone_id) || !used.insert(influence.bone_id).second || !std::isfinite(influence.weight) || influence.weight <= 0.0 || influence.weight > 1.0) add("SPRITE_SKIN_INFLUENCE_INVALID", "influence is absent, duplicate, or out of range"); sum += influence.weight; }
    if (std::abs(sum - 1.0) > 1e-6) add("SPRITE_SKIN_WEIGHT_SUM", "vertex influence weights must sum to one");
  }
  return result;
}

ValidationResult validate_skeletal_clip(const SkeletalClip& clip, const RigDefinition& rig) {
  ValidationResult result; auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  if (clip.id.empty() || clip.id.size() > 128 || clip.duration_ticks == 0 || clip.duration_ticks > 36000) add("SPRITE_CLIP_HEADER_INVALID", "clip id or duration is invalid");
  std::set<std::string_view> bones; for (const auto& bone : rig.bones) bones.insert(bone.id);
  std::set<std::string> tracks;
  for (const auto& track : clip.tracks) {
    if (!bones.contains(track.bone_id) || !tracks.insert(track.bone_id).second || track.keys.empty() || track.keys.size() > 65536) { add("SPRITE_CLIP_TRACK_INVALID", "track is absent, duplicate, empty, or unbounded"); continue; }
    std::uint32_t previous = 0; bool first = true;
    for (const auto& key : track.keys) { if (!finite(key.transform) || key.tick > clip.duration_ticks || (!first && key.tick <= previous)) add("SPRITE_CLIP_KEY_INVALID", "key is malformed, outside clip, or not strictly ordered"); previous = key.tick; first = false; }
  }
  std::set<std::string> events; for (const auto& [id, tick] : clip.events) if (id.empty() || tick >= clip.duration_ticks || !events.insert(id + ":" + std::to_string(tick)).second) add("SPRITE_CLIP_EVENT_INVALID", "event is empty, duplicate, or outside clip");
  return result;
}

Transform2d sample_track(const BoneTrack& track, std::uint32_t tick) {
  if (track.keys.empty()) throw std::invalid_argument("cannot sample empty track");
  if (tick <= track.keys.front().tick) return track.keys.front().transform;
  if (tick >= track.keys.back().tick) return track.keys.back().transform;
  const auto upper = std::ranges::upper_bound(track.keys, tick, {}, &BoneKeyframe::tick); const auto& right = *upper; const auto& left = *(upper - 1);
  const auto amount = static_cast<double>(tick - left.tick) / static_cast<double>(right.tick - left.tick);
  return {std::lerp(left.transform.x, right.transform.x, amount), std::lerp(left.transform.y, right.transform.y, amount), interpolate_angle(left.transform.rotation_degrees, right.transform.rotation_degrees, amount), std::lerp(left.transform.scale_x, right.transform.scale_x, amount), std::lerp(left.transform.scale_y, right.transform.scale_y, amount)};
}

ValidationResult validate_state_graph(const AnimationStateGraph& graph, std::span<const SkeletalClip> clips) {
  ValidationResult result; auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  std::set<std::string_view> clip_ids; for (const auto& clip : clips) clip_ids.insert(clip.id);
  std::map<std::string, const AnimationState*, std::less<>> states;
  for (const auto& state : graph.states) if (state.id.empty() || !states.emplace(state.id, &state).second || !clip_ids.contains(state.clip_id)) add("SPRITE_STATE_INVALID", "state is empty, duplicate, or references absent clip");
  if (!states.contains(graph.initial_state)) add("SPRITE_STATE_INITIAL_MISSING", "initial state is absent");
  for (const auto& state : graph.states) {
    std::set<std::uint32_t> priorities;
    for (const auto& transition : state.transitions) if (!states.contains(transition.target_state) || transition.parameter.empty() || !std::isfinite(transition.threshold) || !priorities.insert(transition.priority).second || transition.blend_ticks > 36000) add("SPRITE_TRANSITION_INVALID", "transition target, parameter, threshold, priority, or blend is invalid");
  }
  if (states.contains(graph.initial_state)) {
    std::set<std::string> reached{graph.initial_state}; std::deque<std::string> pending{graph.initial_state};
    while (!pending.empty()) { const auto current = pending.front(); pending.pop_front(); for (const auto& transition : states.at(current)->transitions) if (states.contains(transition.target_state) && reached.insert(transition.target_state).second) pending.push_back(transition.target_state); }
    if (reached.size() != states.size()) add("SPRITE_STATE_UNREACHABLE", "animation graph contains unreachable states");
  }
  return result;
}

std::optional<std::string> select_transition(const AnimationState& state, std::uint32_t state_ticks, std::string_view parameter, double value) {
  if (!std::isfinite(value)) return std::nullopt;
  std::vector<const AnimationTransition*> ordered; for (const auto& transition : state.transitions) ordered.push_back(&transition);
  std::ranges::sort(ordered, [](const auto* left, const auto* right){ return left->priority < right->priority; });
  for (const auto* transition : ordered) if (transition->parameter == parameter && state_ticks >= transition->minimum_state_ticks && compare(value, transition->comparison, transition->threshold)) return transition->target_state;
  return std::nullopt;
}

ValidationResult validate_collision_contract(std::span<const CollisionShape> shapes, std::span<const CollisionWindow> windows, const RigDefinition& rig, std::uint32_t ability_active_ticks) {
  ValidationResult result; auto add = [&](std::string code, std::string message){ result.diagnostics.push_back({std::move(code), std::move(message)}); };
  std::set<std::string_view> attachments; for (const auto& bone : rig.bones) attachments.insert(bone.id); for (const auto& socket : rig.sockets) attachments.insert(socket.id);
  std::set<std::string> shape_ids;
  for (const auto& shape : shapes) if (shape.id.empty() || !shape_ids.insert(shape.id).second || !attachments.contains(shape.bone_id) || !std::isfinite(shape.offset_x) || !std::isfinite(shape.offset_y) || !std::isfinite(shape.extent_x) || !std::isfinite(shape.extent_y) || shape.extent_x <= 0.0 || shape.extent_y <= 0.0 || (shape.kind == CollisionKind::circle && shape.extent_x != shape.extent_y)) add("SPRITE_COLLISION_SHAPE_INVALID", "collision shape is malformed, duplicate, or references an absent bone/socket");
  for (const auto& window : windows) if (!shape_ids.contains(window.shape_id) || window.start_tick >= window.end_tick || window.end_tick > ability_active_ticks) add("SPRITE_COLLISION_WINDOW_INVALID", "collision window references absent shape or exceeds ability timing");
  return result;
}
} // namespace gspl::sprites
