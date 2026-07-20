#include "gspl_sprites/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
  try {
    check(sha256("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA-256 empty vector failed");
    check(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 abc vector failed");
    const std::string text = R"(schema=gspl.sprite-seed/0.1
id=original.test
name=Test
classification=fictional
rights=ORIGINAL_USER_CREATION
entropy_root=7
primary_color=#112233
accent_color=#AABBCC
ability=arc|electric.projectile|20|4|2
rig=test.rig
bone=root|-|0|0|0|1|1|10|-45|45
bone=head|root|4|0|0|1|1|6|-30|30
socket=muzzle|head|6|0|0|1|1
clip=idle|4|true
track=idle|head|0,4,0,-10,1,1;4,4,0,10,1,1
clip=attack|2|false
track=attack|head|0,4,0,0,1,1;2,6,0,20,1,1
clip_event=attack|release|1
initial_state=idle
state=idle|idle
state=attack|attack
transition=idle|attack|attack|GREATER_EQUAL|1|0|1|10
transition=attack|idle|attack|LESS|1|2|1|10
collision=bolt|CIRCLE|muzzle|2|0|1|1
collision_window=arc|bolt|0|2|true
)";
    const auto seed = parse_seed(text);
    check(validate(seed).ok(), "valid seed rejected");
    check(canonicalize(seed) == canonicalize(parse_seed(text)), "canonicalization not deterministic");
    auto changed_motion=seed; changed_motion.clips[1].tracks[0].keys[1].transform.rotation_degrees=21; check(sha256(canonicalize(changed_motion))!=sha256(canonicalize(seed)), "motion change did not affect canonical identity");
    const auto ir = compile(seed); check(ir.seed_identity.size() == 64, "identity is not SHA-256"); check(ir.rig.has_value() && ir.clips.size()==2 && ir.collision_windows.size()==1, "animation domains were not lowered");
    RuntimeEntity runtime; check(activate(runtime, seed.abilities[0]), "ability did not activate"); check(runtime.energy == 80, "ability cost not applied"); check(!activate(runtime, seed.abilities[0]), "concurrent ability activation allowed"); tick(runtime); tick(runtime); tick(runtime); check(runtime.state == RuntimeState::idle, "runtime failed to recover");
    auto prohibited = seed; prohibited.rights = RightsClass::prohibited; check(!validate(prohibited).ok(), "prohibited rights exported");
    auto missing_ability=seed; missing_ability.collision_windows[0].ability_id="absent"; check(!validate(missing_ability).ok(), "collision window with absent ability accepted");
    auto cycle=seed; cycle.rig->bones[0].parent_id="head"; check(!validate(cycle).ok(), "cyclic authored rig accepted");
    bool duplicate_rejected = false; try { (void)parse_seed(text + "name=Again\n"); } catch (const std::runtime_error&) { duplicate_rejected = true; } check(duplicate_rejected, "duplicate field accepted");
    bool oversized_rejected = false; try { (void)parse_seed(std::string((1U << 20) + 1, 'x')); } catch (const std::runtime_error&) { oversized_rejected = true; } check(oversized_rejected, "oversized source accepted");
    bool unresolved_rejected = false; try { (void)parse_seed("schema=gspl.sprite-seed/0.1\ntrack=missing|root|0,0,0,0,1,1\n"); } catch (const std::runtime_error&) { unresolved_rejected = true; } check(unresolved_rejected, "track before owning clip accepted");
    const auto output = std::filesystem::temp_directory_path() / "gspl-sprites-core-test"; std::filesystem::remove_all(output); std::filesystem::remove_all(output.string() + ".staging"); build_package(seed, output); check(std::filesystem::exists(output / "manifest.json"), "manifest missing"); check(std::filesystem::exists(output / "assets" / "entity.svg"), "projection missing"); check(std::filesystem::exists(output / "asset-graph.json"), "asset graph missing"); check(std::filesystem::exists(output / "provenance.json"), "provenance missing"); check(std::filesystem::exists(output / "rights.json"), "rights decision missing"); check(std::filesystem::exists(output / "rig.json"), "rig artifact missing"); check(std::filesystem::exists(output / "animations.json"), "animation artifact missing"); check(std::filesystem::exists(output / "animation-state-graph.json"), "state graph artifact missing");     check(std::filesystem::exists(output / "collisions.json"), "collision artifact missing");
    bool overwrite_rejected = false; try { build_package(seed, output); } catch (const std::runtime_error&) { overwrite_rejected = true; } check(overwrite_rejected, "existing package overwritten"); std::filesystem::remove_all(output);
    const std::string form_seed_text = R"(schema=gspl.sprite-seed/0.1
id=original.voltfox
name=Voltfox
classification=biological.fictional.electric-fox
rights=ORIGINAL_USER_CREATION
entropy_root=11072026
primary_color=#242038
accent_color=#56F1FF
ability=directional-lightning|electric.projectile.directional|25|8|2

[form.Idle]
transformations=IdleToRunning,IdleToAttack

[form.Running]
transformations=RunningToIdle

[form.Attack]
transformations=AttackToIdle

[transformation.IdleToRunning]
from_form=Idle
to_form=Running
trigger=perception:moving

[transformation.IdleToAttack]
from_form=Idle
to_form=Attack
trigger=combat:threat_detected

[transformation.RunningToIdle]
from_form=Running
to_form=Idle
trigger=perception:stopped

[transformation.AttackToIdle]
from_form=Attack
to_form=Idle
trigger=combat:target_defeated

[morphology.torso]
position=0,0,0
size=40,30,20
color=#242038
rotation=0

[morphology.head]
position=0,25,5
size=24,20,18
color=#242038
rotation=0

[morphology.left_ear]
position=-8,33,10
size=8,16,8
color=#56F1FF
rotation=-10

[morphology.right_ear]
position=8,33,10
size=8,16,8
color=#56F1FF
rotation=10

[morphology.left_eye]
position=-5,27,15
size=4,4,4
color=#56F1FF
rotation=0

[morphology.right_eye]
position=5,27,15
size=4,4,4
color=#56F1FF
rotation=0

[morphology.muzzle]
position=0,22,18
size=10,8,6
color=#FFFFFF
rotation=0

[morphology.tail]
position=-20,-5,0
size=6,30,6
color=#242038
rotation=-15

[morphology.left_front_leg]
position=-10,-20,-5
size=8,20,8
color=#1a1828
rotation=0

[morphology.right_front_leg]
position=10,-20,-5
size=8,20,8
color=#1a1828
rotation=0

[morphology.aura]
position=0,0,0
size=60,50,40
color=#56F1FF
rotation=0

[runtime]
aggression=60
curiosity=80
energy=70
loyalty=50
animation_intents=idle:idle,running:idle,attack:attack
)";
    {
      const auto form_seed = parse_seed(form_seed_text);
      check(validate(form_seed).ok(), "valid form seed rejected");
      check(form_seed.forms.size() == 3, "expected 3 forms");
      check(form_seed.transformations.size() == 4, "expected 4 transformations");
      check(form_seed.morphology.size() >= 11, "expected at least 11 morphology parts");
      check(form_seed.runtime.has_value(), "expected runtime attributes");
      check(form_seed.runtime->aggression == 60, "runtime aggression mismatch");
      check(form_seed.runtime->animation_intents.size() == 3, "expected 3 animation intents");
      const auto ir2 = compile(form_seed);
      check(ir2.form_definitions.size() == 3, "expected 3 compiled form definitions");
      check(ir2.transformation_deltas.size() == 4, "expected 4 compiled transformation deltas");
      check(ir2.morphology.size() >= 11, "expected 11+ compiled morphology parts");
      check(ir2.animation_intents.size() == 3, "expected 3 compiled animation intents");
    }
    {
      const std::string dangling = R"(schema=gspl.sprite-seed/0.1
id=test
name=Test
classification=test
rights=ORIGINAL_USER_CREATION
entropy_root=1
primary_color=#111111
accent_color=#222222
ability=x|test|1|1|1

[form.A]
transformations=AtoB

[transformation.BtoA]
from_form=B
to_form=A
trigger=test
)";
      bool caught = false;
      try { const auto bad_seed = parse_seed(dangling); (void)validate(bad_seed); const auto bad_ir = compile(bad_seed); (void)bad_ir; } catch (const std::runtime_error&) { caught = true; }
      check(caught, "dangling transformation reference not caught");
    }
    {
      const std::string form_seed_text2 = R"(schema=gspl.sprite-seed/0.1
id=test
name=Test
classification=test
rights=ORIGINAL_USER_CREATION
entropy_root=1
primary_color=#111111
accent_color=#222222
ability=x|test|1|1|1

[form.A]
transformations=AtoB

[form.B]
transformations=

[transformation.AtoB]
from_form=A
to_form=B
trigger=test

[morphology.torso]
position=0,0,0
size=1,1,1
color=#000000
rotation=0

[morphology.head]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.left_ear]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.right_ear]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.left_eye]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.right_eye]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.muzzle]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.tail]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.left_front_leg]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.right_front_leg]
position=0,1,0
size=1,1,1
color=#000000
rotation=0

[morphology.aura]
position=0,0,0
size=1,1,1
color=#000000
rotation=0
)";
      const auto seed2 = parse_seed(form_seed_text2);
      const auto ir2 = compile(seed2);
      check(ir2.form_definitions.size() == 2, "expected 2 forms in isolated form test");
      check(ir2.transformation_deltas.size() == 1, "expected 1 transformation delta");
      check(ir2.morphology.size() >= 11, "expected 11+ morphology parts");
    }
    std::cout << "all gspl sprites core tests passed\n"; return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
