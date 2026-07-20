#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/transformation_manifestation.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
CombatProgram combat_program() { return {"synthesis.combat", 4, 4,
  {{"bite", CombatTargetRule::enemy, 0, 0, 2000, {{CombatEffectKind::damage, {}, 5, 0}}},
   {"storm", CombatTargetRule::enemy, 10, 4, 5000, {{CombatEffectKind::damage, {}, 30, 0}, {CombatEffectKind::status, "shocked", 2, 3}}}}}; }
TransformationProgram transformation_program() { return {"synthesis.forms", "base", 8,
  {{"base", 0, 0, {"bite"}}, {"storm", 40, 20, {"bite", "storm"}}},
  {{"ascend", "base", "storm", 20, 4, true}, {"descend", "storm", "base", 0, 2, true}}}; }
std::vector<SkeletalClip> clips() { return {{"idle", 10, true, {}, {}}, {"storm.idle", 10, true, {}, {}}, {"ascend", 4, false, {}, {}}, {"descend", 2, false, {}, {}}}; }
AnimationStateGraph graph() { return {"base", {{"base", "idle", {{"storm", "form", Comparison::equal, 1, 0, 0, 1}}}, {"storm", "storm.idle", {{"base", "form", Comparison::equal, 0, 0, 0, 1}}}}}; }
}
int main() try {
  const auto combat = combat_program();
  const auto transformations = transformation_program();
  const auto animation_clips = clips();
  const auto animation_graph = graph();

  SynthesisPalette base_pal = make_palette("#804020", "#40A0FF");
  SynthesisPalette storm_pal = make_palette("#40A0FF", "#804020");
  auto result = synthesize_unified_entity("synth", base_pal, storm_pal);

  check(result.proj2d_base.id == "synth.base.2d", "2D base id mismatch");
  check(result.proj2d_transformed.id == "synth.storm.2d", "2D transformed id mismatch");
  check(result.proj25d_base.id == "synth.base.25d", "2.5D base id mismatch");
  check(result.proj25d_transformed.id == "synth.storm.25d", "2.5D transformed id mismatch");
  check(result.proj3d_base.id == "synth.base.3d", "3D base id mismatch");
  check(result.proj3d_transformed.id == "synth.storm.3d", "3D transformed id mismatch");

  auto val2d_base = validate_projection2d(result.proj2d_base);
  if (!val2d_base.ok()) {
    for (const auto& d : val2d_base.diagnostics) {
      std::cerr << "2D base validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val2d_base.ok(), "2D base projection invalid");

  auto val2d_transformed = validate_projection2d(result.proj2d_transformed);
  if (!val2d_transformed.ok()) {
    for (const auto& d : val2d_transformed.diagnostics) {
      std::cerr << "2D transformed validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val2d_transformed.ok(), "2D transformed projection invalid");
  auto val25d_base = validate_projection25d(result.proj25d_base);
  if (!val25d_base.ok()) {
    for (const auto& d : val25d_base.diagnostics) {
      std::cerr << "2.5D base validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val25d_base.ok(), "2.5D base projection invalid");

  auto val25d_transformed = validate_projection25d(result.proj25d_transformed);
  if (!val25d_transformed.ok()) {
    for (const auto& d : val25d_transformed.diagnostics) {
      std::cerr << "2.5D transformed validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val25d_transformed.ok(), "2.5D transformed projection invalid");
  auto val3d_base = validate_projection3d(result.proj3d_base);
  if (!val3d_base.ok()) {
    for (const auto& d : val3d_base.diagnostics) {
      std::cerr << "3D base validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val3d_base.ok(), "3D base projection invalid");

  auto val3d_transformed = validate_projection3d(result.proj3d_transformed);
  if (!val3d_transformed.ok()) {
    for (const auto& d : val3d_transformed.diagnostics) {
      std::cerr << "3D transformed validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val3d_transformed.ok(), "3D transformed projection invalid");

  check(result.manifest2d.id == "synth.manifest.2d", "2D manifest id mismatch");
  check(result.manifest25d.id == "synth.manifest.25d", "2.5D manifest id mismatch");
  check(result.manifest3d.id == "synth.manifest.3d", "3D manifest id mismatch");

  std::array proj2d_arr{result.proj2d_base, result.proj2d_transformed};
  std::array proj25d_arr{result.proj25d_base, result.proj25d_transformed};
  std::array proj3d_arr{result.proj3d_base, result.proj3d_transformed};

  auto val2d_manifest = validate_transformation_manifestation2d_program(result.manifest2d, transformations, combat, animation_graph, animation_clips, proj2d_arr);
  if (!val2d_manifest.ok()) {
    for (const auto& d : val2d_manifest.diagnostics) {
      std::cerr << "2D manifestation validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val2d_manifest.ok(), "2D manifestation invalid");

  auto val25d_manifest = validate_transformation_manifestation25d_program(result.manifest25d, transformations, combat, animation_graph, animation_clips, proj25d_arr);
  if (!val25d_manifest.ok()) {
    for (const auto& d : val25d_manifest.diagnostics) {
      std::cerr << "2.5D manifestation validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val25d_manifest.ok(), "2.5D manifestation invalid");

  auto val3d_manifest = validate_transformation_manifestation_program(result.manifest3d, transformations, combat, animation_graph, animation_clips, proj3d_arr);
  if (!val3d_manifest.ok()) {
    for (const auto& d : val3d_manifest.diagnostics) {
      std::cerr << "3D manifestation validation: " << d.code << " - " << d.message << '\n';
    }
  }
  check(val3d_manifest.ok(), "3D manifestation invalid");

  std::cout << "all gspl sprites synthesis tests passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }