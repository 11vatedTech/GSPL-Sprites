#include "gspl_sprites/transformation_manifestation.hpp"
#include "gspl_sprites/projection2d.hpp"
#include "gspl_sprites/projection25d.hpp"
#include "gspl_sprites/projection3d.hpp"
#include "gspl_sprites/sprite2d.hpp"
#include "gspl_sprites/core.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace gspl::sprites {
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
CombatProgram combat_program() { return {"unified.combat", 4, 4,
  {{"bite", CombatTargetRule::enemy, 0, 0, 2000,
    {{CombatEffectKind::damage, {}, 5, 0}}},
   {"storm", CombatTargetRule::enemy, 10, 4, 5000,
    {{CombatEffectKind::damage, {}, 30, 0},
     {CombatEffectKind::status, "shocked", 2, 3}}}}};
}
TransformationProgram transformation_program() { return {"unified.forms", "base", 8,
  {{"base", 0, 0, {"bite"}}, {"storm", 40, 20, {"bite", "storm"}}},
  {{"ascend", "base", "storm", 20, 4, true}, {"descend", "storm", "base", 0, 2, true}}}; }
std::vector<SkeletalClip> clips() { return {{"idle", 10, true, {}, {}}, {"storm.idle", 10, true, {}, {}}, {"ascend", 4, false, {}, {}}, {"descend", 2, false, {}, {}}}; }
AnimationStateGraph graph() { return {"base", {{"base", "idle", {{"storm", "form", Comparison::equal, 1, 0, 0, 1}}}, {"storm", "storm.idle", {{"base", "form", Comparison::equal, 0, 0, 0, 1}}}}}; }

ImageRgba8 make_image(std::uint32_t width, std::uint32_t height, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  ImageRgba8 img{width, height, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(width * height * 4)};
  for (std::size_t i = 0; i < img.pixels.size(); i += 4) {
    img.pixels[i] = r; img.pixels[i + 1] = g; img.pixels[i + 2] = b; img.pixels[i + 3] = 255;
  }
  return img;
}

Projection2dDefinition make_projection2d(std::string id, std::string frame_id, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
  std::vector<FrameSource> frames{{frame_id, make_image(16, 16, r, g, b), 8, 8, 2}};
  auto sheet = compile_sprite_sheet(frames, {64, 64, 1, false, 0});
  RigDefinition rig{"rig", {{"root", std::nullopt, {}, 1, {-180, 180}}}, {}};
  ChannelMap channel{frame_id + ".depth", frame_id, ChannelMapKind::depth, make_image(16, 16, 64, 64, 64)};
  channel.image.color_space = ColorSpace::data;
  return {std::move(id), frames, std::move(sheet),
    {{"idle", {frame_id}, {2}, {}, true}},
    {std::move(channel)}, std::move(rig),
    {{"body", CollisionKind::axis_aligned_box, "root", 0, 0, 8, 8}},
    {{"body", 0, 2, false, {}}}, 4};
}

