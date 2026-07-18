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
  export_godot_3d_project(fixture(), {}, {}, {}, root, {"Godot Fixture"});
  const auto verification = verify_godot_3d_project(root);
  check(verification.ok() && verification.project_identity.size() == 64 &&
            std::filesystem::exists(root / "assets" / "entity.glb"),
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
  std::cout << "all GSPL Sprites Godot export tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
