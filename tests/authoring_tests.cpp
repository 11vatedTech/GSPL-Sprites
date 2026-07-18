#include "gspl_sprites/authoring.hpp"

#include <algorithm>
#include <array>
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
          "authoring.voltfox",
          "A living electric fox with directional lightning.",
          0,
          {},
          {{"identity.stable_id", {"original.voltfox"}, 0, true},
           {"identity.name", {"Voltfox", "Arcfox"}, 0, false},
           {"identity.classification", {"fictional.creature"}, 0, true},
           {"rights.class", {"ORIGINAL_USER_CREATION"}, 0, true},
           {"entropy.root", {"17", "23"}, 0, false},
           {"appearance.primary_color", {"#2244AA", "#6633CC"}, 0, false},
           {"appearance.accent_color", {"#FFFF44"}, 0, true}},
          {{{"arc", "electric.projectile", 20, 4, 2}, true, true},
           {{"dash", "electric.movement", 10, 2, 1}, true, false}},
          {{"violet", {{"appearance.primary_color", "#6633CC"}}, {"dash"}}}};
}
} // namespace

int main() try {
  const auto project = fixture();
  check(validate_authoring_project(project).ok(),
        "valid authoring project was rejected");
  const auto identity = authoring_revision_identity(project);
  auto reordered = project;
  std::ranges::reverse(reordered.fields);
  std::ranges::reverse(reordered.abilities);
  check(identity.size() == 64 && canonicalize_authoring_project(project) ==
                                     canonicalize_authoring_project(reordered),
        "authoring project identity is not deterministic");
  const auto base = lower_authoring_project(project);
  check(base.ok() && base.seed->stable_id == "original.voltfox" &&
            base.seed->abilities.size() == 2,
        "authoring project did not lower to a canonical seed");
  const auto variant = lower_authoring_project(project, "violet");
  check(variant.ok() && variant.seed->primary_color == "#6633CC" &&
            variant.seed->abilities.size() == 1,
        "authoring variant did not apply bounded overrides");

  const std::array edits{AuthoringEdit{"identity.name", "Arcfox", true},
                         AuthoringEdit{"entropy.root", "23", std::nullopt}};
  const auto revised = revise_authoring_project(project, identity, edits);
  check(revised.revision == 1 && revised.parent_revision_identity == identity &&
            authoring_revision_identity(revised) != identity &&
            lower_authoring_project(revised).seed->name == "Arcfox",
        "authoring revision did not preserve ancestry or edits");
  bool conflict = false;
  try {
    static_cast<void>(revise_authoring_project(revised, identity, edits));
  } catch (const std::runtime_error &) {
    conflict = true;
  }
  check(conflict, "stale authoring revision was accepted");
  bool no_op = false;
  try {
    const std::array no_op_edit{
        AuthoringEdit{"identity.name", "Voltfox", std::nullopt}};
    static_cast<void>(revise_authoring_project(project, identity, no_op_edit));
  } catch (const std::invalid_argument &) {
    no_op = true;
  }
  check(no_op, "no-op authoring revision was created");
  bool locked = false;
  try {
    const std::array locked_edit{
        AuthoringEdit{"identity.stable_id", "original.voltfox", std::nullopt}};
    static_cast<void>(revise_authoring_project(project, identity, locked_edit));
  } catch (const std::invalid_argument &) {
    locked = true;
  }
  check(locked, "locked authoring field was edited");

  auto unresolved = project;
  unresolved.fields[1].selected.reset();
  const auto incomplete = lower_authoring_project(unresolved);
  check(!incomplete.ok() && incomplete.validation.diagnostics.front().code ==
                                "SPRITE_AUTHORING_UNRESOLVED",
        "unresolved mandatory authoring decision was lowered");
  auto illegal_variant = project;
  illegal_variant.variants[0].field_overrides = {
      {"identity.stable_id", "original.voltfox"}};
  check(!validate_authoring_project(illegal_variant).ok(),
        "variant overrode a locked identity field");
  std::cout << "all GSPL Sprites authoring tests passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
