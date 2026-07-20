#include "gspl_sprites/headless_evidence.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace gspl::sprites;

namespace {
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

SpriteIr make_headless_ir() {
  const std::string text = R"(schema=gspl.sprite-seed/0.1
id=headless.test
name=HeadlessTest
classification=test
rights=ORIGINAL_USER_CREATION
entropy_root=42
primary_color=#111111
accent_color=#222222
ability=directional-lightning|electric.projectile.directional|25|8|120
)";
  const auto seed = parse_seed(text);
  return compile(seed);
}
} // namespace

int main() try {
  const auto ir = make_headless_ir();

  // 1. run_headless_evidence runs without error
  auto trace = run_headless_evidence(ir);
  check(!trace.entity_id.empty(), "entity_id is empty");
  check(trace.seed == 42, "seed mismatch");
  check(trace.total_ticks == 150, "total_ticks mismatch");

  // 2. Trace has >= 6 form-change events
  std::size_t form_change_count = 0;
  for (const auto &ev : trace.events)
    if (ev.kind == EvidenceEventKind::form_change)
      ++form_change_count;
  check(form_change_count >= 6, "expected at least 6 form-change events");

  // 3. Trace JSON is valid JSON (starts with '{' and ends with '}')
  const auto json = write_trace_json(trace);
  check(!json.empty(), "json is empty");
  check(json.front() == '{', "json does not start with '{'");
  check(json.back() == '}', "json does not end with '}'");

  // 4. Trace is deterministic (two runs with same seed produce same events)
  const auto trace2 = run_headless_evidence(ir);
  check(trace.events.size() == trace2.events.size(), "determinism event count mismatch");
  for (std::size_t i = 0; i < trace.events.size(); ++i) {
    check(trace.events[i].tick == trace2.events[i].tick, "determinism tick mismatch");
    check(trace.events[i].kind == trace2.events[i].kind, "determinism kind mismatch");
  }

  // 5. Headless mode produces no graphical output (no dependency on graphics)
  // Already demonstrated by the fact that the test compiles and runs without
  // any graphics library (no SDL, no OpenGL, etc.)
  std::cout << "all gspl sprites headless evidence tests passed ("
            << trace.events.size() << " events, "
            << form_change_count << " form changes)\n";
  return 0;

} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
