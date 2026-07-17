#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
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
}

int main(){
  const auto base=std::filesystem::temp_directory_path()/"gspl-sprites-package-tests";
  try{
    std::filesystem::remove_all(base);std::filesystem::create_directories(base);
    const auto valid=base/"valid";build_package(fixture(),valid);const auto verified=verify_package(valid);check(verified.ok(),"valid package rejected");check(verified.artifact_count==5,"governance artifacts not declared");
    auto tiny=PackageLimits{};tiny.max_total_bytes=1;const auto limited=verify_package(valid,tiny);check(!limited.ok()&&has_code(limited,"SPRITE_PACKAGE_TOTAL_LIMIT"),"total byte limit ignored");
    write_text(valid/"undeclared.txt","x");const auto undeclared=verify_package(valid);check(!undeclared.ok()&&has_code(undeclared,"SPRITE_PACKAGE_FILE_SET_MISMATCH"),"undeclared file accepted");std::filesystem::remove(valid/"undeclared.txt");
    {std::ofstream tamper(valid/"assets"/"entity.svg",std::ios::binary|std::ios::app);tamper<<"x";}const auto changed=verify_package(valid);check(!changed.ok()&&has_code(changed,"SPRITE_PACKAGE_HASH_MISMATCH"),"tampered artifact accepted");

    const auto traversal=base/"traversal";std::filesystem::create_directories(traversal);write_text(traversal/"manifest.json","{\"artifacts\":[{\"path\":\"../escape\",\"sha256\":\"0000000000000000000000000000000000000000000000000000000000000000\"}],\"assetGraph\":\"asset-graph.json\",\"entityId\":\"x\",\"format\":\"gspl.sprite-package/0.1\",\"provenance\":\"provenance.json\",\"rights\":\"rights.json\",\"seedIdentity\":\"0000000000000000000000000000000000000000000000000000000000000000\"}");const auto escaped=verify_package(traversal);check(!escaped.ok()&&has_code(escaped,"SPRITE_PACKAGE_PATH_UNSAFE"),"traversal path accepted");
    const auto noncanonical=base/"noncanonical";std::filesystem::create_directories(noncanonical);write_text(noncanonical/"manifest.json","{}");const auto malformed=verify_package(noncanonical);check(!malformed.ok()&&has_code(malformed,"SPRITE_PACKAGE_MALFORMED"),"noncanonical manifest accepted");
    std::filesystem::remove_all(base);std::cout<<"all gspl sprites package tests passed\n";return 0;
  }catch(const std::exception& error){std::filesystem::remove_all(base);std::cerr<<error.what()<<'\n';return 1;}
}

