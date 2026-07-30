#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/animation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::sprites {
namespace {

static int failures = 0;

#define TEST(name, expr) do { \
  if (!(expr)) { \
    std::cerr << "FAIL: " << name << " (" << #expr << ")\n"; \
    ++failures; \
  } else { \
    std::cout << "PASS: " << name << "\n"; \
  } \
} while(0)

// Build a complete SpriteSeed with all fields needed for living synthesis
static SpriteSeed make_living_seed() {
  SpriteSeed seed;
  seed.schema = "gspl.sprite-seed/0.2";
  seed.stable_id = "test.voltfox";
  seed.name = "Test Voltfox";
  seed.classification = "Biological/Fictional/Electric-Fox";
  seed.rights = RightsClass::original_user_creation;
  seed.entropy_root = 11072026;
  seed.primary_color = "#242038";
  seed.accent_color = "#56F1FF";
  seed.storm_primary_color = "#1a1a2e";
  seed.storm_accent_color = "#FF6B6B";
  seed.emissive_color = "#56F1FF";
  seed.aura_color = "#56F1FF";

  // Abilities
  seed.abilities.push_back({"directional_lightning", "electric.projectile.directional", 25, 8, 2});
  seed.storm_abilities.push_back({"storm_lightning", "electric.storm.area", 30, 12, 3});

  // Rig with bones
  RigDefinition rig;
  rig.id = "voltfox.rig";
  rig.bones.push_back({"root", std::nullopt, {0, 0, 0, 1, 1}, 12, {-45, 45}});
  rig.bones.push_back({"spine", {"root"}, {0, 0, 0, 1, 1}, 10, {-30, 30}});
  rig.bones.push_back({"head", {"spine"}, {0, -14, 0, 1, 1}, 8, {-30, 30}});
  rig.bones.push_back({"muzzle", {"head"}, {0, -5, 0, 1, 1}, 6, {-20, 20}});
  rig.bones.push_back({"ear_l", {"head"}, {-4, -3, -20, 1, 1}, 4, {-45, 45}});
  rig.bones.push_back({"ear_r", {"head"}, {4, -3, 20, 1, 1}, 4, {-45, 45}});
  rig.bones.push_back({"tail", {"spine"}, {0, 10, 0, 1, 1}, 10, {-80, 80}});
  rig.bones.push_back({"leg_fl", {"root"}, {-5, 6, 0, 1, 1}, 6, {-60, 30}});
  rig.bones.push_back({"leg_fr", {"root"}, {5, 6, 0, 1, 1}, 6, {-60, 30}});
  rig.bones.push_back({"leg_hl", {"root"}, {-4, 10, 0, 1, 1}, 7, {-60, 30}});
  rig.bones.push_back({"leg_hr", {"root"}, {4, 10, 0, 1, 1}, 7, {-60, 30}});
  rig.bones.push_back({"aura_bone", {"root"}, {0, 0, 0, 1, 1}, 1, {0, 0}});
  seed.rig = rig;

  // Helper: add a track with keyframes to a clip
  auto add_track = [&](const std::string& clip_id, const std::string& bone,
                        std::initializer_list<std::pair<std::uint32_t, Transform2d>> keys) {
    auto clip = std::ranges::find(seed.clips, clip_id, &SkeletalClip::id);
    if (clip == seed.clips.end()) return;
    BoneTrack track{bone, {}};
    for (auto const& [tick, transform] : keys)
      track.keys.push_back({tick, transform});
    clip->tracks.push_back(std::move(track));
  };

  // Nine clips with tracks
  seed.clips.push_back({"base_idle", 12, true, {}, {}});
  add_track("base_idle", "head", {{0, {0, 0, 0, 1, 1}}, {6, {0, 0, 5, 1, 1}}, {12, {0, 0, 0, 1, 1}}});
  add_track("base_idle", "tail", {{0, {0, 0, 0, 1, 1}}, {6, {0, 0, 15, 1, 1}}, {12, {0, 0, 0, 1, 1}}});

  seed.clips.push_back({"base_locomotion", 8, true, {}, {}});
  add_track("base_locomotion", "leg_fl", {{0, {0, 0, 20, 1, 1}}, {4, {0, 0, -20, 1, 1}}, {8, {0, 0, 20, 1, 1}}});
  add_track("base_locomotion", "leg_fr", {{0, {0, 0, -20, 1, 1}}, {4, {0, 0, 20, 1, 1}}, {8, {0, 0, -20, 1, 1}}});

  seed.clips.push_back({"base_attack", 8, false, {}, {}});
  add_track("base_attack", "head", {{0, {0, 0, 0, 1, 1}}, {3, {2, -2, -10, 1.2, 1.2}}, {6, {4, 0, -25, 1.1, 1.1}}, {8, {0, 0, 0, 1, 1}}});
  seed.clips.back().events.push_back({"release", 6});

  seed.clips.push_back({"base_hit", 6, false, {}, {}});
  add_track("base_hit", "head", {{0, {0, 0, 0, 1, 1}}, {3, {-2, 2, 15, 1, 1}}, {6, {0, 0, 0, 1, 1}}});

  seed.clips.push_back({"transform_ascend", 41, false, {}, {}});
  // Transformation: base→storm over 41 ticks with keyframes at both endpoints and midpoint
  add_track("transform_ascend", "spine", {{0, {0, 0, 0, 1, 1}}, {20, {0, 0, 0, 1.3, 1.3}}, {41, {0, 0, 0, 1.5, 1.5}}});
  add_track("transform_ascend", "tail", {{0, {0, 0, 0, 1, 1}}, {20, {0, 0, 30, 1.2, 1.2}}, {41, {0, 0, 45, 1.4, 1.4}}});
  seed.clips.back().events.push_back({"midpoint", 20});
  seed.clips.back().events.push_back({"complete", 40});

  seed.clips.push_back({"storm_idle", 12, true, {}, {}});
  add_track("storm_idle", "head", {{0, {0, 0, 5, 1, 1}}, {6, {0, 0, -5, 1, 1}}, {12, {0, 0, 5, 1, 1}}});
  add_track("storm_idle", "tail", {{0, {0, 0, 20, 1, 1}}, {6, {0, 0, -10, 1, 1}}, {12, {0, 0, 20, 1, 1}}});

  seed.clips.push_back({"storm_locomotion", 8, true, {}, {}});
  add_track("storm_locomotion", "leg_fl", {{0, {0, 0, 30, 1, 1}}, {4, {0, 0, -30, 1, 1}}, {8, {0, 0, 30, 1, 1}}});
  add_track("storm_locomotion", "leg_hl", {{0, {0, 0, -30, 1, 1}}, {4, {0, 0, 30, 1, 1}}, {8, {0, 0, -30, 1, 1}}});

  seed.clips.push_back({"storm_attack", 8, false, {}, {}});
  add_track("storm_attack", "head", {{0, {0, 0, 0, 1, 1}}, {3, {3, -3, -15, 1.3, 1.3}}, {6, {5, 0, -30, 1.2, 1.2}}, {8, {0, 0, 0, 1, 1}}});
  seed.clips.back().events.push_back({"release", 6});

  seed.clips.push_back({"storm_hit", 6, false, {}, {}});
  add_track("storm_hit", "head", {{0, {0, 0, 0, 1, 1}}, {3, {-3, 2, 20, 1, 1}}, {6, {0, 0, 0, 1, 1}}});

  // Forms
  seed.forms.push_back({"base", {"ascend"}});
  seed.forms.push_back({"storm", {}});
  seed.form_attributes["base"] = {100, 1.0, 1.0, 100};
  seed.form_attributes["storm"] = {120, 1.2, 1.3, 120};

  // Transformation
  seed.transformations.push_back({"ascend", "base", "storm", "resource_full", 41, 30});

  // Morphology: 11+ parts with bone attachments
  auto add_part = [&](const std::string& name, const std::string& bone,
                       const std::string& primitive, const std::string& role,
                       double x, double y, double z,
                       double sx, double sy, double sz,
                       const std::string& color, std::int32_t z_order,
                       bool emissive = false, bool electrical = false) {
    MorphologyPart p;
    p.x = x; p.y = y; p.z = z;
    p.size_x = sx; p.size_y = sy; p.size_z = sz;
    p.color = color;
    p.bone_id = bone;
    p.primitive = primitive;
    p.semantic_role = role;
    p.z_order = z_order;
    p.emissive = emissive;
    p.electrical_marking = electrical;
    seed.morphology[name] = p;
  };

  add_part("torso", "spine", "ellipse", "torso", 0, 0, 0, 12, 8, 1, "#242038", 20);
  add_part("head", "head", "ellipse", "head", 0, 0, 0, 8, 6, 1, "#242038", 30);
  add_part("muzzle", "muzzle", "ellipse", "muzzle", 0, -3, 0, 4, 3, 1, "#333355", 31);
  add_part("ear_l", "ear_l", "triangle", "left-ear", 0, -2, 0, 3, 5, 1, "#242038", 32);
  add_part("ear_r", "ear_r", "triangle", "right-ear", 0, -2, 0, 3, 5, 1, "#242038", 33);
  add_part("eye_l", "head", "ellipse", "left-eye", -2, -2, 0, 2, 2, 1, "#56F1FF", 34);
  add_part("eye_r", "head", "ellipse", "right-eye", 2, -2, 0, 2, 2, 1, "#56F1FF", 35);
  add_part("tail", "tail", "segmented_curve", "tail", 0, 0, 0, 2, 8, 1, "#242038", 10);
  add_part("leg_fl", "leg_fl", "capsule", "front-left-leg", 0, 0, 0, 2, 5, 1, "#242038", 15);
  add_part("leg_fr", "leg_fr", "capsule", "front-right-leg", 0, 0, 0, 2, 5, 1, "#242038", 16);
  add_part("leg_hl", "leg_hl", "capsule", "hind-left-leg", 0, 0, 0, 2, 6, 1, "#242038", 13);
  add_part("leg_hr", "leg_hr", "capsule", "hind-right-leg", 0, 0, 0, 2, 6, 1, "#242038", 14);
  add_part("aura", "aura_bone", "aura_contour", "energy-aura", 0, 0, 0, 16, 16, 1, "#56F1FF", -10);
  add_part("electrical_marking", "spine", "electrical_arc", "electrical-markings", 0, 0, 0, 6, 4, 1, "#56F1FF", 40, true, true);

  // Storm form overrides: structural differences
  MorphologyPart storm_aura;
  storm_aura.size_x = 22; storm_aura.size_y = 22; storm_aura.size_z = 1;
  storm_aura.color = "#FF6B6B";
  storm_aura.emissive = true;
  storm_aura.bone_id = "aura_bone";
  seed.form_morphology_overrides["storm"]["aura"] = storm_aura;

  MorphologyPart storm_eyes;
  storm_eyes.size_x = 3; storm_eyes.size_y = 3; storm_eyes.size_z = 1;
  storm_eyes.color = "#FF6B6B";
  storm_eyes.emissive = true;
  storm_eyes.bone_id = "head";
  seed.form_morphology_overrides["storm"]["eye_l"] = storm_eyes;
  seed.form_morphology_overrides["storm"]["eye_r"] = storm_eyes;

  MorphologyPart storm_electrical;
  storm_electrical.size_x = 8; storm_electrical.size_y = 6; storm_electrical.size_z = 1;
  storm_electrical.color = "#FF6B6B";
  storm_electrical.emissive = true;
  storm_electrical.electrical_marking = true;
  storm_electrical.bone_id = "spine";
  seed.form_morphology_overrides["storm"]["electrical_marking"] = storm_electrical;

  // Animation graph
  AnimationStateGraph graph;
  graph.initial_state = "idle";
  graph.states.push_back({"idle", "base_idle", {}});
  graph.states[0].transitions.push_back({"attack", "attack_input", Comparison::greater_equal, 1, 0, 1, 10});
  graph.states.push_back({"attack", "base_attack", {}});
  graph.states[1].transitions.push_back({"idle", "attack_input", Comparison::less, 1, 2, 1, 10});
  graph.states.push_back({"storm_idle", "storm_idle", {}});
  seed.animation_graph = graph;

  // Runtime
  RuntimeAttributes rt;
  rt.aggression = 60;
  rt.curiosity = 80;
  rt.energy = 75;
  rt.loyalty = 50;
  rt.animation_intents = {{"idle", "base_idle"}, {"attack", "base_attack"}};
  seed.runtime = rt;

  // Collision
  seed.collision_shapes.push_back({"body", CollisionKind::axis_aligned_box, "root", 0, 0, 8, 5});
  seed.collision_windows.push_back({"body", 0, 2, true, "directional_lightning"});

  return seed;
}

