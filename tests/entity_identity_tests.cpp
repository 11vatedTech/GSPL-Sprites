#include "gspl_sprites/living_runtime.hpp"
#include "gspl_sprites/core.hpp"

#include <iostream>
#include <stdexcept>
#include <sstream>

using namespace gspl::sprites;
namespace { void check(bool v, const char* m) { if (!v) throw std::runtime_error(m); } }

int main() {
  try {
    LivingRuntimeState state;
    state.tick = 42;
    state.energy = 75;
    state.cooldowns["directional-lightning"] = 3;
    set_runtime_variable(state, "health", 80);

    const auto id1 = capture_entity_identity(
        state, "original.voltfox", "inst-001", "base", "", "",
        "", "", "idle");
    check(id1.entity_def_id == "original.voltfox", "entity def id mismatch");
    check(id1.instance_id == "inst-001", "instance id mismatch");
    check(id1.current_form == "base", "current form mismatch");
    check(id1.health == 80, "health mismatch");
    check(id1.resource == 75, "resource mismatch");
    check(id1.cooldowns.count("directional-lightning") && id1.cooldowns.at("directional-lightning") == 3, "cooldown mismatch");
    check(id1.behavior_state == "idle", "behavior state mismatch");

    const auto hash1 = entity_identity_hash(id1);
    check(!hash1.empty(), "identity hash empty");

    LivingRuntimeState state2;
    state2.tick = 42;
    state2.energy = 75;
    state2.cooldowns["directional-lightning"] = 3;
    set_runtime_variable(state2, "health", 80);

    const auto id2 = capture_entity_identity(
        state2, "original.voltfox", "inst-001", "base", "", "",
        "", "", "idle");
    check(entity_identities_match(id1, id2), "identical states should match");

    LivingRuntimeState state3;
    state3.tick = 99;
    state3.energy = 50;
    set_runtime_variable(state3, "health", 30);

    const auto id3 = capture_entity_identity(
        state3, "original.voltfox", "inst-001", "storm", "ascend", "",
        "", "", "storm_idle");
    check(!entity_identities_match(id1, id3), "different states should not match");

    check(id1.runtime_tick == 42, "tick should be 42");
    check(id1.active_transformation.empty(), "active transformation should be empty");
    check(id3.current_form == "storm", "storm form mismatch");
    check(id3.behavior_state == "storm_idle", "storm behavior mismatch");

    std::cout << "all GSPL Sprites entity identity tests passed" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
}
