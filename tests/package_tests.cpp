#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"

#include <filesystem>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool has_code(const PackageVerification& value,std::string_view code){return std::ranges::any_of(value.validation.diagnostics,[&](const auto& diagnostic){return diagnostic.code==code;});}
SpriteSeed fixture(){return parse_seed(R"(schema=gspl.sprite-seed/0.1
id=original.package-test
name=Package Test
classification=fictional
rights=ORIGINAL_USER_CREATION
entropy_root=9
primary_color=#112233
accent_color=#AABBCC
ability=arc|electric.projectile|20|4|2
)" );}
void write_text(const std::filesystem::path& path,std::string_view value){std::ofstream output(path,std::ios::binary);output.write(value.data(),static_cast<std::streamsize>(value.size()));if(!output)throw std::runtime_error("test write failed");}
std::string read_text(const std::filesystem::path& path){std::ifstream input(path,std::ios::binary);return{std::istreambuf_iterator<char>(input),{}};}
FrameSource visual(){ImageRgba8 image{3,3,ColorSpace::srgb,AlphaMode::straight,std::vector<std::uint8_t>(36,0)};for(std::uint32_t y=1;y<3;++y)for(std::uint32_t x=1;x<3;++x){const auto offset=(y*3+x)*4;image.pixels[offset]=255;image.pixels[offset+3]=255;}return{"idle.south.000",std::move(image),1,2,3};}
}

int main(){
  const auto base=std::filesystem::temp_directory_path()/"gspl-sprites-package-tests";
  try{
    std::filesystem::remove_all(base);std::filesystem::create_directories(base);
    const auto valid=base/"valid";build_package(fixture(),valid);const auto verified=verify_package(valid);check(verified.ok(),"valid package rejected");check(verified.artifact_count==5,"governance artifacts not declared");
    const auto visual_package=base/"visual";const std::array visual_frames{visual()};build_package(fixture(),visual_frames,{16,16,1,true,0},visual_package);const auto visual_verified=verify_package(visual_package);check(visual_verified.ok()&&visual_verified.artifact_count==9,"visual package rejected or incomplete");check(std::filesystem::exists(visual_package/"assets"/"sprite-atlas.png")&&std::filesystem::exists(visual_package/"assets"/"sprite-alpha-mask.png")&&std::filesystem::exists(visual_package/"assets"/"sprite-outline-mask.png")&&std::filesystem::exists(visual_package/"atlas.json"),"visual package artifacts missing");const auto atlas_png=read_text(visual_package/"assets"/"sprite-atlas.png");const auto decoded=decode_png(std::as_bytes(std::span(atlas_png)));check(decoded.width==16&&decoded.height==16,"packaged atlas is not decodable");check(read_text(visual_package/"provenance.json").find("ingest-frame/1")!=std::string::npos&&read_text(visual_package/"asset-graph.json").find("compile-atlas/1")!=std::string::npos,"visual provenance or dependency graph missing");
    const auto visual_changed=base/"visual-changed";auto changed_frame=visual();changed_frame.image.pixels[(1*3+1)*4]=0;const std::array changed_frames{changed_frame};build_package(fixture(),changed_frames,{16,16,1,true,0},visual_changed);const auto changed_verified=verify_package(visual_changed);check(changed_verified.ok()&&visual_verified.seed_identity==changed_verified.seed_identity&&visual_verified.package_identity!=changed_verified.package_identity,"visual source change did not preserve seed identity and alter package identity");
    const auto failed_visual=base/"failed-visual";ImageRgba8 empty{1,1,ColorSpace::srgb,AlphaMode::straight,{0,0,0,0}};const std::array empty_frames{FrameSource{"idle.empty.000",empty,0,0,1}};bool visual_failure=false;try{build_package(fixture(),empty_frames,{16,16,1,true,0},failed_visual);}catch(const std::invalid_argument&){visual_failure=true;}check(visual_failure&&!std::filesystem::exists(failed_visual)&&!std::filesystem::exists(failed_visual.string()+".staging"),"failed visual package was published or left staging state");
    auto tiny=PackageLimits{};tiny.max_total_bytes=1;const auto limited=verify_package(valid,tiny);check(!limited.ok()&&has_code(limited,"SPRITE_PACKAGE_TOTAL_LIMIT"),"total byte limit ignored");
    write_text(valid/"undeclared.txt","x");const auto undeclared=verify_package(valid);check(!undeclared.ok()&&has_code(undeclared,"SPRITE_PACKAGE_FILE_SET_MISMATCH"),"undeclared file accepted");std::filesystem::remove(valid/"undeclared.txt");
    {std::ofstream tamper(valid/"assets"/"entity.svg",std::ios::binary|std::ios::app);tamper<<"x";}const auto changed=verify_package(valid);check(!changed.ok()&&has_code(changed,"SPRITE_PACKAGE_HASH_MISMATCH"),"tampered artifact accepted");

    const auto traversal=base/"traversal";std::filesystem::create_directories(traversal);write_text(traversal/"manifest.json","{\"artifacts\":[{\"path\":\"../escape\",\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\"}],\"assetGraph\":\"asset-graph.json\",\"entityId\":\"x\",\"format\":\"gspl.sprite-package/0.1\",\"provenance\":\"provenance.json\",\"rights\":\"rights.json\",\"seedIdentity\":\"0000000000000000000000000000000000000000000000000000000000000000\"}");const auto escaped=verify_package(traversal);check(!escaped.ok()&&has_code(escaped,"SPRITE_PACKAGE_PATH_UNSAFE"),"traversal path accepted");
    const auto noncanonical=base/"noncanonical";std::filesystem::create_directories(noncanonical);write_text(noncanonical/"manifest.json","{}");const auto malformed=verify_package(noncanonical);check(!malformed.ok()&&has_code(malformed,"SPRITE_PACKAGE_MALFORMED"),"noncanonical manifest accepted");
    std::filesystem::remove_all(base);std::cout<<"all gspl sprites package tests passed\n";return 0;
  }catch(const std::exception& error){std::filesystem::remove_all(base);std::cerr<<error.what()<<'\n';return 1;}
}
