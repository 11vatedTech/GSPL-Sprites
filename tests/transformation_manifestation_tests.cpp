#include "gspl_sprites/transformation_manifestation.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template <class F> bool rejects(F f) { try { f(); } catch (const std::exception &) { return true; } return false; }
CombatProgram combat() { return {"manifest.combat", 2, 2, {{"pulse", CombatTargetRule::enemy, 0, 0, 10, {{CombatEffectKind::damage, {}, 1, 0}}}}}; }
TransformationProgram transformations() { return {"manifest.forms", "base", 4,
  {{"base", 0, 0, {"pulse"}}, {"power", 10, 0, {"pulse"}}},
  {{"up", "base", "power", 0, 4, true}, {"down", "power", "base", 0, 2, true}}}; }
std::vector<SkeletalClip> clips() { return {{"idle", 10, true}, {"powered", 10, true}, {"transform.up", 4, false}, {"transform.down", 2, false}}; }
AnimationStateGraph graph() { return {"base", {{"base", "idle", {{"power", "form", Comparison::equal, 1, 0, 0, 1}}}, {"power", "powered", {{"base", "form", Comparison::equal, 0, 0, 0, 1}}}}}; }
Vertex3d vertex(std::int64_t x, std::int64_t y, std::int64_t z) { return {{x,y,z},{0,0,1'000'000},{0,0},{}}; }
Projection3dDefinition projection(std::string id) { Projection3dDefinition p; p.id=std::move(id);
  p.materials={{"material",0xffffffffU,0,1'000'000}};
  p.meshes={{"mesh",MeshPurpose::render,"material",false,{vertex(0,0,0),vertex(1000,0,0),vertex(0,1000,0)},{0,2,1}}}; return p; }
TransformationManifestationProgram manifestation() { return {"manifest.bindings",
  {{"base","base","projection.base"},{"power","power","projection.power"}},
  {{"up","transform.up"},{"down","transform.down"}}}; }
}
int main() try {
  const auto cp=combat(); const auto tp=transformations(); const auto cs=clips(); const auto ag=graph();
  const std::array projections{projection("projection.base"),projection("projection.power")};
  const auto mp=manifestation();
  check(validate_transformation_manifestation_program(mp,tp,cp,ag,cs,projections).ok(), "valid manifestation bindings rejected");
  TransformationState state{"entity","base",10,10}; begin_transformation(tp,cp,state,"up",10);
  const auto midway=project_transformation_manifestation(mp,tp,cp,ag,cs,projections,state,12);
  check(midway.form_id=="base" && midway.projection_id=="projection.base" &&
        midway.transition_animation_clip_id=="transform.up" && midway.transition_progress_per_million==500'000 &&
        midway.authoritative_state_identity==transformation_state_identity(tp,cp,state),
        "active transformation manifestation is incorrect");
  check(rejects([&]{(void)project_transformation_manifestation(mp,tp,cp,ag,cs,projections,state,15);}),
        "manifestation accepted a tick outside active transition");
  auto missing=mp; missing.forms.pop_back();
  check(!validate_transformation_manifestation_program(missing,tp,cp,ag,cs,projections).ok(), "incomplete form coverage accepted");
  auto unknown=mp; unknown.transitions[0].animation_clip_id="absent";
  check(!validate_transformation_manifestation_program(unknown,tp,cp,ag,cs,projections).ok(), "unknown transition clip accepted");
  std::cout << "all gspl sprites transformation manifestation tests passed\n"; return 0;
} catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