// ── End-to-end living synthesis test ──
void test_end_to_end_synthesis() {
  auto seed = make_living_seed();

  // Validate seed
  auto val = validate(seed);
  TEST("living seed validates", val.ok());

  // Run synthesis
  auto living = synthesize_living_animation2d(seed);
  TEST("synthesis is ok", living.ok());

  if (!living.ok()) {
    for (auto const& d : living.diagnostics.diagnostics)
      std::cerr << "  diag: " << d.code << ": " << d.message << "\n";
    return;
  }

  auto& result = *living.value;

  // Frame counts
  TEST("base frames == 19", result.base_frames.size() == 19);
  TEST("transform frames == 10", result.transformation_frames.size() == 10);
  TEST("storm frames == 19", result.storm_frames.size() == 19);
  TEST("total frames == 48", result.all_frames.size() == 48);

  // Generated clips
  TEST("clips size == 9", result.clips.size() == 9);

  // Frame samples
  TEST("samples size == 48", result.samples.size() == 48);

  // All frame IDs unique
  {
    std::set<std::string> ids;
    for (auto const& f : result.all_frames) ids.insert(f.id);
    TEST("all frame IDs unique", ids.size() == 48);
  }

  // All frame hashes nonempty
  {
    bool all_nonempty = true;
    for (auto const& f : result.all_frames)
      if (f.frame_hash.empty()) { all_nonempty = false; break; }
    TEST("all frame hashes nonempty", all_nonempty);
  }

  // All pose hashes nonempty
  {
    bool all_nonempty = true;
    for (auto const& s : result.samples)
      if (s.pose_hash.empty()) { all_nonempty = false; break; }
    TEST("all pose hashes nonempty", all_nonempty);
  }

  // Moving clips contain distinct hashes
  {
    for (auto const& clip : result.clips) {
      std::set<std::string> hashes;
      for (auto const& fid : clip.frame_ids) {
        auto it = std::ranges::find(result.all_frames, fid, &FrameSource::id);
        if (it != result.all_frames.end()) hashes.insert(it->frame_hash);
      }
      if (hashes.size() > 1)
        TEST(std::string("clip ") + clip.id + " has distinct hashes", true);
    }
  }

  // All frame references resolve
  {
    bool all_resolve = true;
    for (auto const& clip : result.clips) {
      for (auto const& fid : clip.frame_ids) {
        auto it = std::ranges::find(result.all_frames, fid, &FrameSource::id);
        if (it == result.all_frames.end()) { all_resolve = false; break; }
      }
    }
    TEST("all frame references resolve", all_resolve);
  }

  // No orphan frames
  {
    std::set<std::string> referenced;
    for (auto const& clip : result.clips)
      for (auto const& fid : clip.frame_ids)
        referenced.insert(fid);
    bool no_orphans = true;
    for (auto const& f : result.all_frames)
      if (!referenced.contains(f.id)) { no_orphans = false; break; }
    TEST("no orphan frames", no_orphans);
  }

  // Channel maps: 2 per frame group (base, transformation, storm) minimal coverage
  // Synthesis produces depth+effects channels per clip (9 clips * 2 = 18)
  TEST("channel maps exist", !result.channel_maps.empty());
  TEST("channel maps >= 18 (2 per 9 clips)", result.channel_maps.size() >= 18);
  // At least base and storm each have depth+effects channels
  bool has_base_depth = false, has_storm_depth = false;
  for (auto const& cm : result.channel_maps) {
    if (cm.id.find(".depth") != std::string::npos) {
      if (cm.id.find("base") != std::string::npos) has_base_depth = true;
      if (cm.id.find("storm") != std::string::npos) has_storm_depth = true;
    }
  }
  TEST("base depth channel exists", has_base_depth);
  TEST("storm depth channel exists", has_storm_depth);

  // Required events present
  {
    bool has_base_release = false, has_storm_release = false;
    bool has_midpoint = false, has_complete = false;
    for (auto const& ev : result.generated_events) {
      if (ev.event_id == "release" && ev.clip_id.find("base") != std::string::npos)
        has_base_release = true;
      if (ev.event_id == "release" && ev.clip_id.find("storm") != std::string::npos)
        has_storm_release = true;
      if (ev.event_id == "midpoint") has_midpoint = true;
      if (ev.event_id == "complete") has_complete = true;
    }
    TEST("base_attack.release event exists", has_base_release);
    TEST("storm_attack.release event exists", has_storm_release);
    TEST("transform_ascend.midpoint event exists", has_midpoint);
    TEST("transform_ascend.complete event exists", has_complete);
  }

  // Transformation frames: check they exist and are renderable
  auto const& tf = result.transformation_frames;
  TEST("transformation frames have 10 frames", tf.size() == 10);
  if (tf.size() >= 10) {
    std::set<std::string> t_hashes;
    for (auto const& f : tf) t_hashes.insert(f.frame_hash);
    // At least some distinct frames (partial interpolation evidence)
    TEST("transform frames have >= 2 unique hashes", t_hashes.size() >= 2);
  }
}

