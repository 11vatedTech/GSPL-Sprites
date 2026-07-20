#include "gspl_sprites/core.hpp"
#include "gspl_sprites/synthesis.hpp"
#include "gspl_sprites/animation.hpp"

#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace gspl::sprites;
namespace { void check(bool v, const char* m) { if (!v) throw std::runtime_error(m); } }

std::string voltfox_seed() {
  return "schema=gspl.sprite-seed/0.2\n"
    "id=original.voltfox\n"
    "name=Voltfox\n"
    "classification=biological.fictional.electric-fox\n"
    "rights=ORIGINAL_USER_CREATION\n"
    "entropy_root=11072026\n"
    "primary_color=#242038\n"
    "accent_color=#56F1FF\n"
    "ability=directional-lightning|electric.projectile.directional|25|8|2\n"
    "rig=voltfox.rig\n"
    "bone=root|-|0|0|0|1|1|12|-45|45\n"
    "bone=head|root|4|0|0|1|1|8|-30|30\n"
    "bone=tail|root|-4|0|0|1|1|10|-80|80\n"
    "socket=muzzle|head|8|0|0|1|1\n"
    "clip=idle|10|true\n"
    "track=idle|head|0,4,0,-10,1,1;10,4,0,10,1,1\n"
    "clip_event=idle|blink|5\n"
    "clip=attack|2|false\n"
    "track=attack|head|0,4,0,0,1,1;2,6,0,25,1,1\n"
    "clip_event=attack|release|1\n"
    "initial_state=idle\n"
    "state=idle|idle\n"
    "state=attack|attack\n"
    "transition=idle|attack|attack|GREATER_EQUAL|1|0|1|10\n"
    "transition=attack|idle|attack|LESS|1|2|1|10\n"
    "collision=body|AXIS_ALIGNED_BOX|root|0|0|8|5\n"
    "collision=bolt|CIRCLE|muzzle|3|0|2|2\n"
    "collision_window=directional-lightning|bolt|0|2|true\n"
    "[form.base]\n"
    "transformations=ascend\n"
    "[form.storm]\n"
    "transformations=descend\n"
    "resource_capacity=150\n"
    "collision_scale=1.3\n"
    "ability_envelope=1.5\n"
    "max_health=150\n"
    "[transformation.ascend]\n"
    "from_form=base\n"
    "to_form=storm\n"
    "trigger=command:transform\n"
    "duration_ticks=6\n"
    "resource_cost=50\n"
    "[transformation.descend]\n"
    "from_form=storm\n"
    "to_form=base\n"
    "trigger=command:transform\n"
    "duration_ticks=6\n"
    "resource_cost=30\n"
    "[morphology.torso]\n"
    "position=0,0,0\nsize=40,30,20\ncolor=#242038\nrotation=0\n"
    "[morphology.head]\n"
    "position=0,25,5\nsize=24,20,18\ncolor=#242038\nrotation=0\n"
    "[morphology.left_ear]\n"
    "position=-8,33,10\nsize=8,16,8\ncolor=#56F1FF\nrotation=-10\n"
    "[morphology.right_ear]\n"
    "position=8,33,10\nsize=8,16,8\ncolor=#56F1FF\nrotation=10\n"
    "[morphology.left_eye]\n"
    "position=-5,27,15\nsize=4,4,4\ncolor=#56F1FF\nrotation=0\n"
    "[morphology.right_eye]\n"
    "position=5,27,15\nsize=4,4,4\ncolor=#56F1FF\nrotation=0\n"
    "[morphology.muzzle]\n"
    "position=0,22,18\nsize=10,8,6\ncolor=#FFFFFF\nrotation=0\n"
    "[morphology.tail]\n"
    "position=-20,-5,0\nsize=6,30,6\ncolor=#242038\nrotation=-15\n"
    "[morphology.left_front_leg]\n"
    "position=-10,-20,-5\nsize=8,20,8\ncolor=#1a1828\nrotation=0\n"
    "[morphology.right_front_leg]\n"
    "position=10,-20,-5\nsize=8,20,8\ncolor=#1a1828\nrotation=0\n"
    "[morphology.aura]\n"
    "position=0,0,0\nsize=60,50,40\ncolor=#56F1FF\nrotation=0\n"
    "[runtime]\n"
    "aggression=60\ncuriosity=80\nenergy=70\nloyalty=50\n"
    "animation_intents=idle:idle,ascend:ascend,storm_idle:storm_idle,storm_attack:storm_attack,descend:descend,attack:attack\n";
}

