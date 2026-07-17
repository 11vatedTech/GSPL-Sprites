#include "gspl_sprites/visual_set.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace{void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}void write(const std::filesystem::path&path,std::span<const std::byte> bytes){std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if(!output)throw std::runtime_error("fixture write failed");}void text(const std::filesystem::path&path,std::string_view value){write(path,std::as_bytes(std::span(value)));}std::string manifest(std::string_view rights="ORIGINAL_USER_CREATION",std::string_view frame_path="frame.png"){return"schema=gspl.visual-set/0.1\nrights="+std::string(rights)+"\nmax_width=16\nmax_height=16\npadding=1\ntrim=true\nalpha_threshold=0\nframe=idle|south|body|0|"+std::string(frame_path)+"|1|2|3\n";}}

int main(){const auto root=std::filesystem::temp_directory_path()/"gspl-visual-set-tests";try{std::filesystem::remove_all(root);std::filesystem::create_directories(root);ImageRgba8 image{2,2,ColorSpace::srgb,AlphaMode::straight,{255,0,0,255,0,0,0,0,0,0,0,0,0,0,0,0}};write(root/"frame.png",encode_png(image));text(root/"visual.txt",manifest());const auto loaded=load_authored_visual_set(root/"visual.txt");check(loaded.schema=="gspl.visual-set/0.1"&&loaded.frames.size()==1&&loaded.frames[0].id=="idle.south.body.0000"&&loaded.frames[0].pivot_x==1&&loaded.frames[0].duration_ticks==3,"valid visual set loaded incorrectly");
  bool rights=false;try{text(root/"bad-rights.txt",manifest("RESEARCH_ONLY_REFERENCE"));(void)load_authored_visual_set(root/"bad-rights.txt");}catch(const std::runtime_error&){rights=true;}check(rights,"unsupported source rights accepted");
  bool traversal=false;try{text(root/"traversal.txt",manifest("ORIGINAL_USER_CREATION","../frame.png"));(void)load_authored_visual_set(root/"traversal.txt");}catch(const std::runtime_error&){traversal=true;}check(traversal,"visual path traversal accepted");
  bool duplicate=false;try{text(root/"duplicate.txt",manifest()+"frame=idle|south|body|0|frame.png|1|2|3\n");(void)load_authored_visual_set(root/"duplicate.txt");}catch(const std::runtime_error&){duplicate=true;}check(duplicate,"duplicate semantic frame accepted");
  bool aggregate=false;try{auto limits=VisualSetLimits{};limits.max_total_decoded_bytes=1;(void)load_authored_visual_set(root/"visual.txt",limits);}catch(const std::runtime_error&){aggregate=true;}check(aggregate,"aggregate decoded-image limit ignored");
  auto untagged=encode_png(image);const std::array marker{std::byte{'s'},std::byte{'R'},std::byte{'G'},std::byte{'B'}};const auto found=std::search(untagged.begin(),untagged.end(),marker.begin(),marker.end());check(found!=untagged.end(),"PNG fixture lacks sRGB chunk");untagged.erase(found-4,found+9);write(root/"untagged.png",untagged);bool color=false;try{text(root/"untagged.txt",manifest("ORIGINAL_USER_CREATION","untagged.png"));(void)load_authored_visual_set(root/"untagged.txt");}catch(const std::runtime_error&){color=true;}check(color,"untagged visual PNG accepted");
  std::filesystem::remove_all(root);std::cout<<"all gspl sprites visual-set tests passed\n";return 0;}catch(const std::exception&error){std::filesystem::remove_all(root);std::cerr<<error.what()<<'\n';return 1;}}
