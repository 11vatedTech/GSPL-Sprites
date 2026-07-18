#include "gspl_sprites/runtime_replication.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
LivingRuntimeProgram program() {
  return {"replication.runtime", 60, 8, {{"idle", 0, {}}},
          {{"wait", "idle", 1, {}, {}, 1, 0, 0, true, {}}}};
}
template <class Function> bool rejects(Function function) {
  try { function(); } catch (const std::exception &) { return true; }
  return false;
}
} // namespace

int main() {
  try {
    const auto definition = program();
    LivingRuntimeState base;
    auto authoritative = base;
    set_runtime_variable(authoritative, "world.phase", 2);
    (void)step_living_runtime(definition, authoritative);
    const auto update =
        make_runtime_replication_update(definition, base, authoritative);
    const auto encoded = serialize_runtime_replication_update(update);
    const auto decoded = deserialize_runtime_replication_update(encoded);
    auto local = base;
    apply_runtime_replication_update(definition, local, decoded);
    check(serialize_living_runtime_state(definition, local) ==
              serialize_living_runtime_state(definition, authoritative),
          "authoritative update did not converge state");
    check(rejects([&] { apply_runtime_replication_update(definition, local, decoded); }),
          "stale replication base was accepted");
    auto divergent = base;
    set_runtime_variable(divergent, "world.phase", 9);
    check(rejects([&] { apply_runtime_replication_update(definition, divergent, decoded); }),
          "divergent replication base was accepted");
    auto corrupt = encoded;
    corrupt.back() = corrupt.back() == '0' ? '1' : '0';
    check(rejects([&] { (void)deserialize_runtime_replication_update(corrupt); }),
          "corrupt replication payload was accepted");
    auto rollback = base;
    (void)step_living_runtime(definition, rollback);
    check(rejects([&] { (void)make_runtime_replication_update(definition, rollback, base); }),
          "rollback update was created");
    auto other = definition;
    other.actions[0].duration_ticks = 2;
    auto other_local = base;
    check(rejects([&] { apply_runtime_replication_update(other, other_local, decoded); }),
          "update for another program was accepted");
    check(rejects([&] { (void)deserialize_runtime_replication_update(encoded, 16); }),
          "replication byte limit was ignored");
    std::cout << "all gspl sprites runtime replication tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