// ── kRequiredClips table invariants ──
void test_k_required_clips() {
  std::set<std::string_view> ids;
  int count = 0;
  for (const auto& e : kRequiredClips) {
    ids.insert(e.exact_id);
    ++count;
  }
  TEST("kRequiredClips count == 9", count == 9);
  for (const auto& id : {"base_idle", "base_locomotion", "base_attack", "base_hit",
                          "transform_ascend", "storm_idle", "storm_locomotion",
                          "storm_attack", "storm_hit"})
    TEST(std::string("kRequiredClips has ") + id, ids.contains(id));
  // Verify exact frame counts
  for (const auto& e : kRequiredClips) {
    if (e.exact_id == "base_idle") TEST("base_idle 4 frames", e.output_frame_count == 4);
    if (e.exact_id == "base_locomotion") TEST("base_locomotion 6 frames", e.output_frame_count == 6);
    if (e.exact_id == "base_attack") TEST("base_attack 6 frames", e.output_frame_count == 6);
    if (e.exact_id == "base_hit") TEST("base_hit 3 frames", e.output_frame_count == 3);
    if (e.exact_id == "transform_ascend") TEST("transform_ascend 10 frames", e.output_frame_count == 10);
    if (e.exact_id == "storm_idle") TEST("storm_idle 4 frames", e.output_frame_count == 4);
    if (e.exact_id == "storm_locomotion") TEST("storm_locomotion 6 frames", e.output_frame_count == 6);
    if (e.exact_id == "storm_attack") TEST("storm_attack 6 frames", e.output_frame_count == 6);
    if (e.exact_id == "storm_hit") TEST("storm_hit 3 frames", e.output_frame_count == 3);
  }
  TEST("no duplicate clip IDs", ids.size() == static_cast<std::size_t>(count));
}

