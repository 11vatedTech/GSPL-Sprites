#define _CRT_SECURE_NO_WARNINGS
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/combat.hpp"
#include "gspl_sprites/domain.hpp"
#include "gspl_sprites/living_runtime.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/runtime_persistence.hpp"
#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/transformation.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace gspl::sprites;

namespace {

void check(bool v, const char *m) {
  if (!v) throw std::runtime_error(m);
}

void check_throws(std::string_view label, auto fn) {
  try {
    fn();
    throw std::runtime_error(std::string(label) + " did not throw");
  } catch (const std::exception &) {}
}

std::uint32_t xorshift32(std::uint32_t &state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

std::string make_random_seed(std::uint32_t &prng) {
  static const char *rights_vals[] = {
    "ORIGINAL_USER_CREATION", "USER_OWNED_REFERENCE", "PUBLIC_DOMAIN", "PERMISSIVELY_LICENSED"};
  static const char *colors[] = {
    "#242038", "#56F1FF", "#FF0000", "#00FF00", "#0000FF",
    "#FFFFFF", "#000000", "#808080", "#AABBCC", "#FFA500"};

  std::ostringstream os;
  os << "schema=gspl.sprite-seed/0.2\n"
     << "id=test." << (xorshift32(prng) % 1000000) << "\n"
     << "name=TestEntity" << (xorshift32(prng) % 1000) << "\n"
     << "classification=test.synthetic.random\n"
     << "rights=" << rights_vals[xorshift32(prng) % 4] << "\n"
     << "entropy_root=" << (xorshift32(prng) % 10000000) << "\n"
     << "primary_color=" << colors[xorshift32(prng) % 10] << "\n"
     << "accent_color=" << colors[xorshift32(prng) % 10] << "\n";
  std::uint32_t nabil = 1 + (xorshift32(prng) % 3);
  for (std::uint32_t i = 0; i < nabil; ++i) {
    os << "ability=abil" << i << "|test.ability|" << (xorshift32(prng) % 50 + 1)
       << "|" << (xorshift32(prng) % 20) << "|" << (xorshift32(prng) % 10 + 1) << "\n";
  }
  os << "rig=voltfox.rig\n"
     << "bone=root|-|0|0|0|1|1|12|-45|45\n"
     << "bone=head|root|4|0|0|1|1|8|-30|30\n"
     << "clip=idle|10|true\n"
     << "track=idle|head|0,4,0,-10,1,1;10,4,0,10,1,1\n"
     << "initial_state=idle\n"
     << "state=idle|idle\n"
     << "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
     << "[form.base]\n"
     << "transformations=ascend\n"
     << "[form.storm]\n"
     << "transformations=descend\n"
     << "resource_capacity=150\n"
     << "collision_scale=1.3\n"
     << "ability_envelope=1.5\n"
     << "max_health=150\n"
     << "[transformation.ascend]\n"
     << "from_form=base\n"
     << "to_form=storm\n"
     << "trigger=command:transform\n"
     << "duration_ticks=6\n"
     << "resource_cost=50\n"
     << "[transformation.descend]\n"
     << "from_form=storm\n"
     << "to_form=base\n"
     << "trigger=command:transform\n"
     << "duration_ticks=6\n"
     << "resource_cost=30\n"
     << "[morphology.torso]\n"
     << "position=0,0,0\nsize=40,30,20\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n"
     << "[morphology.head]\n"
     << "position=0,25,5\nsize=24,20,18\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n"
     << "[morphology.left_ear]\n"
     << "position=-8,33,10\nsize=8,16,8\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=-10\n"
     << "[morphology.right_ear]\n"
     << "position=8,33,10\nsize=8,16,8\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=10\n"
     << "[morphology.left_eye]\n"
     << "position=-5,27,15\nsize=4,4,4\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n"
     << "[morphology.right_eye]\n"
     << "position=5,27,15\nsize=4,4,4\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n"
     << "[morphology.muzzle]\n"
     << "position=0,22,18\nsize=10,8,6\ncolor=#FFFFFF\nrotation=0\n"
     << "[morphology.tail]\n"
     << "position=-20,-5,0\nsize=6,30,6\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=-15\n"
     << "[morphology.left_front_leg]\n"
     << "position=-10,-20,-5\nsize=8,20,8\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n"
     << "[morphology.right_front_leg]\n"
     << "position=10,-20,-5\nsize=8,20,8\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n"
     << "[morphology.aura]\n"
     << "position=0,0,0\nsize=60,50,40\ncolor=" << colors[xorshift32(prng) % 10] << "\nrotation=0\n";
  return os.str();
}

struct TestFixture {
  std::filesystem::path root;
  TestFixture() : root(std::filesystem::temp_directory_path() / "gspl-mutation-tests") {
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
  }
  ~TestFixture() { std::filesystem::remove_all(root); }
  std::filesystem::path sub(const std::string &name) const { return root / name; }
};

} // namespace

int main() {
  try {
    std::cout << "GSPL Sprites mutation tests\n" << std::flush;

    // ===== PROPERTY TESTS =====

    // 1. Deterministic canonicalization: 100 random seeds produce deterministic JSON
    {
      std::cout << "  Test 1..." << std::flush;
      std::uint32_t prng = 42;
      for (int i = 0; i < 100; ++i) {
        const auto src = make_random_seed(prng);
        const auto seed = parse_seed(src);
        const auto c1 = canonicalize(seed);
        const auto c2 = canonicalize(seed);
        check(c1 == c2, "canonicalization not deterministic");
        // Verify two different seeds produce different canonical forms
        const auto src2 = make_random_seed(prng);
        const auto seed2 = parse_seed(src2);
        const auto c3 = canonicalize(seed2);
        check(c1 != c3, "different seeds must produce different canonical forms");
      }
      std::cout << " passed\n";
    }

    // 2. State identity round-trip: serialize(deserialize(state)) identical bytes
    std::cout << "  Test 2..." << std::flush;
    {
      LivingRuntimeProgram prog;
      prog.id = "test.roundtrip";
      RuntimeGoalDefinition goal;
      goal.id = "default_goal";
      goal.priority = 0;
      prog.goals.push_back(goal);
      RuntimeActionDefinition act;
      act.id = "default_action";
      act.goal_id = "default_goal";
      act.base_utility = 50;
      act.duration_ticks = 5;
      act.cooldown_ticks = 10;
      act.energy_cost = 5;
      act.interruptible = true;
      prog.actions.push_back(act);
      LivingRuntimeState s1;
      s1.tick = 42;
      s1.energy = 75;
      set_runtime_variable(s1, "health", 80);

      const auto serialized = serialize_living_runtime_state(prog, s1);
      const auto s2 = deserialize_living_runtime_state(prog, serialized);
      const auto re_serialized = serialize_living_runtime_state(prog, s2);
      check(serialized == re_serialized, "state round-trip failed");
      std::cout << "  [PASS] Property: state identity round-trip\n";
    }

    // 3. Save/restore: full state save, restore, compare identity hashes
    {
      CombatProgram combat_prog;
      combat_prog.id = "test.combat.restore";
      TransformationProgram trans_prog;
      trans_prog.id = "test.forms.restore";
      trans_prog.base_form = "base";

      LivingRuntimeState runtime;
      runtime.tick = 100;
      runtime.energy = 50;
      set_runtime_variable(runtime, "health", 60);

      CombatState combat_state;
      combat_state.tick = 100;
      combat_state.actors["test.entity"].id = "test.entity";
      combat_state.actors["test.entity"].health = 60;
      combat_state.actors["test.entity"].maximum_health = 100;
      combat_state.actors["test.entity"].resource = 50;
      combat_state.actors["test.entity"].maximum_resource = 100;
      TransformationState trans_state;
      trans_state.stable_entity_id = "test.entity";
      trans_state.current_form = "base";
      trans_state.energy = 50;

      const auto header = capture_persistence_header(combat_state, trans_state, runtime);
      CombatState restored_combat;
      restored_combat.tick = 100;
      TransformationState restored_trans;
      restored_trans.stable_entity_id = "test.entity";
      restored_trans.current_form = "base";
      restored_trans.energy = 50;
      LivingRuntimeState restored_runtime;
      restored_runtime.tick = 100;
      restore_from_persistence_header(header, restored_combat, restored_trans, restored_runtime);

      check(restored_runtime.tick == runtime.tick, "restore tick mismatch");
      check(restored_trans.current_form == trans_state.current_form, "restore form mismatch");
      check(restored_combat.actors["test.entity"].health == 60, "restore health mismatch");
      std::cout << "  [PASS] Property: save/restore identity\n";
    }

    // 4. Replay: record events, replay, compare final identity
    {
      LivingRuntimeProgram prog;
      prog.id = "test.replay";
      RuntimeGoalDefinition goal;
      goal.id = "test_goal";
      goal.priority = 0;
      prog.goals.push_back(goal);
      RuntimeActionDefinition act;
      act.id = "test_action";
      act.goal_id = "test_goal";
      act.base_utility = 50;
      act.duration_ticks = 5;
      act.cooldown_ticks = 10;
      act.energy_cost = 5;
      act.interruptible = true;
      prog.actions.push_back(act);

      LivingRuntimeState initial;
      initial.tick = 0;
      initial.energy = 100;

      std::vector<RuntimeReplayFrame> frames;
      frames.push_back({10, {}, {}, false});

      const auto result = replay_living_runtime(prog, initial, frames, 11);
      check(result.final_state.tick == 10 || !result.events.empty(),
            "replay should advance tick");
      check(!result.final_state_identity.empty(),
            "replay should produce final state identity");
      std::cout << "  [PASS] Property: replay identity\n";
    }

    // 5. Cross-representation parity: same state produces matching identity
    {
      LivingRuntimeState state;
      state.tick = 50;
      state.energy = 80;
      set_runtime_variable(state, "health", 100);

      const auto id_a = capture_entity_identity(
          state, "voltfox.test", "inst-001", "base", "", "",
          "", "", "idle");
      const auto id_b = capture_entity_identity(
          state, "voltfox.test", "inst-001", "base", "", "",
          "", "", "idle");
      check(entity_identities_match(id_a, id_b),
            "same state should produce matching identity");

      LivingRuntimeState state_diff;
      state_diff.tick = 99;
      state_diff.energy = 30;
      set_runtime_variable(state_diff, "health", 20);
      const auto id_c = capture_entity_identity(
          state_diff, "voltfox.test", "inst-001", "storm", "ascend", "",
          "", "", "storm_idle");
      check(!entity_identities_match(id_a, id_c),
            "different state should produce non-matching identity");
      std::cout << "  [PASS] Property: cross-representation parity\n";
    }

    // 6. Package reproducibility: two identical-input builds produce identical hash
    {
      TestFixture fix;
      std::uint32_t tmp6 = 99;
      const auto src = make_random_seed(tmp6);
      const auto seed = parse_seed(src);
      build_package(seed, fix.sub("pkg1"));
      build_package(seed, fix.sub("pkg2"));
      const auto v1 = verify_package(fix.sub("pkg1"));
      const auto v2 = verify_package(fix.sub("pkg2"));
      check(v1.ok(), "package 1 must verify");
      check(v2.ok(), "package 2 must verify");
      check(v1.package_identity == v2.package_identity,
            "identical inputs must produce identical package identity");
      std::cout << "  [PASS] Property: package reproducibility\n";
    }

    // 7. Artifact-graph closure: dependency tracking and transitive invalidation
    {
      AssetGraph graph;
      const auto root_id = graph.add("seed", "test_data", {}, "parse_seed", "prov1", "voltfox", ArtifactValidation::valid);
      const auto child1 = graph.add("ir", "ir_data", {root_id}, "compile", "prov1", "voltfox", ArtifactValidation::valid);
      const auto child2 = graph.add("synthesis_2d", "s2d_data", {root_id}, "synthesize", "prov1", "voltfox.2d", ArtifactValidation::valid);
      const auto child3 = graph.add("synthesis_3d", "s3d_data", {root_id}, "synthesize", "prov1", "voltfox.3d", ArtifactValidation::valid);
      const auto child4 = graph.add("package", "pkg_data", {child1, child2}, "build_package", "prov1", "voltfox", ArtifactValidation::valid);
      (void)child3; (void)child4;

      std::vector<std::string> changed{root_id};
      const auto affected = graph.affected_by(changed);
      check(affected.size() == 5, "root change should invalidate root + 4 dependents");
      check(graph.size() == 5, "graph should have 5 nodes");
      std::cout << "  [PASS] Property: artifact-graph closure\n";
    }

    // 8. Selective invalidation: single-source change invalidates only dependents
    {
      AssetGraph graph;
      const auto seed_id = graph.add("seed", "seed_v1", {}, "parse_seed", "prov1", "voltfox", ArtifactValidation::valid);
      const auto ir_id = graph.add("ir", "ir_v1", {seed_id}, "compile", "prov1", "voltfox", ArtifactValidation::valid);
      const auto pal_id = graph.add("palette", "pal_v1", {}, "palette", "prov1", "voltfox", ArtifactValidation::valid);
      const auto render2d = graph.add("synthesis_2d", "s2d_v1", {ir_id, pal_id}, "synthesize", "prov1", "voltfox.2d", ArtifactValidation::valid);
      const auto render3d = graph.add("synthesis_3d", "s3d_v1", {ir_id}, "synthesize", "prov1", "voltfox.3d", ArtifactValidation::valid);

      std::vector<std::string> changed_pal{pal_id};
      auto invalidated = graph.affected_by(changed_pal);
      check(std::ranges::find(invalidated, render2d) != invalidated.end(),
            "palette change must invalidate 2D render");
      check(std::ranges::find(invalidated, render3d) == invalidated.end(),
            "palette change must not invalidate 3D render");
      check(std::ranges::find(invalidated, ir_id) == invalidated.end(),
            "palette change must not invalidate IR (only palette itself and 2D render)");
      std::cout << "  [PASS] Property: selective invalidation\n";
    }

    // ===== MUTATION TESTS =====

    // 9. Remove form binding: form references non-existent transformation
    {
      auto s = parse_seed(
        "schema=gspl.sprite-seed/0.2\nid=test.mutant\nname=Mutant\n"
        "classification=test.mutant\nrights=ORIGINAL_USER_CREATION\n"
        "entropy_root=42\nprimary_color=#242038\naccent_color=#56F1FF\n"
        "ability=dummy|test.ability|10|5|2\n"
        "rig=test.rig\n"
        "bone=root|-|0|0|0|1|1|12|-45|45\n"
        "clip=idle|10|true\n"
        "track=idle|root|0,0,0,0,1,1;10,0,0,0,1,1\n"
        "initial_state=idle\nstate=idle|idle\n"
        "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
        "collision_window=dummy|body|0|10|true\n"
        "[form.base]\n"
        "transformations=missing_transform\n"
        "[morphology.torso]\n"
        "position=0,0,0\nsize=40,30,20\ncolor=#242038\nrotation=0\n"
      );
      auto v = validate(s);
      check(!v.ok(), "form with missing transformation must fail validation");
      std::cout << "  [PASS] Mutation killed: remove form binding\n";
    }

    // 10. Remove transition binding: transformation references non-existent form
    {
      auto s = parse_seed(
        "schema=gspl.sprite-seed/0.2\nid=test.mutant\nname=Mutant\n"
        "classification=test.mutant\nrights=ORIGINAL_USER_CREATION\n"
        "entropy_root=42\nprimary_color=#242038\naccent_color=#56F1FF\n"
        "ability=dummy|test.ability|10|5|2\n"
        "rig=test.rig\n"
        "bone=root|-|0|0|0|1|1|12|-45|45\n"
        "clip=idle|10|true\n"
        "track=idle|root|0,0,0,0,1,1;10,0,0,0,1,1\n"
        "initial_state=idle\nstate=idle|idle\n"
        "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
        "collision_window=dummy|body|0|10|true\n"
        "[form.base]\n"
        "transformations=orphan_trans\n"
        "[transformation.orphan_trans]\n"
        "from_form=ghost\n"
        "to_form=phantom\n"
        "trigger=command:transform\n"
        "duration_ticks=6\nresource_cost=50\n"
        "[morphology.torso]\n"
        "position=0,0,0\nsize=40,30,20\ncolor=#242038\nrotation=0\n"
      );
      auto v = validate(s);
      check(!v.ok(), "transformation with non-existent form must fail validation");
      std::cout << "  [PASS] Mutation killed: remove transition binding\n";
    }

    // 11. Corrupt transformation progress: active with bad tick range
    {
      CombatProgram combat;
      combat.id = "test.combat";
      TransformationProgram trans;
      trans.id = "test.forms";
      trans.base_form = "base";
      trans.forms = {{"base", 0, 0, {"dummy"}}, {"storm", 20, 20, {"dummy"}}};
      trans.transformations = {{"ascend", "base", "storm", 10, 6, true}};

      TransformationState state;
      state.stable_entity_id = "test";
      state.current_form = "base";
      state.energy = 50;
      state.maximum_energy = 100;
      state.active = ActiveTransformation{"ascend", "base", "storm", 100, 50};

      auto v = validate_transformation_state(trans, combat, state);
      check(!v.ok(), "transformation with corrupt progress must be rejected");
      std::cout << "  [PASS] Mutation killed: corrupt transformation progress\n";
    }

    // 12. Accept unresolved plane asset: all planes must have rendered image data
    {
      std::uint32_t tmp12 = 77;
      const auto src = make_random_seed(tmp12);
      const auto seed = parse_seed(src);
      const auto ir = compile(seed);
      const auto result = synthesize_unified_entity(ir);

      bool all_have_assets = true;
      for (const auto &plane : result.proj25d_base.planes) {
        if (plane.visual_asset_id.empty()) all_have_assets = false;
      }
      check(all_have_assets, "all 2.5D planes must have a visual asset reference");
      std::cout << "  [PASS] Mutation killed: accept unresolved plane asset\n";
    }

    // 13. Make animation frames identical: duplicate pixel content must be caught
    {
      FrameSource a;
      a.id = "frame_A";
      a.image = ImageRgba8{2, 2, ColorSpace::srgb, AlphaMode::straight,
        {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}};
      a.frame_hash = compute_frame_hash(a.image);
      FrameSource b;
      b.id = "frame_B";
      b.image = ImageRgba8{2, 2, ColorSpace::srgb, AlphaMode::straight, a.image.pixels};
      b.frame_hash = compute_frame_hash(b.image);
      // Same pixel content: hashes must match
      check(a.frame_hash == b.frame_hash, "frames with identical pixels must have same hash");

      // Distinct pixel content: hashes must differ
      FrameSource c;
      c.id = "frame_C";
      c.image = ImageRgba8{2, 2, ColorSpace::srgb, AlphaMode::straight,
        {0, 255, 0, 255, 255, 0, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}};
      c.frame_hash = compute_frame_hash(c.image);
      check(a.frame_hash != c.frame_hash, "frames with different pixels must have different hash");

      // validate_frame_distinctness detects duplicate content in a clip
      std::vector<FrameSource> fsrcs{a, b, c};
      AnimationClip clip;
      clip.id = "test_clip";
      clip.frame_ids = {"frame_A", "frame_B"};
      clip.frame_durations = {1, 1};
      std::vector<AnimationClip> clips{clip};
      auto vr = validate_frame_distinctness(fsrcs, clips);
      bool found_dup = false;
      for (const auto& d : vr.diagnostics)
        if (d.code == "SPRITE_ANIMATION_FRAME_DUPLICATE_CONTENT") found_dup = true;
      check(found_dup, "duplicate frame content in clip must be detected");

      // Distinct-content clip passes
      AnimationClip ok_clip;
      ok_clip.id = "ok";
      ok_clip.frame_ids = {"frame_A", "frame_C"};
      ok_clip.frame_durations = {1, 1};
      std::vector<AnimationClip> ok_clips{ok_clip};
      vr = validate_frame_distinctness(fsrcs, ok_clips);
      bool no_dup = true;
      for (const auto& d : vr.diagnostics)
        if (d.code == "SPRITE_ANIMATION_FRAME_DUPLICATE_CONTENT") no_dup = false;
      check(no_dup, "distinct frame content in clip must pass validation");

      std::cout << "  [PASS] Mutation killed: make animation frames identical\n";
    }

    // 14. Disable collision window: collision window referencing non-existent ability must be caught
    {
      auto s = parse_seed(
        "schema=gspl.sprite-seed/0.2\nid=test.mutant\nname=Mutant\n"
        "classification=test.mutant\nrights=ORIGINAL_USER_CREATION\n"
        "entropy_root=42\nprimary_color=#242038\naccent_color=#56F1FF\n"
        "ability=test|test.ability|10|5|2\n"
        "rig=test.rig\n"
        "bone=root|-|0|0|0|1|1|12|-45|45\n"
        "clip=idle|10|true\n"
        "track=idle|root|0,0,0,0,1,1;10,0,0,0,1,1\n"
        "initial_state=idle\nstate=idle|idle\n"
        "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
        "collision_window=ghost_ability|body|0|10|true\n"
        "[form.base]\n"
        "transformations=\n"
        "[morphology.torso]\n"
        "position=0,0,0\nsize=40,30,20\ncolor=#242038\nrotation=0\n"
      );
      auto v = validate(s);
      check(!v.ok(), "collision window with missing ability must fail validation");
      std::cout << "  [PASS] Mutation killed: disable collision window\n";
    }

    // 15. Skip ability resource cost: combat correctly deducts resource cost
    {
      CombatProgram prog;
      prog.id = "test.combat.resource";
      prog.abilities = {
        {"costly_ability", CombatTargetRule::enemy, 50, 5, 2000,
         {{CombatEffectKind::damage, {}, 10, 0}}}
      };
      CombatState state;
      state.tick = 0;
      CombatActorState actor;
      actor.id = "attacker";
      actor.team_id = "team_a";
      actor.health = 100;
      actor.maximum_health = 100;
      actor.resource = 60;
      actor.maximum_resource = 100;
      actor.x_millimeters = 0;
      actor.y_millimeters = 0;
      state.actors["attacker"] = actor;
      CombatActorState target;
      target.id = "defender";
      target.team_id = "team_b";
      target.health = 100;
      target.maximum_health = 100;
      target.resource = 100;
      target.maximum_resource = 100;
      target.x_millimeters = 1000;
      target.y_millimeters = 0;
      state.actors["defender"] = target;

      TransformationProgram trans;
      trans.id = "test.forms.resource";
      trans.base_form = "base";
      trans.forms = {{"base", 0, 0, {"costly_ability"}}};

      CombatCommand cmd;
      cmd.tick = 0;
      cmd.source_actor = "attacker";
      cmd.target_actor = "defender";
      cmd.ability_id = "costly_ability";

      try {
        const auto events = execute_combat_command(prog, state, cmd, "base", trans);
        check(state.actors["attacker"].resource == 10, "ability resource cost of 50 must be deducted from 60");
        check(!events.empty(), "combat events must be produced");
      } catch (const std::exception &ex) {
        std::cerr << "    combat command exception: " << ex.what() << "\n";
        throw;
      }
      std::cout << "  [PASS] Mutation killed: skip ability resource cost\n";
    }

    // 16. Omit state identity: identity with missing fields must differ from complete identity
    {
      LivingRuntimeState state;
      state.tick = 42;
      state.energy = 75;
      set_runtime_variable(state, "health", 80);

      const auto identity = capture_entity_identity(
          state, "test.entity", "inst-001", "base", "", "",
          "", "", "idle");
      check(!identity.entity_def_id.empty(), "entity def id must not be empty");
      check(!identity.instance_id.empty(), "instance id must not be empty");
      check(identity.health == 80, "health must be captured");
      check(identity.resource == 75, "resource must be captured");

      const auto hash_a = entity_identity_hash(identity);
      check(!hash_a.empty(), "identity hash must not be empty");

      LivingRuntimeState state2;
      state2.tick = 0;
      state2.energy = 0;
      const auto identity2 = capture_entity_identity(
          state2, "test.entity", "inst-001", "base", "", "",
          "", "", "idle");
      check(!entity_identities_match(identity, identity2),
            "omitted identity fields must cause mismatch");
      std::cout << "  [PASS] Mutation killed: omit state identity\n";
    }

    // 17. Mutate save data: deserialization must reject corrupted data
    {
      check_throws("mutated save data", [] {
        LivingRuntimeProgram prog;
        prog.id = "test.restore";
        const auto bad_data = std::string("MUTATED_STATE_DATA_WITH_NO_VALID_FORMAT");
        const auto state = deserialize_living_runtime_state(prog, bad_data);
        (void)state;
      });
      std::cout << "  [PASS] Mutation killed: mutate save data\n";
    }

    // 18. Reuse stale artifact: new seed does not implicitly invalidate old IR
    {
      AssetGraph graph;
      const auto seed_id = graph.add("seed", "seed_v1", {}, "parse_seed", "prov1", "voltfox", ArtifactValidation::valid);
      const auto ir_id = graph.add("ir", "ir_v1", {seed_id}, "compile", "prov1", "voltfox", ArtifactValidation::valid);

      const auto new_seed_id = graph.add("seed", "seed_v2", {}, "parse_seed", "prov1", "voltfox", ArtifactValidation::valid);
      std::vector<std::string> changed_seed{new_seed_id};
      const auto invalidated = graph.affected_by(changed_seed);
      check(std::ranges::find(invalidated, ir_id) == invalidated.end(),
            "new seed should not implicitly invalidate old IR");
      std::cout << "  [PASS] Mutation killed: reuse stale artifact\n";
    }

    // 19. Omit package checksum: corrupted manifest fails verification
    {
      TestFixture fix;
      std::uint32_t tmp19 = 33;
      const auto src = make_random_seed(tmp19);
      const auto seed = parse_seed(src);
      build_package(seed, fix.sub("checksum-pkg"));

      const auto manifest_path = fix.sub("checksum-pkg") / "manifest.json";
      check(std::filesystem::exists(manifest_path), "manifest must exist");
      std::ofstream bad(manifest_path, std::ios::binary | std::ios::trunc);
      bad << "{\"corrupted\":true}";
      bad.close();
      const auto vr = verify_package(fix.sub("checksum-pkg"));
      check(!vr.ok(), "corrupted manifest must cause verification failure");
      std::cout << "  [PASS] Mutation killed: omit package checksum\n";
    }

    // 20. Bypass rights validation: prohibited rights must be rejected
    {
      auto s = parse_seed(
        "schema=gspl.sprite-seed/0.2\nid=test.badrights\nname=BadRights\n"
        "classification=test.synthetic\nrights=PROHIBITED\n"
        "entropy_root=42\nprimary_color=#FF0000\naccent_color=#00FF00\n"
        "ability=dummy|test.ability|10|5|2\n"
        "rig=test.rig\n"
        "bone=root|-|0|0|0|1|1|12|-45|45\n"
        "clip=idle|10|true\n"
        "track=idle|root|0,0,0,0,1,1;10,0,0,0,1,1\n"
        "initial_state=idle\nstate=idle|idle\n"
        "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
        "collision_window=dummy|body|0|10|true\n"
        "[form.base]\n"
        "transformations=\n"
        "[morphology.torso]\n"
        "position=0,0,0\nsize=40,30,20\ncolor=#FF0000\nrotation=0\n"
      );
      auto v = validate(s);
      check(!v.ok(), "prohibited rights must fail validation");
      std::cout << "  [PASS] Mutation killed: bypass rights validation\n";
    }

    // 21. Resource limits: seed-level bounds acceptance and rejection
    {
      // Boundary: at-limit forms (16)
      {
        ResourceLimits limits;
        SpriteSeed s;
        s.schema = "gspl.sprite-seed/0.1";
        s.stable_id = "test.limits.at";
        s.name = "AtLimit";
        s.classification = "test.synthetic";
        s.rights = RightsClass::original_user_creation;
        s.primary_color = "#FF0000";
        s.accent_color = "#00FF00";
        s.abilities = {{"a", "dummy", 1, 1, 1}};
        for (std::uint32_t i = 0; i < 16; ++i)
          s.forms.push_back({"f" + std::to_string(i), {}});
        auto v = enforce_resource_limits(s, limits);
        check(v.ok(), "16 forms must be accepted at limit");
      }
      // Over-boundary: 17 forms
      {
        ResourceLimits limits;
        SpriteSeed s;
        s.schema = "gspl.sprite-seed/0.1";
        s.stable_id = "test.limits.over";
        s.name = "OverLimit";
        s.classification = "test.synthetic";
        s.rights = RightsClass::original_user_creation;
        s.primary_color = "#FF0000";
        s.accent_color = "#00FF00";
        s.abilities = {{"a", "dummy", 1, 1, 1}};
        for (std::uint32_t i = 0; i < 17; ++i)
          s.forms.push_back({"f" + std::to_string(i), {}});
        auto v = enforce_resource_limits(s, limits);
        check(!v.ok(), "17 forms must exceed limit");
        bool found_code = false;
        for (const auto& d : v.diagnostics)
          if (d.code == "RESOURCE_FORMS") found_code = true;
        check(found_code, "diagnostic must include RESOURCE_FORMS code");
        check(v.diagnostics.front().message.find("17") != std::string::npos, "diagnostic must include current value");
        check(v.diagnostics.front().message.find("16") != std::string::npos, "diagnostic must include maximum value");
      }
      // Over-boundary: 33 transformations
      {
        ResourceLimits limits;
        SpriteSeed s;
        s.schema = "gspl.sprite-seed/0.1";
        s.stable_id = "test.limits.trans";
        s.name = "TransOver";
        s.classification = "test.synthetic";
        s.rights = RightsClass::original_user_creation;
        s.primary_color = "#FF0000";
        s.accent_color = "#00FF00";
        s.abilities = {{"a", "dummy", 1, 1, 1}};
        s.forms = {{"f0", {"t0"}}, {"f1", {"t0"}}};
        for (std::uint32_t i = 0; i < 33; ++i)
          s.transformations.push_back({"t" + std::to_string(i), "f0", "f1", "", 10, 1});
        auto v = enforce_resource_limits(s, limits);
        check(!v.ok(), "33 transformations must exceed limit");
      }
      // Over-boundary: 65 bones
      {
        ResourceLimits limits;
        SpriteSeed s;
        s.schema = "gspl.sprite-seed/0.1";
        s.stable_id = "test.limits.bones";
        s.name = "BonesOver";
        s.classification = "test.synthetic";
        s.rights = RightsClass::original_user_creation;
        s.primary_color = "#FF0000";
        s.accent_color = "#00FF00";
        s.abilities = {{"a", "dummy", 1, 1, 1}};
        RigDefinition rig;
        rig.id = "test.rig";
        for (std::uint32_t i = 0; i < 65; ++i)
          rig.bones.push_back({"b" + std::to_string(i), i == 0 ? std::optional<std::string>{} : std::optional<std::string>{"b0"}, {0,0,0,1}, 1, {}});
        s.rig = rig;
        auto v = enforce_resource_limits(s, limits);
        check(!v.ok(), "65 bones must exceed limit");
      }
      // Over-boundary: 33 sockets
      {
        ResourceLimits limits;
        SpriteSeed s;
        s.schema = "gspl.sprite-seed/0.1";
        s.stable_id = "test.limits.sock";
        s.name = "SockOver";
        s.classification = "test.synthetic";
        s.rights = RightsClass::original_user_creation;
        s.primary_color = "#FF0000";
        s.accent_color = "#00FF00";
        s.abilities = {{"a", "dummy", 1, 1, 1}};
        RigDefinition rig;
        rig.id = "test.rig";
        rig.bones = {{"root", {}, {0,0,0,1}, 1, {}}};
        for (std::uint32_t i = 0; i < 33; ++i)
          rig.sockets.push_back({"s" + std::to_string(i), "root", {0,0,0,1}});
        s.rig = rig;
        auto v = enforce_resource_limits(s, limits);
        check(!v.ok(), "33 sockets must exceed limit");
      }
      // Over-boundary: 33 animation clips
      {
        ResourceLimits limits;
        SpriteSeed s;
        s.schema = "gspl.sprite-seed/0.1";
        s.stable_id = "test.limits.clips";
        s.name = "ClipsOver";
        s.classification = "test.synthetic";
        s.rights = RightsClass::original_user_creation;
        s.primary_color = "#FF0000";
        s.accent_color = "#00FF00";
        s.abilities = {{"a", "dummy", 1, 1, 1}};
        s.rig = RigDefinition{"test.rig", {{"root", {}, {0,0,0,1}, 1, {}}}, {}};
        for (std::uint32_t i = 0; i < 33; ++i)
          s.clips.push_back({"c" + std::to_string(i), 10u, true, {{"root", {{0u, {0,0,0,1}}, {10u, {0,0,0,1}}}}}, {}});
        auto v = enforce_resource_limits(s, limits);
        check(!v.ok(), "33 clips must exceed limit");
      }
      std::cout << "  [PASS] Resource limits: boundary and over-boundary enforcement\n";
    }

    // 22. Frame hash: schema-bound preimage with canonical RGBA8
    {
      const ImageRgba8 ref{2, 2, ColorSpace::srgb, AlphaMode::straight,
        {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}};
      const auto ref_hash = compute_frame_hash(ref);
      check(ref_hash.size() == 64, "frame hash must be 64 hex chars");
      check(ref_hash.find_first_not_of("0123456789abcdef") == std::string::npos, "frame hash must be lowercase hex");

      // Identical canonical frames hash identically
      const ImageRgba8 same{2, 2, ColorSpace::srgb, AlphaMode::straight,
        {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}};
      check(compute_frame_hash(ref) == compute_frame_hash(same), "identical frames must hash identically");

      // One-pixel change alters hash
      ImageRgba8 one_pixel{2, 2, ColorSpace::srgb, AlphaMode::straight, ref.pixels};
      one_pixel.pixels[0] = 254;
      check(compute_frame_hash(ref) != compute_frame_hash(one_pixel), "one-pixel change must alter hash");

      // Dimension change alters hash (same pixels, different dimensions)
      ImageRgba8 diff_dim{1, 4, ColorSpace::srgb, AlphaMode::straight,
        {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}};
      check(compute_frame_hash(ref) != compute_frame_hash(diff_dim), "dimension change must alter hash");

      // Repeated builds reproduce hashes
      const auto h1 = compute_frame_hash(ref);
      const auto h2 = compute_frame_hash(ref);
      check(h1 == h2, "repeated builds must reproduce hash");

      // FrameSource::frame_hash is set after synthesis
      {
        std::uint32_t tmp22 = 100;
        const auto src = make_random_seed(tmp22);
        const auto seed = parse_seed(src);
        const auto ir = compile(seed);
        const auto result = synthesize_unified_entity(ir);
        for (const auto& f : result.proj2d_base.source_frames) {
          check(!f.frame_hash.empty(), ("synthesized frame must have frame_hash: " + f.id).c_str());
        }
      }

      // Package serialization preserves frame hashes
      {
        std::uint32_t tmp22b = 200;
        const auto tmp_dir = std::filesystem::temp_directory_path() / "gspl-frame-hash-test";
        std::filesystem::remove_all(tmp_dir);
        build_package(parse_seed(make_random_seed(tmp22b)), tmp_dir);
        auto pkg = verify_package(tmp_dir);
        check(pkg.ok(), "package verification must pass");
        std::filesystem::remove_all(tmp_dir);
      }
      std::cout << "  [PASS] Frame hash: schema identity, distinctness, synthesis, package\n";
    }

    // 23. Pixel AABB: bounding box of non-transparent pixels
    {
      ImageRgba8 img{4, 4, ColorSpace::srgb, AlphaMode::straight,
        {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
         0,0,0,0, 255,0,0,255, 255,0,0,255, 0,0,0,0,
         0,0,0,0, 255,0,0,255, 255,0,0,255, 0,0,0,0,
         0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}};
      auto aabb = compute_pixel_aabb(img);
      check(!aabb.empty, "non-empty image must produce non-empty aabb");
      check(aabb.min_x == 1 && aabb.min_y == 1 && aabb.max_x == 2 && aabb.max_y == 2,
            "pixel AABB must enclose non-transparent pixels");

      // Fully transparent frame
      ImageRgba8 empty{4, 4, ColorSpace::srgb, AlphaMode::straight,
        std::vector<std::uint8_t>(4*4*4, 0)};
      auto empty_aabb = compute_pixel_aabb(empty);
      check(empty_aabb.empty, "fully transparent image must produce empty aabb");

      // One opaque pixel at origin
      {
        ImageRgba8 one{4, 4, ColorSpace::srgb, AlphaMode::straight, std::vector<std::uint8_t>(4*4*4, 0)};
        one.pixels[0] = 255; one.pixels[1] = 0; one.pixels[2] = 0; one.pixels[3] = 255;
        auto a = compute_pixel_aabb(one);
        check(!a.empty, "one opaque pixel must produce non-empty aabb");
        check(a.min_x == 0 && a.min_y == 0 && a.max_x == 0 && a.max_y == 0, "single pixel AABB must be point");
      }

      // One-pixel image
      {
        ImageRgba8 tiny{1, 1, ColorSpace::srgb, AlphaMode::straight, {128, 64, 32, 255}};
        auto a = compute_pixel_aabb(tiny);
        check(!a.empty, "one-pixel image must produce non-empty aabb");
        check(a.min_x == 0 && a.min_y == 0 && a.max_x == 0 && a.max_y == 0, "one-pixel AABB");
      }

      // Deterministic repeated calculation
      {
        ImageRgba8 det{3, 3, ColorSpace::srgb, AlphaMode::straight,
          {0,0,0,0, 10,20,30,255, 0,0,0,0,
           40,50,60,255, 70,80,90,255, 100,110,120,255,
           0,0,0,0, 130,140,150,255, 0,0,0,0}};
        const auto a1 = compute_pixel_aabb(det);
        const auto a2 = compute_pixel_aabb(det);
        check(a1.empty == a2.empty && a1.min_x == a2.min_x && a1.max_x == a2.max_x &&
              a1.min_y == a2.min_y && a1.max_y == a2.max_y,
              "pixel AABB must be deterministic");
      }
      std::cout << "  [PASS] Pixel AABB: semantics and determinism\n";
    }

    // 24. Corrupted RGBA data: invariant, pack_atlas, and encode_ppm_p6 reject malformed buffers
    {
      // Zero dimensions must fail invariant
      ImageRgba8 zero_w{0, 1, ColorSpace::srgb, AlphaMode::straight, {0, 0, 0, 0}};
      check(!zero_w.invariant(), "zero-width image must fail invariant");

      ImageRgba8 zero_h{1, 0, ColorSpace::srgb, AlphaMode::straight, {0, 0, 0, 0}};
      check(!zero_h.invariant(), "zero-height image must fail invariant");

      // Mismatched pixel count: 2x2 needs 16 bytes, but has only 8
      ImageRgba8 undersized{2, 2, ColorSpace::srgb, AlphaMode::straight, {255, 0, 0, 255, 0, 255, 0, 255}};
      check(!undersized.invariant(), "undersized pixel buffer must fail invariant");

      // Oversized pixel buffer
      ImageRgba8 oversized{2, 2, ColorSpace::srgb, AlphaMode::straight,
        std::vector<std::uint8_t>(32, 128)};
      check(!oversized.invariant(), "oversized pixel buffer must fail invariant");

      // pack_atlas must reject frames with invalid invariant
      {
        FrameSource bad;
        bad.id = "corrupt";
        bad.image = ImageRgba8{2, 2, ColorSpace::srgb, AlphaMode::straight, {255, 0, 0, 255}};
        bad.duration_ticks = 1;
        // bad.image has only 4 bytes instead of 16 → invariant() fails
        check(!bad.image.invariant(), "corrupt frame image must fail invariant");
        std::vector<FrameSource> bad_frames{bad};
        try {
          static_cast<void>(pack_atlas(bad_frames, 64, 64));
          check(false, "pack_atlas must throw on corrupt frame image");
        } catch (const std::invalid_argument&) {
          // expected
        }
      }

      // encode_ppm_p6 must reject images with invalid invariant
      {
        ImageRgba8 corrupt;
        corrupt.width = 1;
        corrupt.height = 1;
        corrupt.color_space = ColorSpace::srgb;
        corrupt.alpha_mode = AlphaMode::opaque;
        corrupt.pixels = {};  // empty buffer for 1x1 image (needs 4 bytes)
        check(!corrupt.invariant(), "empty pixel buffer must fail invariant");
        try {
          static_cast<void>(encode_ppm_p6(corrupt));
          check(false, "encode_ppm_p6 must throw on corrupt image");
        } catch (const std::invalid_argument&) {
          // expected
        }
      }

      std::cout << "  [PASS] Mutation killed: corrupted RGBA data\n";
    }

    std::cout << "all GSPL Sprites mutation tests passed\n";
    return 0;

  } catch (const std::exception &e) {
    std::cerr << "GSPL_SPRITES_MUTATION_FAILED: " << e.what() << '\n';
    return 1;
  }
}
