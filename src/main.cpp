#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/target_contract.hpp"
#include "gspl_sprites/visual_set.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
  try {
    if(argc>=4&&std::string_view(argv[1])=="target-check"){
      const auto adapter=gspl::sprites::builtin_target_adapter(argv[2]);std::vector<gspl::sprites::TargetRequirement> requirements;
      for(int index=3;index<argc;++index){const auto feature=gspl::sprites::parse_target_feature(argv[index]);if(!feature){std::cerr<<"unknown target feature: "<<argv[index]<<'\n';return 2;}requirements.push_back({*feature,true});}
      const auto report=gspl::sprites::evaluate_target_compatibility(adapter,requirements);std::cout<<gspl::sprites::canonicalize_target_compatibility(report)<<'\n';for(const auto& diagnostic:report.validation.diagnostics)std::cerr<<diagnostic.code<<": "<<diagnostic.message<<'\n';return report.compatible()?0:1;
    }
    if(argc==3&&std::string_view(argv[1])=="verify"){
      const auto verification=gspl::sprites::verify_package(argv[2]);
      if(!verification.ok()){for(const auto& diagnostic:verification.validation.diagnostics)std::cerr<<diagnostic.code<<": "<<diagnostic.message<<'\n';return 1;}
      std::cout<<"verified "<<verification.entity_id<<" seed="<<verification.seed_identity<<" package="<<verification.package_identity<<" artifacts="<<verification.artifact_count<<" bytes="<<verification.total_artifact_bytes<<'\n';return 0;
    }
    const bool seed_only=argc==4&&std::string_view(argv[1])=="build";const bool visual=argc==5&&std::string_view(argv[1])=="build-visual";
    if (!seed_only&&!visual) { std::cerr << "usage:\n  gspl-sprites build <seed.sprite> <output-directory>\n  gspl-sprites build-visual <seed.sprite> <visual-set.txt> <output-directory>\n  gspl-sprites verify <package-directory>\n  gspl-sprites target-check <adapter> <required-feature>...\n"; return 2; }
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) { std::cerr << "cannot open input seed\n"; return 2; }
    const std::string source{std::istreambuf_iterator<char>(input), {}};
    const auto seed = gspl::sprites::parse_seed(source);
    const auto result = gspl::sprites::validate(seed);
    if (!result.ok()) { for (const auto& d : result.diagnostics) std::cerr << d.code << ": " << d.message << '\n'; return 1; }
    const std::filesystem::path output=visual?argv[4]:argv[3];
    if(visual){const auto visual_set=gspl::sprites::load_authored_visual_set(argv[3]);gspl::sprites::build_package(seed,visual_set,output);}
    else gspl::sprites::build_package(seed,output);
    const auto verification=gspl::sprites::verify_package(output);if(!verification.ok())throw std::runtime_error("published package failed self-verification: "+verification.validation.diagnostics.front().code+": "+verification.validation.diagnostics.front().message);
    std::cout << "built " << seed.stable_id << " seed=" << verification.seed_identity << " package=" << verification.package_identity << '\n';
    return 0;
  } catch (const std::exception& error) { std::cerr << "GSPL_SPRITES_FATAL: " << error.what() << '\n'; return 1; }
}
