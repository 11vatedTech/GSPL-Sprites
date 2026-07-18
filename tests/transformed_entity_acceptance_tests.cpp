#include "gspl_sprites/transformation_manifestation.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
CombatProgram combat_program() { return {"voltfox.combat",4,4,
  {{"bite",CombatTargetRule::enemy,0,0,2000,{{CombatEffectKind::damage,{},5,0}}},
   {"storm",CombatTargetRule::enemy,10,4,5000,{{CombatEffectKind::damage,{},30,0},{CombatEffectKind::status,"shocked",2,3}}}}}; }
TransformationProgram transformation_program() { return {"voltfox.forms","base",8,
  {{"base",0,0,{"bite"}},{"storm",40,20,{"bite","storm"}}},
  {{"ascend","base","storm",20,4,true},{"descend","storm","base",0,2,true}}}; }
std::vector<SkeletalClip> clips() { return {{"idle",10,true},{"storm.idle",10,true},{"ascend",4,false},{"descend",2,false}}; }
AnimationStateGraph graph() { return {"base",{{"base","idle",{{"storm","form",Comparison::equal,1,0,0,1}}},{"storm","storm.idle",{{"base","form",Comparison::equal,0,0,0,1}}}}}; }
Vertex3d vertex(std::int64_t x,std::int64_t y){return {{x,y,0},{0,0,1'000'000},{0,0},{}};}
Projection3dDefinition projection(std::string id,std::uint32_t color){Projection3dDefinition p;p.id=std::move(id);p.materials={{"fur",color,0,900'000}};p.meshes={{"body",MeshPurpose::render,"fur",false,{vertex(0,0),vertex(1000,0),vertex(0,1000)},{0,2,1}}};return p;}
TransformationManifestationProgram manifestation_program(){return {"voltfox.manifestation",{{"base","base","voltfox.base.3d"},{"storm","storm","voltfox.storm.3d"}},{{"ascend","ascend"},{"descend","descend"}}};}
CombatState combat_state(){CombatState state;state.actors.emplace("voltfox",CombatActorState{"voltfox","hero",100,100,20,20,0,0});state.actors.emplace("target",CombatActorState{"target","enemy",100,100,0,0,1000,0});return state;}
}
int main() try {
  const auto combat=combat_program();const auto transformations=transformation_program();
  const auto animation_clips=clips();const auto animation_graph=graph();
  const std::array projections{projection("voltfox.base.3d",0x804020ffU),projection("voltfox.storm.3d",0x40a0ffffU)};
  const auto manifestations=manifestation_program();
  TransformationState form{"voltfox","base",50,50};auto combat_state_value=combat_state();
  const auto base_identity=transformation_state_identity(transformations,combat,form);
  begin_transformation(transformations,combat,form,"ascend",10);
  const auto transition=project_transformation_manifestation(manifestations,transformations,combat,animation_graph,animation_clips,projections,form,12);
  check(transition.form_id=="base"&&transition.transition_progress_per_million==500'000&&transition.authoritative_state_identity!=base_identity,
        "transition did not manifest authoritative progress");
  advance_transformation_to(transformations,combat,form,combat_state_value,14);
  const auto encoded=serialize_transformation_state(transformations,combat,form);
  const auto restored=deserialize_transformation_state(transformations,combat,encoded);
  const auto manifested=project_transformation_manifestation(manifestations,transformations,combat,animation_graph,animation_clips,projections,restored,14);
  check(manifested.form_id=="storm"&&manifested.projection_id=="voltfox.storm.3d"&&
        manifested.animation_state_id=="storm"&&!manifested.transition_animation_clip_id&&
        manifested.authoritative_state_identity==transformation_state_identity(transformations,combat,restored),
        "restored authoritative form did not drive presentation");
  const auto events=execute_transformed_combat_command(transformations,combat,restored,combat_state_value,{14,"voltfox","target","storm"});
  check(events.size()==2&&combat_state_value.actors.at("target").health==70&&
        combat_state_value.actors.at("target").statuses.contains("shocked")&&
        combat_state_value.actors.at("voltfox").resource==10&&
        combat_state_value.actors.at("voltfox").maximum_resource==40,
        "transformed ability did not produce typed combat effects");
  std::cout<<"all gspl sprites transformed entity acceptance tests passed\n";return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
