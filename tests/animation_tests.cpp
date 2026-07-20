#include "gspl_sprites/animation.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
  try {
    const RigDefinition rig{"voltfox.rig",{{"root",std::nullopt,{},12,{-45,45}},{"head","root",{4,0,0,1,1},8,{-30,30}},{"tail","root",{-4,0,0,1,1},10,{-80,80}}},{{"muzzle","head",{8,0,0,1,1}}}};
    check(validate_rig(rig).ok(), "valid rig rejected");
    auto cyclic = rig; cyclic.bones[0].parent_id="tail"; check(!validate_rig(cyclic).ok(), "cyclic rig accepted");
    const std::array vertices{SkinnedVertex{0,0,{{"root",1.0}}},SkinnedVertex{4,0,{{"root",0.25},{"head",0.75}}}}; check(validate_skin(vertices,rig).ok(), "valid skin rejected");
    auto bad_vertices=vertices; bad_vertices[1].influences[1].weight=0.5; check(!validate_skin(bad_vertices,rig).ok(), "invalid weight sum accepted");

    const SkeletalClip idle{"idle",10,true,{{"head",{{0,{4,0,350,1,1}},{10,{4,0,10,1,1}}}}},{{"blink",5}}};
    const SkeletalClip attack{"attack",6,false,{{"head",{{0,{4,0,0,1,1}},{6,{6,0,25,1,1}}}}},{{"release",3}}};
    check(validate_skeletal_clip(idle,rig).ok() && validate_skeletal_clip(attack,rig).ok(), "valid clip rejected");
    const auto middle=sample_track(idle.tracks[0],5); check(std::abs(middle.rotation_degrees-360.0)<1e-9, "angle did not interpolate shortest path");
    auto bad_clip=idle; bad_clip.tracks[0].keys[1].tick=0; check(!validate_skeletal_clip(bad_clip,rig).ok(), "unordered keys accepted");

    const AnimationStateGraph graph{"idle",{{"idle","idle",{{"attack","attack",Comparison::greater_equal,1,0,1,10}}},{"attack","attack",{{"idle","attack",Comparison::less,1,6,1,10}}}}};
    const std::array clips{idle,attack}; check(validate_state_graph(graph,clips).ok(), "valid state graph rejected");
    check(select_transition(graph.states[0],0,"attack",1).value_or("")=="attack", "transition selection failed");
    auto unreachable=graph; unreachable.states.push_back({"orphan","idle",{}}); check(!validate_state_graph(unreachable,clips).ok(), "unreachable state accepted");

    const std::array shapes{CollisionShape{"body",CollisionKind::axis_aligned_box,"root",0,0,8,5},CollisionShape{"bolt",CollisionKind::circle,"muzzle",3,0,2,2}};
    const std::array windows{CollisionWindow{"body",0,6,false,{}},CollisionWindow{"bolt",2,4,true,{}}}; check(validate_collision_contract(shapes,windows,rig,6).ok(), "valid collision contract rejected");
    auto bad_windows=windows; bad_windows[1].end_tick=7; check(!validate_collision_contract(shapes,bad_windows,rig,6).ok(), "out-of-bounds hit window accepted");
    std::cout << "all gspl sprites animation tests passed\n"; return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

