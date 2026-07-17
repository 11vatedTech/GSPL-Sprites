#include "gspl_sprites/sprite2d.hpp"

#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace gspl::sprites;
namespace{void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}ImageRgba8 fixture(){ImageRgba8 image{4,4,ColorSpace::srgb,AlphaMode::straight,std::vector<std::uint8_t>(64,0)};for(std::uint32_t y=1;y<3;++y)for(std::uint32_t x=1;x<3;++x){const auto o=(y*4+x)*4;image.pixels[o]=x==1?255:0;image.pixels[o+1]=x==2?255:0;image.pixels[o+3]=255;}return image;}}

int main(){try{
  const FrameSource source{"idle.north.000",fixture(),2,3,4};const auto trimmed=trim_transparent(source);check(trimmed.frame.image.width==2&&trimmed.frame.image.height==2&&trimmed.frame.pivot_x==1&&trimmed.frame.pivot_y==2,"transparent trim or pivot adjustment failed");check(trimmed.source_x==1&&trimmed.source_y==1&&trimmed.source_width==4&&trimmed.source_height==4,"trim source metadata failed");
  const auto scaled=scale_nearest(trimmed.frame,3);check(scaled.image.width==6&&scaled.image.height==6&&scaled.pivot_x==3&&scaled.pivot_y==6,"nearest scaling failed");check(scaled.image.pixels[0]==255&&scaled.image.pixels[(3*4)]==0,"nearest scaling introduced interpolation");
  const auto palette=extract_palette(trimmed.frame.image);check(palette.size()==2&&palette[0].occurrences==2&&palette[0].rgba==0x00ff00ff,"palette extraction is not canonical");const std::array fixed_palette{0xff0000ffU,0x0000ffffU};const auto remapped=remap_palette(trimmed.frame.image,fixed_palette);check(extract_palette(remapped).size()==2,"palette remap failed");
  const auto alpha=alpha_mask(source.image);const auto outline=outline_mask(source.image);check(alpha.pixels[(1*4+1)*4]==255&&alpha.pixels[0]==0,"alpha mask incorrect");check(outline.pixels[(0*4+1)*4]==255&&outline.pixels[(1*4+1)*4]==0,"outline mask incorrect");
  check(validate_pixel_art(trimmed.frame.image,{1,1,2,true}).ok(),"valid pixel art rejected");auto fractional=trimmed.frame.image;fractional.pixels[3]=127;check(!validate_pixel_art(fractional,{1,1,2,true}).ok(),"fractional pixel alpha accepted");check(!validate_pixel_art(trimmed.frame.image,{3,1,2,true}).ok(),"misaligned pixel grid accepted");
  ImageRgba8 excessive{3,1,ColorSpace::srgb,AlphaMode::opaque,{255,0,0,255,0,255,0,255,0,0,255,255}};check(!validate_pixel_art(excessive,{1,1,2,true}).ok(),"excessive pixel palette accepted");
  const std::array frames{trimmed.frame,scale_nearest(trimmed.frame,2)};auto second=frames[1];second.id="idle.north.001";const std::array named{frames[0],second};const auto atlas=pack_atlas(named,16,16,1);const auto metadata=canonicalize_atlas_metadata(atlas);check(metadata.find("gspl.sprite-atlas/0.1")!=std::string::npos&&metadata.find("idle.north.000")<metadata.find("idle.north.001"),"atlas metadata is not canonical");
  const auto sheet=compile_sprite_sheet(named,{16,16,1,true,0});check(sheet.atlas.placements.size()==2&&sheet.alpha.width==16&&sheet.outline.height==16&&sheet.metadata==canonicalize_atlas_metadata(sheet.atlas),"sprite-sheet compilation failed");
  auto temporal_next=source;temporal_next.id="idle.north.001";temporal_next.image.pixels[(1*4+1)*4]=0;temporal_next.image.pixels[(1*4+2)*4]=255;const std::array temporal_frames{source,temporal_next};const auto temporal=analyze_temporal_stability(temporal_frames);check(temporal.size()==1&&temporal[0].compared_pixels==16&&temporal[0].changed_pixels==2&&temporal[0].changed_per_million==125000&&temporal[0].silhouette_union_pixels==4&&temporal[0].silhouette_intersection_pixels==4&&temporal[0].silhouette_iou_per_million==1000000,"temporal stability metrics are incorrect");
  bool placement_rejected=false;try{auto invalid=atlas;invalid.placements[0].x=std::numeric_limits<std::uint32_t>::max();(void)canonicalize_atlas_metadata(invalid);}catch(const std::invalid_argument&){placement_rejected=true;}check(placement_rejected,"overflowing atlas placement accepted");
  bool empty_rejected=false;try{ImageRgba8 empty{1,1,ColorSpace::srgb,AlphaMode::straight,{0,0,0,0}};(void)trim_transparent({"empty",empty,0,0,1});}catch(const std::invalid_argument&){empty_rejected=true;}check(empty_rejected,"empty foreground frame accepted");
  std::cout<<"all gspl sprites 2D tests passed\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
