// GSPL Sprites - Source Pipeline Integration Regression Test
// Reads the checked-in examples/living-voltfox/voltfox.gspl through the
// production compiler pipeline and validates the 48-frame living-animation
// result. No hang, no timeout - every phase completes deterministically.

#include "gspl/sdk.hpp"
#include "gspl/semantics.hpp"
#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/animation.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace gspl::sprites;

namespace {
static int failures = 0;
void check(bool v, const char* m) {
  if (!v) { std::cerr << "FAIL: " << m << "\n"; ++failures; }
  else { std::cout << "PASS: " << m << "\n"; }
}
std::string load_source() {
  namespace fs = std::filesystem;
  fs::path src(GSPL_SPRITES_SOURCE_DIR);
  src = src / "examples" / "living-voltfox" / "voltfox.gspl";
  std::ifstream in(src);
  if (!in) throw std::runtime_error("Cannot open: " + src.string());
  std::stringstream buf; buf << in.rdbuf();
  auto r = buf.str();
  if (r.empty()) throw std::runtime_error("Empty source: " + src.string());
  return r;
}
}

int main() try {
  std::cout << "=== Voltfox Source Pipeline Integration ===\n\n";

  std::string src;
  try { src = load_source(); check(true, "source loaded"); }
  catch (std::exception const& e) { check(false, e.what()); return 1; }
  check(src.size() > 1000, "source > 1000 bytes");
  check(src.find("module original.voltfox") != std::string::npos,
        "contains module declaration");

  gspl::GsplContext ctx;
  {
    auto buf = gspl::SourceBuffer::from_string("voltfox.gspl", src);
    ctx.compile_source(std::move(buf));
    auto const& cc = ctx.compilation_context();
    check(!ctx.has_fatal_errors(), "no fatal errors");
    check(cc.ast != nullptr, "AST produced");
    check(!cc.tokens.empty(), "tokens produced");
    auto const& canon = cc.canonical;
    check(!canon.stable_id.empty(), "stable_id nonempty");
    check(canon.stable_id == "original.voltfox", "stable_id correct");
    check(!canon.morphology.empty(), "morphology nonempty");
    check(!canon.clips.empty(), "clips nonempty");
    check(canon.forms.size() >= 2, ">= 2 forms");
    check(!canon.bones.empty(), "bones nonempty");
  }

  SpriteSeed seed;
  if (!ctx.has_fatal_errors())
    seed = gspl::SpriteSeedLowering::lower(ctx.compilation_context().canonical);
  check(!seed.stable_id.empty(), "seed stable_id nonempty");
  check(seed.stable_id == "original.voltfox", "seed stable_id correct");
  check(!seed.morphology.empty(), "morphology nonempty");
  check(seed.morphology.find("torso") != seed.morphology.end(), "has torso");
  check(seed.morphology.find("head") != seed.morphology.end(), "has head");
  check(seed.morphology.find("tail") != seed.morphology.end(), "has tail");
  check(seed.morphology.find("aura") != seed.morphology.end(), "has aura");
  check(seed.rig.has_value() && !seed.rig->bones.empty(), "rig bones nonempty");
  {
    bool hb = false, hs = false;
    for (auto const& f : seed.forms) {
      if (f.id == "base") hb = true;
      if (f.id == "storm") hs = true;
    }
    check(hb, "base form"); check(hs, "storm form");
  }
  check(!seed.transformations.empty(), "transformations nonempty");
  check(!seed.abilities.empty(), "abilities nonempty");
  check(seed.morphology.at("torso").z_order == 5, "torso z_order 5");
  check(seed.morphology.at("aura").z_order == -10, "aura z_order -10");
  check(seed.morphology.at("aura").emissive, "aura emissive true");

  {
    auto living = synthesize_living_animation2d(seed);
    check(living.ok(), "synthesis ok");
    if (living.ok() && living.value.has_value()) {
      auto const& lv = *living.value;
      check(lv.base_frames.size() == 19, "base frames == 19");
      check(lv.transformation_frames.size() == 10, "transform frames == 10");
      check(lv.storm_frames.size() == 19, "storm frames == 19");
      check(lv.all_frames.size() == 48, "total frames == 48");
      check(lv.clips.size() == 9, "generated clips == 9");
      check(lv.samples.size() == 48, "frame samples == 48");

      std::set<std::string> ids;
      for (auto const& f : lv.all_frames) ids.insert(f.id);
      check(ids.size() == lv.all_frames.size(), "unique frame IDs");

      bool all_ne = true;
      for (auto const& s : lv.samples)
        if (s.pose_hash.empty() || s.frame_hash.empty()) { all_ne = false; break; }
      check(all_ne, "pose/frame hashes nonempty");

      bool hr = false, hm = false, hc = false;
      for (auto const& ev : lv.generated_events) {
        if (ev.event_id == "release") hr = true;
        if (ev.event_id == "midpoint") hm = true;
        if (ev.event_id == "complete") hc = true;
      }
      check(hr, "release event exists");
      check(hm, "midpoint event exists");
      check(hc, "complete event exists");

      bool all_refs = true;
      for (auto const& clip : lv.clips)
        for (auto const& ref : clip.frame_ids)
          if (std::none_of(lv.all_frames.begin(), lv.all_frames.end(),
              [&](auto const& f) { return f.id == ref; }))
            { all_refs = false; break; }
      check(all_refs, "all frame references resolve");
    }
  }

  {
    gspl::GsplContext ctx2;
    auto buf2 = gspl::SourceBuffer::from_string("voltfox.gspl", src);
    ctx2.compile_source(std::move(buf2));
    auto seed2 = gspl::SpriteSeedLowering::lower(ctx2.compilation_context().canonical);
    check(canonicalize(seed) == canonicalize(seed2), "deterministic seed identity");
  }

  std::cout << "\n";
  if (failures == 0)
    std::cout << "ALL VOLTFOX SOURCE PIPELINE TESTS PASSED (48 frames, 9 clips, 48 samples)\n";
  else
    std::cout << failures << " FAILURE(S)\n";
  return failures > 0 ? 1 : 0;

} catch (std::exception const& e) {
  std::cerr << "FATAL: " << e.what() << "\n"; return 1;
}