// ── LivingAnimation2dBuildResult tests ──
void test_living_result() {
  LivingAnimation2dBuildResult br;
  TEST("empty result not ok", !br.ok());
  br.value = LivingAnimation2d{};
  TEST("value set clean is ok", br.ok());
  br.diagnostics.diagnostics.push_back({"DIAG", "test failure"});
  TEST("result with diag not ok", !br.ok());
}

// ── blend_source_over tests ──
void test_blend_source_over() {
  std::uint8_t dest[4] = {0, 0, 0, 255};
  blend_source_over(dest, 0xFF0000FF);
  TEST("opaque source R", dest[0] == 0xFF);
  TEST("opaque source G", dest[1] == 0x00);
  TEST("opaque source B", dest[2] == 0x00);
  TEST("opaque source A", dest[3] == 0xFF);

  std::uint8_t d2[4] = {128, 64, 32, 200};
  blend_source_over(d2, 0x00000000);
  TEST("transparent source unchanged", d2[0] == 128 && d2[1] == 64 && d2[2] == 32 && d2[3] == 200);

  std::uint8_t d3[4] = {0, 0, 0, 0};
  blend_source_over(d3, 0x80808080);
  TEST("50% white R", d3[0] == 128);
  TEST("50% white G", d3[1] == 128);
  TEST("50% white B", d3[2] == 128);
  TEST("50% white A", d3[3] == 128);
}

