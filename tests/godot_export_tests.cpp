#include "gspl_sprites/core.hpp"
#include "gspl_sprites/godot_export.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

Projection3dDefinition fixture() {
  Projection3dDefinition projection;
  projection.id = "entity.godot";
  projection.materials = {{"body", 0xff8040ffU, 0, 800'000,
                           MaterialAlphaMode::opaque, 500'000, false}};
  projection.meshes = {{"body",
                        MeshPurpose::render,
                        "body",
                        false,
                        {{{0, 0, 0}, {}, {}, {}},
                         {{1'000'000, 0, 0}, {}, {1'000'000, 0}, {}},
                         {{0, 1'000'000, 0}, {}, {0, 1'000'000}, {}}},
                        {0, 1, 2}}};
  return projection;
}

SpriteSeed seed_fixture() {
  return parse_seed(R"(schema=gspl.sprite-seed/0.1
id=original.godot-source
name=Godot Source
classification=fictional
rights=ORIGINAL_USER_CREATION
entropy_root=47
primary_color=#112233
accent_color=#AABBCC
ability=arc|electric.projectile|20|4|2
)");
}

std::string read(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), {}};
}
} // namespace

int main(int argc, char **argv) try {
  if (argc > 2)
    throw std::invalid_argument("expected at most one fixture path");
  const auto root = argc == 2 ? std::filesystem::path(argv[1])
                              : std::filesystem::temp_directory_path() /
                                    "gspl-sprites-godot-export-tests";
  if (argc != 2)
    std::filesystem::remove_all(root);
  std::filesystem::remove_all(root.string() + ".staging");
  const std::filesystem::path source_package =
      root.string() + "-source-package";
  std::filesystem::remove_all(source_package);
  build_package(seed_fixture(), source_package);
  const auto source = target_source_evidence_from_package(source_package);
  export_godot_3d_project(fixture(), {}, {}, {}, root,
                          {"Godot Fixture", {}, source});
  const auto verification = verify_godot_3d_project(root);
  check(
      verification.ok() && verification.project_identity.size() == 64 &&
          std::filesystem::exists(root / "assets" / "entity.glb") &&
          read(root / "source-evidence.json").find(source.package_identity()) !=
              std::string::npos,
      "generated Godot target failed verification");
  const std::array unsupported{
      TargetRequirement{TargetFeature::living_runtime, true}};
  bool rejected = false;
  try {
    export_godot_3d_project(fixture(), {}, {}, unsupported,
                            root.string() + "-unsupported");
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected && !std::filesystem::exists(root.string() + "-unsupported"),
        "Godot target silently discarded living runtime semantics");
  bool invalid_name_rejected = false;
  try {
    export_godot_3d_project(fixture(), {}, {}, {},
                            root.string() + "-invalid-name",
                            {std::string("bad\xc0\xaf", 5)});
  } catch (const std::invalid_argument &) {
    invalid_name_rejected = true;
  }
  check(invalid_name_rejected &&
            !std::filesystem::exists(root.string() + "-invalid-name"),
        "Godot target accepted a malformed UTF-8 project name");
  bool invalid_source_rejected = false;
  try {
    std::ofstream tamper(source_package / "authoring-provenance.json",
                         std::ios::app);
    tamper << "x";
    tamper.close();
    (void)target_source_evidence_from_package(source_package);
  } catch (const std::invalid_argument &) {
    invalid_source_rejected = true;
  }
  check(invalid_source_rejected,
        "Godot target evidence accepted a modified source package");
  if (argc != 2) {
    {
      std::ofstream undeclared(root / "undeclared.txt");
      undeclared << "not governed";
    }
    check(!verify_godot_3d_project(root).ok(),
          "Godot target verifier accepted an undeclared artifact");
    std::filesystem::remove(root / "undeclared.txt");
    std::ofstream tamper(root / "project.godot", std::ios::app);
    tamper << "\n; tampered\n";
    tamper.close();
    check(!verify_godot_3d_project(root).ok(),
          "Godot target verifier accepted a modified artifact");
    std::filesystem::remove_all(root);
  }
  std::filesystem::remove_all(source_package);
  std::cout << "all GSPL Sprites Godot export tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
