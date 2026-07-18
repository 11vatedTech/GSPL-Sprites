#include "gspl_sprites/transformation_manifestation.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
CombatProgram combat(){return {"forms.combat",2,2,{{"pulse",CombatTargetRule::enemy,0,0,10,{{CombatEffectKind::damage,{},1,0}}}}};}
TransformationProgram transformations(){return {"forms","base",4,{{"base",0,0,{"pulse"}},{"power",0,0,{"pulse"}}},{{"up","base","power",0,4,true},{"down","power","base",0,2,true}}};}
std::vector<SkeletalClip> clips(){return {{"idle",10,true},{"power",10,true},{"up",4,false},{"down",2,false}};}
AnimationStateGraph graph(){return {"base",{{"base","idle",{{"power","form",Comparison::equal,1,0,0,1}}},{"power","power",{{"base","form",Comparison::equal,0,0,0,1}}}}};}
Projection25dDefinition projection(std::string id,std::string asset){return {std::move(id),RepresentationKind::two_point_five_d,BillboardMode::camera_facing,
  {{"body",std::move(asset),{},{},0,200'000,0,false}},{{"front",0,false,{},{{"body",true,{},{}}}}},{},{{"body","body",-100,-100,100,100,-10,10}}};}
TransformationManifestationProgram bindings(){return {"forms.25d",{{"base","base","base.25d"},{"power","power","power.25d"}},{{"up","up"},{"down","down"}}};}
}
int main()try{const auto cp=combat();const auto tp=transformations();const auto cs=clips();const auto ag=graph();
  const std::array projections{projection("base.25d","base.png"),projection("power.25d","power.png")};const auto mp=bindings();
  check(validate_transformation_manifestation25d_program(mp,tp,cp,ag,cs,projections).ok(),"valid 2.5D bindings rejected");
  TransformationState state{"entity","base",10,10};begin_transformation(tp,cp,state,"up",2);
  const auto frame=project_transformation_manifestation25d(mp,tp,cp,ag,cs,projections,state,3);
  check(frame.projection_id=="base.25d"&&frame.transition_animation_clip_id=="up"&&frame.transition_progress_per_million==250'000&&
        frame.authoritative_state_identity==transformation_state_identity(tp,cp,state),"2.5D manifestation is incorrect");
  auto invalid=projections;invalid[1].views[0].planes.clear();
  check(!validate_transformation_manifestation25d_program(mp,tp,cp,ag,cs,invalid).ok(),"invalid 2.5D projection accepted");
  std::cout<<"all gspl sprites 2.5D transformation manifestation tests passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
