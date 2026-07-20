#include "gspl_sprites/transformation_manifestation.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
CombatProgram combat(){return {"forms.combat",2,2,{{"pulse",CombatTargetRule::enemy,0,0,10,{{CombatEffectKind::damage,{},1,0}}}}};}
TransformationProgram transformations(){return {"forms","base",4,{{"base",0,0,{"pulse"}},{"power",0,0,{"pulse"}}},{{"up","base","power",0,4,true},{"down","power","base",0,2,true}}};}
std::vector<SkeletalClip> clips(){return {{"idle",10,true,{},{}},{"power",10,true,{},{}},{"up",4,false,{},{}},{"down",2,false,{},{}}};}
AnimationStateGraph graph(){return {"base",{{"base","idle",{{"power","form",Comparison::equal,1,0,0,1}}},{"power","power",{{"base","form",Comparison::equal,0,0,0,1}}}}};}
ImageRgba8 image(std::uint8_t value){return {2,2,ColorSpace::srgb,AlphaMode::straight,{value,0,0,255,value,0,0,255,value,0,0,255,value,0,0,255}};}
Projection2dDefinition projection(std::string id,std::string frame_id,std::uint8_t color){
  std::vector<FrameSource> frames{{frame_id,image(color),1,1,2}};auto sheet=compile_sprite_sheet(frames,{8,8,1,false,0});
  RigDefinition rig{"rig",{{"root",std::nullopt,{},1,{-180,180}}},{}};
  return {std::move(id),frames,std::move(sheet),{{"idle",{frame_id},{2},{},true}},{},std::move(rig),
          {{"body",CollisionKind::axis_aligned_box,"root",0,0,1,1}},{{"body",0,2,false,{}}},2};}
TransformationManifestationProgram bindings(){return {"forms.2d",{{"base","base","base.2d"},{"power","power","power.2d"}},{{"up","up"},{"down","down"}}};}
}
int main()try{const auto cp=combat();const auto tp=transformations();const auto cs=clips();const auto ag=graph();
  const std::array projections{projection("base.2d","base.frame",80),projection("power.2d","power.frame",180)};const auto mp=bindings();
  check(validate_transformation_manifestation2d_program(mp,tp,cp,ag,cs,projections).ok(),"valid 2D bindings rejected");
  TransformationState state{"entity","base",10,10,std::nullopt,{}};begin_transformation(tp,cp,state,"up",4);
  const auto frame=project_transformation_manifestation2d(mp,tp,cp,ag,cs,projections,state,6);
  check(frame.projection_id=="base.2d"&&frame.transition_animation_clip_id=="up"&&
        frame.transition_progress_per_million==500'000&&frame.authoritative_state_identity==transformation_state_identity(tp,cp,state),
        "2D manifestation is incorrect");
  auto invalid=projections;invalid[1].sheet.atlas.placements[0].width=3;
  check(!validate_transformation_manifestation2d_program(mp,tp,cp,ag,cs,invalid).ok(),"invalid compiled 2D projection accepted");
  std::cout<<"all gspl sprites 2D transformation manifestation tests passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
