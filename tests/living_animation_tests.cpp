#include <iostream>
#include <set>
#include <string_view>
#include "gspl_sprites/synthesis.hpp"
using namespace gspl::sprites;
int main() {
  // Test kRequiredClips table
  std::set<std::string_view> ids;
  int count = 0;
  for (const auto& e : kRequiredClips) { ids.insert(e.exact_id); ++count; }
  if (count != 9) { std::cerr << "FAIL clip count: " << count << std::endl; return 1; }
  if (!ids.contains("base_idle")) { std::cerr << "FAIL missing base_idle" << std::endl; return 1; }
  if (!ids.contains("transform_ascend")) { std::cerr << "FAIL missing transform_ascend" << std::endl; return 1; }
  if (!ids.contains("storm_attack")) { std::cerr << "FAIL missing storm_attack" << std::endl; return 1; }
  // Test LivingAnimation2dBuildResult
  LivingAnimation2dBuildResult br;
  if (br.ok()) { std::cerr << "FAIL empty ok" << std::endl; return 1; }
  br.value = LivingAnimation2d{};
  if (!br.ok()) { std::cerr << "FAIL clean not ok" << std::endl; return 1; }
  // Test GeneratedFrameSample
  GeneratedFrameSample s; s.clip_id = "test";
  if (s.clip_id != "test") { std::cerr << "FAIL sample clip" << std::endl; return 1; }
  // Test GeneratedAnimationEvent
  GeneratedAnimationEvent e; e.event_id = "release";
  if (e.event_id != "release") { std::cerr << "FAIL event id" << std::endl; return 1; }
  // Test blend_source_over
  std::uint8_t d[4] = {0,0,0,0}; blend_source_over(d, 0xFF0000FF);
  if (d[0] != 0xFF) { std::cerr << "FAIL blend R" << std::endl; return 1; }
  std::cout << "ALL PASSED" << std::endl;
  return 0;
}