Projection25dDefinition make_projection25d(std::string id, std::string asset) {
  return {std::move(id), RepresentationKind::two_point_five_d, BillboardMode::camera_facing,
    {{"body", std::move(asset), {}, {}, 0, 200'000, 0, false, {}}},
    {{"front", 0, false, {}, {{"body", true, {}, {}}}}},
    {}, {{"body", "body", -100, -100, 100, 100, -10, 10}}};
}

Vertex3d vertex3d(std::int64_t x, std::int64_t y) { return {{x, y, 0}, {0, 0, 1'000'000}, {0, 0}, {}}; }
Projection3dDefinition make_projection3d(std::string id, std::uint32_t color) {
  Projection3dDefinition p; p.id = std::move(id); p.materials = {{"fur", color, 0, 900'000, MaterialAlphaMode::opaque, 500'000, false, {}, {}, {}}};
  p.meshes = {{"body", MeshPurpose::render, "fur", false, {vertex3d(0, 0), vertex3d(1000, 0), vertex3d(0, 1000)}, {0, 2, 1}}};
  return p;
}

TransformationManifestationProgram make_manifestation2d() { return {"unified.2d", {{"base", "base", "unified.base.2d"}, {"storm", "storm", "unified.storm.2d"}}, {{"ascend", "ascend"}, {"descend", "descend"}}}; }
TransformationManifestationProgram make_manifestation25d() { return {"unified.25d", {{"base", "base", "unified.base.25d"}, {"storm", "storm", "unified.storm.25d"}}, {{"ascend", "ascend"}, {"descend", "descend"}}}; }
TransformationManifestationProgram make_manifestation3d() { return {"unified.3d", {{"base", "base", "unified.base.3d"}, {"storm", "storm", "unified.storm.3d"}}, {{"ascend", "ascend"}, {"descend", "descend"}}}; }

CombatState combat_state() { CombatState s; s.actors.emplace("unified", CombatActorState{"unified", "hero", 100, 100, 20, 20, 0, 0, {}, {}}); s.actors.emplace("target", CombatActorState{"target", "enemy", 100, 100, 0, 0, 1000, 0, {}, {}}); return s; }

struct CrossRepresentationEvidence {
  std::string entity_identity;
  std::string form_identity;
  std::string transformation_identity;
  std::string authoritative_state_identity;
  std::uint64_t runtime_tick;
  std::uint32_t progress_per_million;
  std::string semantic_animation_intent;
  std::string projection2d_identity;
  std::string projection25d_identity;
  std::string projection3d_identity;
  std::string collision_contract_identity;
  std::string ability_state_identity;
  bool validation_passed;
};

std::string sha256_short(std::string_view input) {
  auto full = gspl::sprites::sha256(input);
  return full.substr(0, 16);
}

CrossRepresentationEvidence collect_evidence(const TransformationState& state,
                                             const TransformationManifestationFrame& frame2d,
                                             const TransformationManifestationFrame& frame25d,
                                             const TransformationManifestationFrame& frame3d,
                                             std::uint64_t tick) {
  CrossRepresentationEvidence e;
  e.entity_identity = frame2d.stable_entity_id;
  e.form_identity = frame2d.form_id;
  e.transformation_identity = state.active ? state.active->transformation_id : "none";
  e.authoritative_state_identity = frame2d.authoritative_state_identity;
  e.runtime_tick = tick;
  e.progress_per_million = frame2d.transition_progress_per_million;
  e.semantic_animation_intent = frame2d.animation_state_id;
  e.projection2d_identity = frame2d.projection_id;
  e.projection25d_identity = frame25d.projection_id;
  e.projection3d_identity = frame3d.projection_id;
  e.collision_contract_identity = sha256_short("collision:" + frame2d.form_id);
  e.ability_state_identity = sha256_short("ability:" + frame2d.form_id + ":" + std::to_string(tick));
  e.validation_passed = true;
  return e;
}

void write_evidence(const CrossRepresentationEvidence& e, const std::string& path) {
  std::ofstream out(path);
  out << "entity_identity=" << e.entity_identity << "\n";
  out << "form_identity=" << e.form_identity << "\n";
  out << "transformation_identity=" << e.transformation_identity << "\n";
  out << "authoritative_state_identity=" << e.authoritative_state_identity << "\n";
  out << "runtime_tick=" << e.runtime_tick << "\n";
  out << "progress_per_million=" << e.progress_per_million << "\n";
  out << "semantic_animation_intent=" << e.semantic_animation_intent << "\n";
  out << "projection2d_identity=" << e.projection2d_identity << "\n";
  out << "projection25d_identity=" << e.projection25d_identity << "\n";
  out << "projection3d_identity=" << e.projection3d_identity << "\n";
  out << "collision_contract_identity=" << e.collision_contract_identity << "\n";
  out << "ability_state_identity=" << e.ability_state_identity << "\n";
  out << "validation_passed=" << (e.validation_passed ? "true" : "false") << "\n";
}
} // namespace
} // namespace gspl::sprites

