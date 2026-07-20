#include "gspl_sprites/authoring.hpp"
#include "gspl_sprites/authoring_cli.hpp"
#include "gspl_sprites/package.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

AuthoringProject fixture() {
  AuthoringProject project{
      "gspl.sprite-authoring/0.1",
      "authoring.cli",
      "CLI-authored electric fox",
      0,
      {},
      {{"identity.stable_id", {"original.cli-authoring"}, 0, true},
       {"identity.name", {"Voltfox", "Arcfox"}, 0, false},
       {"identity.classification", {"fictional.creature"}, 0, true},
       {"rights.class", {"ORIGINAL_USER_CREATION"}, 0, true},
       {"entropy.root", {"17"}, 0, true},
       {"appearance.primary_color", {"#1122AA", "#6633CC"}, 0, false},
       {"appearance.accent_color", {"#FFFF44"}, 0, true}},
      {{{"arc", "electric.projectile", 20, 4, 2}, true, true},
       {{"dash", "electric.movement", 10, 2, 1}, true, false}},
      {{"violet", {{"appearance.primary_color", "#6633CC"}}, {"dash"}}}, {}, {}};
  project.references.push_back(
      {"concept.front", "file:///references/front.png",
       "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
       RightsClass::licensed, AuthoringReferenceUse::visual_asset, true});
  project.targets.push_back({"portable-package",
                             {{TargetFeature::canonical_seed, true},
                              {TargetFeature::living_runtime, false}}});
  return project;
}

std::string read(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), {}};
}

std::string quote(const std::filesystem::path &path) {
  return '"' + path.string() + '"';
}

int run(std::string_view command) {
  const auto wrapped = '"' + std::string(command) + '"';
  return std::system(wrapped.c_str());
}
} // namespace

int main(int argc, char **argv) try {
  check(argc == 2, "CLI executable argument missing");
  const auto root = std::filesystem::temp_directory_path() /
                    "gspl-sprites-authoring-cli-tests";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  const auto source_path = root / "project.gspl-authoring";
  const auto source_text = source_path.string();
  save_authoring_project(fixture(), source_path);
  const auto identity = authoring_revision_identity(fixture());
  std::ostringstream output;
  std::ostringstream error;

  const auto seed_source_path = root / "import.sprite";
  const auto imported_path = root / "imported.gspl-authoring";
  {
    std::ofstream seed_source(seed_source_path);
    seed_source << "schema=gspl.sprite-seed/0.1\n"
                   "id=original.imported\nname=Imported Fox\n"
                   "classification=fictional.creature\n"
                   "rights=ORIGINAL_USER_CREATION\nentropy_root=31\n"
                   "primary_color=#123456\naccent_color=#FEDCBA\n"
                   "ability=arc|electric.projectile|20|4|2\n";
  }
  const auto seed_source_text = seed_source_path.string();
  const auto imported_text = imported_path.string();
  const std::array import{std::string_view{"authoring-import-seed"},
                          std::string_view{seed_source_text},
                          std::string_view{"authoring.imported"},
                          std::string_view{"Imported for refinement"},
                          std::string_view{imported_text}};
  check(run_authoring_cli(import, output, error) == 0 &&
            lower_authoring_project(load_authoring_project(imported_path)).ok(),
        "authoring seed import command failed");

  const std::array inspect{std::string_view{"authoring-inspect"},
                           std::string_view{source_text}};
  check(run_authoring_cli(inspect, output, error) == 0 &&
            output.str().find(identity) != std::string::npos &&
            error.str().empty(),
        "authoring inspect command failed");

  output.str({});
  const auto revised_path = root / "revision-1.gspl-authoring";
  const auto revised_text = revised_path.string();
  const std::array revise{std::string_view{"authoring-revise"},
                          std::string_view{source_text},
                          std::string_view{identity},
                          std::string_view{revised_text},
                          std::string_view{"identity.name"},
                          std::string_view{"Arcfox"},
                          std::string_view{"lock"}};
  check(run_authoring_cli(revise, output, error) == 0 &&
            load_authoring_project(revised_path).revision == 1,
        "authoring revise command failed");
  const auto stale_path = root / "stale.gspl-authoring";
  const auto stale_text = stale_path.string();
  auto stale = revise;
  stale[1] = revised_text;
  stale[3] = stale_text;
  check(run_authoring_cli(stale, output, error) == 1 &&
            !std::filesystem::exists(stale_path),
        "authoring revise accepted stale identity");

  const auto seed_path = root / "violet.seed.json";
  const auto seed_text = seed_path.string();
  const std::array lower{
      std::string_view{"authoring-lower"}, std::string_view{source_text},
      std::string_view{seed_text}, std::string_view{"violet"}};
  check(run_authoring_cli(lower, output, error) == 0 &&
            read(seed_path).find("#6633CC") != std::string::npos &&
            read(seed_path).find("electric.movement") == std::string::npos,
        "authoring variant lowering command failed");
  const auto package_path = root / "package";
  const auto package_text = package_path.string();
  const std::array build{std::string_view{"authoring-build"},
                         std::string_view{source_text},
                         std::string_view{package_text}};
  check(run_authoring_cli(build, output, error) == 0 &&
            verify_package(package_path).ok() &&
            read(package_path / "authoring-provenance.json")
                    .find("concept.front") != std::string::npos &&
            read(package_path / "target-compatibility.json")
                    .find("portable-package") != std::string::npos,
        "authoring build command failed package verification");

  const auto command =
      quote(argv[1]) + " authoring-inspect " + quote(source_path) + " > NUL";
  check(run(command) == 0,
        "process entry point did not dispatch authoring command");
  std::filesystem::remove_all(root);
  std::cout << "all GSPL Sprites authoring CLI tests passed\n";
  return 0;
} catch (const std::exception &exception) {
  std::cerr << exception.what() << '\n';
  return 1;
}