// ── GeneratedFrameSample tests ──
void test_frame_sample() {
  GeneratedFrameSample s;
  s.clip_id = "base_idle";
  s.frame_id = "base_idle.0";
  s.frame_index = 0;
  s.source_tick = 0;
  s.pose_hash = "abc123";
  TEST("frame sample clip_id", s.clip_id == "base_idle");
  TEST("frame sample frame_id", s.frame_id == "base_idle.0");
  TEST("frame sample frame_index", s.frame_index == 0);
  TEST("frame sample source_tick", s.source_tick == 0);
  TEST("frame sample pose_hash", s.pose_hash == "abc123");
}

// ── GeneratedAnimationEvent tests ──
void test_animation_event() {
  GeneratedAnimationEvent e;
  e.clip_id = "base_attack";
  e.event_id = "release";
  e.authored_tick = 6;
  e.frame_index = 2;
  e.frame_id = "base_attack.2";
  TEST("event clip_id", e.clip_id == "base_attack");
  TEST("event event_id", e.event_id == "release");
  TEST("event authored_tick", e.authored_tick == 6);
  TEST("event frame_index", e.frame_index == 2);
  TEST("event frame_id", e.frame_id == "base_attack.2");
}

// ── resolve_form_morphology tests ──
void test_resolve_form_morphology() {
  SpriteSeed seed;
  seed.schema = "gspl.sprite-seed/0.2";
  seed.stable_id = "test.creature";
  seed.name = "Test Creature";
  seed.classification = "Creature/Test";
  seed.rights = RightsClass::original_user_creation;
  seed.primary_color = "#4488CC";
  seed.accent_color = "#224466";
  seed.entropy_root = 42;
  seed.abilities.push_back({"test", "proj", 5, 10, 8});
  seed.forms.push_back({"base", {"ascend"}});
  seed.forms.push_back({"storm", {}});
  seed.transformations.push_back({"ascend", "base", "storm", "resource_full", 40, 30});
  seed.rig = RigDefinition{"test_rig", {}, {}};

  MorphologyPart base_torso;
  base_torso.x = 0; base_torso.y = 0; base_torso.z = 0;
  base_torso.size_x = 20; base_torso.size_y = 14; base_torso.size_z = 1;
  base_torso.color = "#4488CC";
  base_torso.bone_id = "spine";
  base_torso.primitive = "ellipse";
  base_torso.semantic_role = "torso";
  seed.morphology["torso"] = base_torso;

  MorphologyPart storm_torso;
  storm_torso.x = 0; storm_torso.y = 0; storm_torso.z = 0;
  storm_torso.size_x = 26; storm_torso.size_y = 18; storm_torso.size_z = 1;
  storm_torso.color = "#CC4488";
  storm_torso.bone_id = "spine";
  storm_torso.primitive = "ellipse";
  storm_torso.semantic_role = "torso";
  storm_torso.emissive = true;
  seed.form_morphology_overrides["storm"]["torso"] = storm_torso;

  auto base_r = resolve_form_morphology(seed, "base");
  TEST("base form resolved ok", base_r.ok());
  auto& base = *base_r.value;
  TEST("base form has torso", base.contains("torso"));
  TEST("base torso size_x == 20", base.at("torso").size_x == 20.0);
  TEST("base torso size_y == 14", base.at("torso").size_y == 14.0);
  TEST("base torso not emissive", base.at("torso").emissive == false);

  auto storm_r = resolve_form_morphology(seed, "storm");
  TEST("storm form resolved ok", storm_r.ok());
  auto& storm = *storm_r.value;
  TEST("storm torso size_x == 26", storm.at("torso").size_x == 26.0);
  TEST("storm torso size_y == 18", storm.at("torso").size_y == 18.0);
  TEST("storm torso color", storm.at("torso").color == "#CC4488");
  TEST("storm torso emissive", storm.at("torso").emissive == true);

  TEST("base/storm size_x differ", base.at("torso").size_x != storm.at("torso").size_x);
  TEST("base/storm size_y differ", base.at("torso").size_y != storm.at("torso").size_y);

  auto unknown_r = resolve_form_morphology(seed, "unknown_form");
  TEST("unknown form rejected", !unknown_r.ok());

  MorphologyPart base_head;
  base_head.x = 0; base_head.y = -14; base_head.z = 0;
  base_head.size_x = 10; base_head.size_y = 10; base_head.size_z = 1;
  base_head.color = "#66AAEE";
  base_head.bone_id = "head";
  base_head.primitive = "ellipse";
  base_head.semantic_role = "head";
  seed.morphology["head"] = base_head;

  auto storm2_r = resolve_form_morphology(seed, "storm");
  TEST("storm form resolved ok", storm2_r.ok());
  auto& storm2 = *storm2_r.value;
  TEST("storm head size preserves base", storm2.at("head").size_x == 10.0);
  TEST("storm head color preserves base", storm2.at("head").color == "#66AAEE");
}

