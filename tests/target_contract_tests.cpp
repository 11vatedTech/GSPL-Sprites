#include "gspl_sprites/target_contract.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
} // namespace

int main(int argc, char **argv) try {
  const auto glb = builtin_target_adapter("glb-2.0");
  const std::array accepted{
      TargetRequirement{TargetFeature::mesh_3d, true},
      TargetRequirement{TargetFeature::animation_3d, true},
      TargetRequirement{TargetFeature::lod_3d, true}};
  const auto accepted_report = evaluate_target_compatibility(glb, accepted);
  check(accepted_report.compatible() && accepted_report.resolutions.size() == 3,
        "GLB target rejected implemented features");
  const std::array rejected{
      TargetRequirement{TargetFeature::living_runtime, true},
      TargetRequirement{TargetFeature::mesh_3d, true}};
  const auto rejected_report = evaluate_target_compatibility(glb, rejected);
  check(!rejected_report.compatible() &&
            rejected_report.validation.diagnostics.front().code ==
                "SPRITE_TARGET_FEATURE_UNSUPPORTED",
        "GLB target silently accepted semantic runtime loss");
  const std::array optional{
      TargetRequirement{TargetFeature::living_runtime, false}};
  check(evaluate_target_compatibility(glb, optional).compatible(),
        "optional unsupported target feature blocked compatibility");
  const std::array duplicated{TargetRequirement{TargetFeature::mesh_3d, true},
                              TargetRequirement{TargetFeature::mesh_3d, false}};
  check(!evaluate_target_compatibility(glb, duplicated).compatible(),
        "duplicate target requirement was accepted");
  const auto canonical = canonicalize_target_compatibility(rejected_report);
  check(canonical.find("\"compatible\":false") != std::string::npos &&
            canonical.find("living-runtime") < canonical.find("mesh-3d"),
        "target report is not deterministic");
  auto duplicate = glb;
  duplicate.capabilities.push_back(duplicate.capabilities.front());
  check(!validate_target_adapter(duplicate).ok(),
        "duplicate target capability was accepted");
  check(argc == 2, "CLI executable argument missing");
  const std::string command = '"' + std::string(argv[1]) +
                              "\" target-check glb-2.0 mesh-3d living-runtime";
  check(std::system(command.c_str()) != 0,
        "target-check CLI accepted an unsupported required feature");
  const std::string accepted_command =
      '"' + std::string(argv[1]) +
      "\" target-check glb-2.0 mesh-3d animation-3d";
  check(std::system(accepted_command.c_str()) == 0,
        "target-check CLI rejected implemented required features");
  std::cout << "all GSPL Sprites target contract tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
