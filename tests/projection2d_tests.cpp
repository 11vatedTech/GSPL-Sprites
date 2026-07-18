#include "gspl_sprites/projection2d.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace gspl::sprites;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
ImageRgba8 image(std::uint8_t red){return {2,2,ColorSpace::srgb,AlphaMode::straight,{red,0,0,255,red,0,0,255,red,0,0,255,red,0,0,255}};}
Projection2dDefinition fixture(){
  std::vector<FrameSource> frames{{"idle.0",image(100),1,1,2},{"idle.1",image(120),1,1,2}};
  auto sheet=compile_sprite_sheet(frames,{16,16,1,false,0});
  RigDefinition rig{"rig",{{"root",std::nullopt,{},1,{-180,180}}},{}};
  auto channel=image(0);channel.color_space=ColorSpace::data;
  for(std::size_t i=0;i<channel.pixels.size();i+=4)
    channel.pixels[i]=channel.pixels[i+1]=channel.pixels[i+2]=64;
  return {"fox.2d",frames,std::move(sheet),{{"idle",{"idle.0","idle.1"},{2,2},{{"blink",2}},true}},
          {{"idle.0.depth","idle.0",ChannelMapKind::depth,std::move(channel)}},std::move(rig),{{"body",CollisionKind::axis_aligned_box,"root",0,0,1,1}},
          {{"body",0,4,false,{}}},4};
}
}
int main()try{
  const auto projection=fixture();
  check(validate_projection2d(projection).ok()&&projection2d_identity(projection).size()==64,
        "valid 2D projection rejected");
  auto pixels=projection;pixels.source_frames[0].image.pixels[0]=99;
  check(projection2d_identity(pixels)!=projection2d_identity(projection),"2D identity ignored source pixels");
  auto channel_pixels=projection;
  channel_pixels.channel_maps[0].image.pixels[0]=channel_pixels.channel_maps[0].image.pixels[1]=
      channel_pixels.channel_maps[0].image.pixels[2]=65;
  check(projection2d_identity(channel_pixels)!=projection2d_identity(projection),"2D identity ignored channel pixels");
  auto overlap=projection;overlap.sheet.atlas.placements[1].x=overlap.sheet.atlas.placements[0].x;
  overlap.sheet.atlas.placements[1].y=overlap.sheet.atlas.placements[0].y;
  overlap.sheet.metadata=canonicalize_atlas_metadata(overlap.sheet.atlas);
  check(!validate_projection2d(overlap).ok(),"overlapping atlas placements accepted");
  auto uncovered=projection;uncovered.animations[0].frame_ids.pop_back();uncovered.animations[0].frame_durations.pop_back();
  check(!validate_projection2d(uncovered).ok(),"unanimated source frame accepted");
  auto metadata=projection;metadata.sheet.metadata.push_back(' ');
  check(!validate_projection2d(metadata).ok(),"noncanonical atlas metadata accepted");
  auto collision=projection;collision.collision_windows[0].end_tick=5;
  check(!validate_projection2d(collision).ok(),"out-of-range 2D collision window accepted");
  std::cout<<"all gspl sprites 2D projection tests passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