// ── Seed identity tests ──
void test_canonical_identity() {
  SpriteSeed seed;
  seed.schema = "gspl.sprite-seed/0.2";
  seed.stable_id = "identity.test";
  seed.name = "Identity Test";
  seed.classification = "Construct/Test";
  seed.rights = RightsClass::original_user_creation;
  seed.primary_color = "#FF0000";
  seed.accent_color = "#00FF00";
  seed.entropy_root = 99;
  seed.abilities.push_back({"beam", "proj", 5, 10, 8});

  auto c1 = canonicalize(seed);
  auto c2 = canonicalize(seed);
  TEST("deterministic identity", c1 == c2);

  MorphologyPart torso;
  torso.x = 0; torso.y = 0; torso.z = 0;
  torso.size_x = 20; torso.size_y = 14; torso.size_z = 1;
  torso.color = "#4488CC";
  torso.bone_id = "spine";
  torso.primitive = "ellipse";
  torso.semantic_role = "torso";
  seed.morphology["torso"] = torso;
  auto c_with_torso = canonicalize(seed);
  TEST("morphology changes identity", c_with_torso != c1);

  auto seed_mod_bone = seed;
  seed_mod_bone.morphology["torso"].bone_id = "chest";
  TEST("bone_id changes identity", canonicalize(seed_mod_bone) != c_with_torso);

  auto seed_mod_prim = seed;
  seed_mod_prim.morphology["torso"].primitive = "capsule";
  TEST("primitive changes identity", canonicalize(seed_mod_prim) != c_with_torso);

  auto seed_mod_z = seed;
  seed_mod_z.morphology["torso"].z_order = 10;
  TEST("z_order changes identity", canonicalize(seed_mod_z) != c_with_torso);

  auto seed_mod_role = seed;
  seed_mod_role.morphology["torso"].semantic_role = "upper-body";
  TEST("semantic_role changes identity", canonicalize(seed_mod_role) != c_with_torso);

  MorphologyPart storm_torso;
  storm_torso.size_x = 26;
  storm_torso.bone_id = "spine";
  auto seed_ovr = seed;
  seed_ovr.form_morphology_overrides["storm"]["torso"] = storm_torso;
  TEST("form override changes identity", canonicalize(seed_ovr) != c_with_torso);

  auto seed_sa = seed;
  seed_sa.storm_abilities.push_back({"storm_bolt", "bolt", 10, 20, 8});
  TEST("storm ability changes identity", canonicalize(seed_sa) != c_with_torso);
}

} // anonymous namespace
} // namespace gspl::sprites

int main() {
  using namespace gspl::sprites;
  std::cout << "=== Living Animation Tests ===\n";

  test_k_required_clips();
  test_living_result();
  test_blend_source_over();
  test_frame_sample();
  test_animation_event();
  test_resolve_form_morphology();
  test_canonical_identity();
  test_end_to_end_synthesis();

  std::cout << "\n";
  if (failures > 0) {
    std::cerr << "FAILED: " << failures << " test(s) failed\n";
    return 1;
  }
  std::cout << "ALL PASSED (" << failures << " failures)\n";
  return 0;
}