int main() {
  try {
    const auto seed = parse_seed(voltfox_seed());
    const auto v = validate(seed); check(v.ok(), "seed validation failed");
    const auto ir = compile(seed);

    const auto pal = make_palette(ir.primary_color, ir.accent_color);
    const auto rig = ir.rig.value_or(make_biped_rig(ir.entity_id));
    const auto result = synthesize_unified_entity(ir);

    check(!result.proj2d_base.source_frames.empty(), "no 2D base frames");
    std::set<std::string> frame_hashes;
    std::set<std::string> frame_ids_set;
    for (const auto& f : result.proj2d_base.source_frames) {
      check(frame_ids_set.insert(f.id).second, ("duplicate frame id: " + f.id).c_str());
      const auto hash = sha256(std::string_view(reinterpret_cast<const char*>(f.image.pixels.data()), f.image.pixels.size()));
      frame_hashes.insert(hash);
    }
    check(!frame_ids_set.empty(), "no frame ids");
    if (!result.proj2d_transformed.source_frames.empty()) {
      std::set<std::string> storm_hashes;
      for (const auto& f : result.proj2d_transformed.source_frames) {
        const auto hash = sha256(std::string_view(reinterpret_cast<const char*>(f.image.pixels.data()), f.image.pixels.size()));
        storm_hashes.insert(hash);
      }
      size_t shared = 0;
      for (const auto& bh : frame_hashes) { if (storm_hashes.count(bh)) ++shared; }
      check(shared < frame_hashes.size(), "storm frames should not be identical to all base frames");
    }

    if (!result.proj2d_base.animations.empty()) {
      std::set<std::string> clip_frame_ids;
      for (const auto& clip : result.proj2d_base.animations) {
        for (const auto& fid : clip.frame_ids) {
          check(clip_frame_ids.insert(fid).second, ("duplicate frame id in clip " + clip.id).c_str());
        }
      }
    }

    if (!ir.form_morphology_overrides.empty()) {
      auto storm_morph = ir.morphology;
      for (const auto& [part_name, override_part] : ir.form_morphology_overrides.begin()->second) {
        storm_morph[part_name] = override_part;
      }
      SynthesisPalette storm_pal = pal;
      const auto storm_2d = synthesize_projection2d_voltfox(ir.entity_id, "storm", storm_pal, storm_morph, rig);
      std::set<std::string> storm_frame_hashes;
      for (const auto& f : storm_2d.source_frames) {
        const auto hash = sha256(std::string_view(reinterpret_cast<const char*>(f.image.pixels.data()), f.image.pixels.size()));
        storm_frame_hashes.insert(hash);
      }
      for (const auto& bh : frame_hashes) {
        check(storm_frame_hashes.count(bh) == 0, "storm frame should differ from base frame");
      }
    }

    for (const auto& f : result.proj2d_base.collision_shapes) {
      check(!f.id.empty(), "collision shape has empty id");
    }

    for (const auto& w : result.proj2d_base.collision_windows) {
      check(!w.shape_id.empty(), "collision window has empty shape");
    }

    check(!result.proj2d_transformed.source_frames.empty(), "no 2D transformed frames");

    std::cout << "all GSPL Sprites frame distinctness tests passed" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << std::endl;
    return 1;
  }
}
