#include "gspl_sprites/authoring.hpp"

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

AuthoringProject fixture() {
  return {"gspl.sprite-authoring/0.1",
          "authoring.io",
          "Electric fox | variant; multilingual: 雷狐",
          0,
          {},
          {{"identity.stable_id", {"original.io"}, 0, true},
           {"identity.name", {"Volt|Fox", "雷狐"}, 1, false},
           {"identity.classification", {"fictional.creature"}, 0, true},
           {"rights.class", {"ORIGINAL_USER_CREATION"}, 0, true},
           {"entropy.root", {"71"}, 0, true},
           {"appearance.primary_color", {"#1122AA"}, 0, false},
           {"appearance.accent_color", {"#FFFF44"}, 0, true}},
          {{{"arc", "electric|projectile;directed", 20, 4, 2}, true, false}},
          {{"bright", {{"appearance.primary_color", "#1122AA"}}, {}}}};
}
} // namespace

int main() try {
  const auto project = fixture();
  const auto source = serialize_authoring_project(project);
  const auto parsed = parse_authoring_project(source);
  check(serialize_authoring_project(parsed) == source &&
            authoring_revision_identity(parsed) ==
                authoring_revision_identity(project) &&
            parsed.intent == project.intent,
        "authoring source did not round-trip deterministically");
  check(source.find("%7C") != std::string::npos &&
            source.find("%E9%9B%B7") != std::string::npos,
        "authoring delimiters or UTF-8 bytes were not encoded");

  const auto root = std::filesystem::temp_directory_path() /
                    "gspl-sprites-authoring-io-tests";
  const auto path = root / "project.gspl-authoring";
  std::filesystem::remove_all(root);
  std::filesystem::create_directories(root);
  save_authoring_project(project, path);
  check(load_authoring_project(path).intent == project.intent &&
            !std::filesystem::exists(path.string() + ".staging"),
        "transactional authoring persistence failed");
  bool overwrite = false;
  try {
    save_authoring_project(project, path);
  } catch (const std::runtime_error &) {
    overwrite = true;
  }
  check(overwrite, "authoring persistence overwrote an existing project");

  bool noncanonical = false;
  try {
    static_cast<void>(parse_authoring_project(source + "\n"));
  } catch (const std::runtime_error &) {
    noncanonical = true;
  }
  check(noncanonical, "non-canonical authoring source was accepted");
  bool malformed_escape = false;
  try {
    auto malformed = source;
    const auto marker = malformed.find("%7C");
    malformed.replace(marker, 3, "%7c");
    static_cast<void>(parse_authoring_project(malformed));
  } catch (const std::runtime_error &) {
    malformed_escape = true;
  }
  check(malformed_escape, "non-canonical percent escape was accepted");
  bool malformed_utf8 = false;
  try {
    auto malformed = source;
    const auto marker = malformed.find("intent=");
    malformed.insert(marker + 7, "%C0%AF");
    static_cast<void>(parse_authoring_project(malformed));
  } catch (const std::runtime_error &) {
    malformed_utf8 = true;
  }
  check(malformed_utf8, "malformed UTF-8 authoring value was accepted");
  {
    std::ofstream tamper(path, std::ios::app);
    tamper << "unknown=record\n";
  }
  bool tampered = false;
  try {
    static_cast<void>(load_authoring_project(path));
  } catch (const std::runtime_error &) {
    tampered = true;
  }
  check(tampered, "tampered authoring project was accepted");
  std::filesystem::remove_all(root);
  std::cout << "all GSPL Sprites authoring I/O tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