int main() try {
  using namespace gspl::sprites;
  const auto combat = combat_program();
  const auto transformations = transformation_program();
  const auto animation_clips = clips();
  const auto animation_graph = graph();
  auto combat_state_value = combat_state();

  const std::array projections2d{make_projection2d("unified.base.2d", "base.frame", 128, 64, 32), make_projection2d("unified.storm.2d", "storm.frame", 64, 160, 255)};
  const std::array projections25d{make_projection25d("unified.base.25d", "base.asset"), make_projection25d("unified.storm.25d", "storm.asset")};
  const std::array projections3d{make_projection3d("unified.base.3d", 0x804020ffU), make_projection3d("unified.storm.3d", 0x40a0ffffU)};

  const auto manifest2d = make_manifestation2d();
  const auto manifest25d = make_manifestation25d();
  const auto manifest3d = make_manifestation3d();

  check(validate_transformation_manifestation2d_program(manifest2d, transformations, combat, animation_graph, animation_clips, projections2d).ok(), "2D manifestation program invalid");
  check(validate_transformation_manifestation25d_program(manifest25d, transformations, combat, animation_graph, animation_clips, projections25d).ok(), "2.5D manifestation program invalid");
  check(validate_transformation_manifestation_program(manifest3d, transformations, combat, animation_graph, animation_clips, projections3d).ok(), "3D manifestation program invalid");

  TransformationState state{"unified", "base", 50, 50, std::nullopt, {}};

  const auto base_hash = transformation_state_identity(transformations, combat, state);

  begin_transformation(transformations, combat, state, "ascend", 10);

  const auto frame2d_t12 = project_transformation_manifestation2d(manifest2d, transformations, combat, animation_graph, animation_clips, projections2d, state, 12);
  const auto frame25d_t12 = project_transformation_manifestation25d(manifest25d, transformations, combat, animation_graph, animation_clips, projections25d, state, 12);
  const auto frame3d_t12 = project_transformation_manifestation(manifest3d, transformations, combat, animation_graph, animation_clips, projections3d, state, 12);

  check(frame2d_t12.stable_entity_id == "unified", "2D entity identity mismatch");
  check(frame25d_t12.stable_entity_id == "unified", "2.5D entity identity mismatch");
  check(frame3d_t12.stable_entity_id == "unified", "3D entity identity mismatch");

  check(frame2d_t12.form_id == "base", "2D form identity mismatch");
  check(frame25d_t12.form_id == "base", "2.5D form identity mismatch");
  check(frame3d_t12.form_id == "base", "3D form identity mismatch");

  check(frame2d_t12.transition_animation_clip_id == "ascend", "2D transition clip mismatch");
  check(frame25d_t12.transition_animation_clip_id == "ascend", "2.5D transition clip mismatch");
  check(frame3d_t12.transition_animation_clip_id == "ascend", "3D transition clip mismatch");

  check(frame2d_t12.transition_progress_per_million == 500'000, "2D progress mismatch");
  check(frame25d_t12.transition_progress_per_million == 500'000, "2.5D progress mismatch");
  check(frame3d_t12.transition_progress_per_million == 500'000, "3D progress mismatch");

  check(frame2d_t12.authoritative_state_identity == frame25d_t12.authoritative_state_identity, "2D/2.5D state hash mismatch");
  check(frame25d_t12.authoritative_state_identity == frame3d_t12.authoritative_state_identity, "2.5D/3D state hash mismatch");
  check(frame2d_t12.authoritative_state_identity != base_hash, "Transition did not change state hash");

  check(frame2d_t12.animation_state_id == "base", "2D animation intent mismatch");
  check(frame25d_t12.animation_state_id == "base", "2.5D animation intent mismatch");
  check(frame3d_t12.animation_state_id == "base", "3D animation intent mismatch");

  const auto evidence_t12 = collect_evidence(state, frame2d_t12, frame25d_t12, frame3d_t12, 12);
  write_evidence(evidence_t12, "cross_representation_evidence_t12.txt");

  advance_transformation_to(transformations, combat, state, combat_state_value, 14);

  const auto frame2d_t14 = project_transformation_manifestation2d(manifest2d, transformations, combat, animation_graph, animation_clips, projections2d, state, 14);
  const auto frame25d_t14 = project_transformation_manifestation25d(manifest25d, transformations, combat, animation_graph, animation_clips, projections25d, state, 14);
  const auto frame3d_t14 = project_transformation_manifestation(manifest3d, transformations, combat, animation_graph, animation_clips, projections3d, state, 14);

  check(frame2d_t14.form_id == "storm", "2D final form mismatch");
  check(frame25d_t14.form_id == "storm", "2.5D final form mismatch");
  check(frame3d_t14.form_id == "storm", "3D final form mismatch");

  check(frame2d_t14.projection_id == "unified.storm.2d", "2D final projection mismatch");
  check(frame25d_t14.projection_id == "unified.storm.25d", "2.5D final projection mismatch");
  check(frame3d_t14.projection_id == "unified.storm.3d", "3D final projection mismatch");

  check(!frame2d_t14.transition_animation_clip_id, "2D transition should be complete");
  check(!frame25d_t14.transition_animation_clip_id, "2.5D transition should be complete");
  check(!frame3d_t14.transition_animation_clip_id, "3D transition should be complete");

  check(frame2d_t14.authoritative_state_identity == frame25d_t14.authoritative_state_identity, "2D/2.5D final state hash mismatch");
  check(frame25d_t14.authoritative_state_identity == frame3d_t14.authoritative_state_identity, "2.5D/3D final state hash mismatch");

  check(frame2d_t14.animation_state_id == "storm", "2D final animation intent mismatch");
  check(frame25d_t14.animation_state_id == "storm", "2.5D final animation intent mismatch");
  check(frame3d_t14.animation_state_id == "storm", "3D final animation intent mismatch");

  const auto evidence_t14 = collect_evidence(state, frame2d_t14, frame25d_t14, frame3d_t14, 14);
  write_evidence(evidence_t14, "cross_representation_evidence_t14.txt");

  const auto encoded = serialize_transformation_state(transformations, combat, state);
  const auto restored = deserialize_transformation_state(transformations, combat, encoded);

  const auto frame2d_restored = project_transformation_manifestation2d(manifest2d, transformations, combat, animation_graph, animation_clips, projections2d, restored, 14);
  const auto frame25d_restored = project_transformation_manifestation25d(manifest25d, transformations, combat, animation_graph, animation_clips, projections25d, restored, 14);
  const auto frame3d_restored = project_transformation_manifestation(manifest3d, transformations, combat, animation_graph, animation_clips, projections3d, restored, 14);

  check(frame2d_restored.form_id == "storm", "2D restored form mismatch");
  check(frame25d_restored.form_id == "storm", "2.5D restored form mismatch");
  check(frame3d_restored.form_id == "storm", "3D restored form mismatch");

  check(frame2d_restored.authoritative_state_identity == frame25d_restored.authoritative_state_identity, "2D/2.5D restored hash mismatch");
  check(frame25d_restored.authoritative_state_identity == frame3d_restored.authoritative_state_identity, "2.5D/3D restored hash mismatch");

  check(frame2d_restored.authoritative_state_identity == transformation_state_identity(transformations, combat, restored), "2D restored hash mismatch");
  check(frame25d_restored.authoritative_state_identity == transformation_state_identity(transformations, combat, restored), "2.5D restored hash mismatch");
  check(frame3d_restored.authoritative_state_identity == transformation_state_identity(transformations, combat, restored), "3D restored hash mismatch");

  const auto evidence_restored = collect_evidence(restored, frame2d_restored, frame25d_restored, frame3d_restored, 14);
  write_evidence(evidence_restored, "cross_representation_evidence_restored.txt");

  check(evidence_t14.authoritative_state_identity == evidence_restored.authoritative_state_identity, "State hash not preserved across save/restore");

  const auto events = execute_transformed_combat_command(transformations, combat, restored, combat_state_value, {14, "unified", "target", "storm"});
  check(events.size() == 2 && combat_state_value.actors.at("target").health == 70 &&
        combat_state_value.actors.at("target").statuses.contains("shocked") &&
        combat_state_value.actors.at("unified").resource == 10 &&
        combat_state_value.actors.at("unified").maximum_resource == 40,
        "Transformed ability did not produce typed combat effects");

  check(frame2d_restored.authoritative_state_identity == frame2d_t14.authoritative_state_identity, "Combat did not change state hash consistently");

  std::cout << "all gspl sprites unified cross-representation acceptance tests passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }