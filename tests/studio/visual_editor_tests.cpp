#include "gspl/studio/gene_editor_model.hpp"
#include "gspl/studio/morphology_editor_model.hpp"
#include "gspl/studio/form_editor_model.hpp"
#include "gspl/studio/animation_editor_model.hpp"
#include "gspl/studio/behavior_editor_model.hpp"
#include "gspl/studio/combat_editor_model.hpp"
#include "gspl/studio/graph_editor_model.hpp"
#include "gspl/semantics.hpp"
#include "gspl/genes.hpp"
#include <cassert>
#include <cstdio>
#include <string>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; \
  fprintf(stdout, "  " name "..."); fflush(stdout); } while(0)
#define PASS() fprintf(stdout, " PASS\n")
#define FAIL(msg) do { fprintf(stdout, " FAIL: %s\n", msg); ++tests_failed; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

// ============================================================
// Gene editor model tests
// ============================================================
static void test_gene_editor_create() {
    TEST("create and refresh");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test-sprite";
    entity.name = "TestSprite";

    gspl::GeneInstance gi;
    gi.descriptor.kind = gspl::GeneKind::identity;
    gi.descriptor.type_id = "core.identity";
    gi.descriptor.schema_version = 1;
    gi.source_module = "main.gspl";
    entity.genes.push_back(gi);

    gspl::GeneInstance gi2;
    gi2.descriptor.kind = gspl::GeneKind::form;
    gi2.descriptor.type_id = "core.form";
    gi2.descriptor.schema_version = 1;
    gi2.source_module = "main.gspl";
    gi2.is_override = true;
    entity.genes.push_back(gi2);

    gspl::studio::GeneEditorModel model(entity);
    ASSERT(model.entries().empty(), "should be empty before refresh");
    model.refresh();
    ASSERT(model.entries().size() == 2, "should have 2 entries after refresh");
    ASSERT(model.entry(0) != nullptr, "entry 0 not null");
    ASSERT(model.entry(1) != nullptr, "entry 1 not null");
    ASSERT(model.entry(2) == nullptr, "entry 2 out of bounds");
    ASSERT(model.entries()[0].type_id == "core.identity", "first type_id");
    ASSERT(model.entries()[0].kind == static_cast<int>(gspl::GeneKind::identity), "first kind");
    ASSERT(model.entries()[0].is_override == false, "first not override");
    ASSERT(model.entries()[1].is_override == true, "second is override");
    PASS();
}

static void test_gene_find_by_type() {
    TEST("find_by_type");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::GeneInstance gi;
    gi.descriptor.kind = gspl::GeneKind::identity;
    gi.descriptor.type_id = "core.identity";
    entity.genes.push_back(gi);

    gspl::GeneInstance gi2;
    gi2.descriptor.kind = gspl::GeneKind::combat;
    gi2.descriptor.type_id = "core.combat";
    entity.genes.push_back(gi2);

    gspl::studio::GeneEditorModel model(entity);
    model.refresh();
    ASSERT(model.find_by_type("core.identity") != nullptr, "found identity");
    ASSERT(model.find_by_type("core.combat") != nullptr, "found combat");
    ASSERT(model.find_by_type("nonexistent") == nullptr, "not found");
    PASS();
}

static void test_gene_toggle_selection() {
    TEST("toggle_selection");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::GeneInstance gi;
    gi.descriptor.kind = gspl::GeneKind::identity;
    gi.descriptor.type_id = "core.identity";
    entity.genes.push_back(gi);

    gspl::studio::GeneEditorModel model(entity);
    model.refresh();

    ASSERT(model.entries()[0].selected == true, "initially selected");
    ASSERT(model.toggle_selection(0) == true, "toggle ok");
    ASSERT(model.entries()[0].selected == false, "after toggle deselected");
    ASSERT(model.toggle_selection(0) == true, "toggle back ok");
    ASSERT(model.entries()[0].selected == true, "after second toggle selected");
    ASSERT(model.toggle_selection(99) == false, "invalid index");
    PASS();
}

static void test_gene_resolve_conflict() {
    TEST("resolve_conflict");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::GeneInstance gi;
    gi.descriptor.kind = gspl::GeneKind::identity;
    gi.descriptor.type_id = "core.identity";
    entity.genes.push_back(gi);

    gspl::studio::GeneEditorModel model(entity);
    model.refresh();

    // No conflicts initially
    ASSERT(model.conflicts().empty(), "no conflicts initially");
    // resolve_conflict on non-conflicted entry does nothing but returns false
    ASSERT(model.resolve_conflict(0) == false, "no conflict to resolve");
    ASSERT(model.resolve_conflict(99) == false, "invalid index");
    PASS();
}

// ============================================================
// Morphology editor model tests
// ============================================================
static void test_morphology_editor_create() {
    TEST("create and refresh");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalPart part;
    part.name = "torso";
    part.parent = "";
    part.x = 0; part.y = 0; part.z = 0;
    part.size_x = 2; part.size_y = 3; part.size_z = 1;
    entity.morphology["torso"] = part;

    gspl::CanonicalPart head;
    head.name = "head";
    head.parent = "torso";
    head.x = 0; head.y = 2; head.z = 0;
    entity.morphology["head"] = head;

    gspl::studio::MorphologyEditorModel model(entity);
    model.refresh();
    ASSERT(model.entries().size() == 2, "2 parts");
    ASSERT(model.entry(0) != nullptr, "entry 0");
    ASSERT(model.entry(0)->name == "head" || model.entry(0)->name == "torso",
           "has one of the expected parts");
    ASSERT(model.entry(2) == nullptr, "out of bounds");
    PASS();
}

static void test_morphology_find_and_children() {
    TEST("find_by_name and children_of");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalPart root;
    root.name = "root"; root.parent = "";
    entity.morphology["root"] = root;

    gspl::CanonicalPart child;
    child.name = "child"; child.parent = "root";
    entity.morphology["child"] = child;

    gspl::CanonicalPart sub;
    sub.name = "sub"; sub.parent = "child";
    entity.morphology["sub"] = sub;

    gspl::studio::MorphologyEditorModel model(entity);
    model.refresh();
    ASSERT(model.find_by_name("root") != nullptr, "found root");
    ASSERT(model.find_by_name("child") != nullptr, "found child");
    ASSERT(model.find_by_name("none") == nullptr, "not found");

    auto children = model.children_of("root");
    ASSERT(children.size() == 1, "root has 1 child");
    ASSERT(children[0]->name == "child", "root child is child");
    PASS();
}

static void test_morphology_add_remove() {
    TEST("add_part and remove_part");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::studio::MorphologyEditorModel model(entity);
    model.refresh();
    ASSERT(model.entries().empty(), "empty initially");

    ASSERT(model.add_part("torso", "", 0, 0, 0, 2, 3, 1) == true, "add torso");
    ASSERT(model.add_part("head", "torso", 0, 2, 0, 1, 1, 1) == true, "add head");
    ASSERT(model.entries().size() == 2, "2 after add");

    ASSERT(model.add_part("torso", "", 0, 0, 0, 2, 3, 1) == false, "duplicate name rejected");

    ASSERT(model.remove_part("head") == true, "remove head");
    ASSERT(model.entries().size() == 1, "1 after remove");
    ASSERT(model.remove_part("none") == false, "remove nonexistent");
    PASS();
}

static void test_morphology_update() {
    TEST("update_position and update_size");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalPart part;
    part.name = "torso";
    part.parent = "";
    entity.morphology["torso"] = part;

    gspl::studio::MorphologyEditorModel model(entity);
    model.refresh();

    ASSERT(model.update_position("torso", 10, 20, 30) == true, "update position");
    ASSERT(model.entries()[0].x == 10.0 && model.entries()[0].y == 20.0 && model.entries()[0].z == 30.0,
           "position updated");
    ASSERT(model.update_position("none", 1, 2, 3) == false, "nonexistent");

    ASSERT(model.update_size("torso", 4, 5, 6) == true, "update size");
    ASSERT(model.entries()[0].size_x == 4.0 && model.entries()[0].size_y == 5.0 && model.entries()[0].size_z == 6.0,
           "size updated");
    ASSERT(model.update_size("none", 1, 2, 3) == false, "nonexistent");
    PASS();
}

// ============================================================
// Form editor model tests
// ============================================================
static void test_form_editor_create() {
    TEST("create and refresh");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalForm f1;
    f1.id = "base";
    f1.resource_capacity = 100;
    f1.max_health = 100;
    entity.forms.push_back(f1);

    gspl::CanonicalForm f2;
    f2.id = "storm";
    f2.resource_capacity = 200;
    f2.max_health = 150;
    entity.forms.push_back(f2);

    gspl::studio::FormEditorModel model(entity);
    model.refresh();
    ASSERT(model.entries().size() == 2, "2 forms");
    ASSERT(model.entry(0) != nullptr, "entry 0");
    ASSERT(model.entry(2) == nullptr, "out of bounds");
    PASS();
}

static void test_form_find_add_remove() {
    TEST("find_by_id, add_form, remove_form");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::studio::FormEditorModel model(entity);
    model.refresh();
    ASSERT(model.entries().empty(), "empty initially");

    ASSERT(model.add_form("base") == true, "add base");
    ASSERT(model.add_form("base") == false, "duplicate rejected");
    ASSERT(model.entries().size() == 1, "1 form");
    ASSERT(model.find_by_id("base") != nullptr, "found by id");
    ASSERT(model.find_by_id("none") == nullptr, "not found");

    ASSERT(model.remove_form("base") == true, "remove base");
    ASSERT(model.entries().empty(), "empty after remove");
    ASSERT(model.remove_form("none") == false, "remove nonexistent");
    PASS();
}

static void test_form_update() {
    TEST("update_capacity, update_max_health");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalForm f;
    f.id = "base";
    entity.forms.push_back(f);

    gspl::studio::FormEditorModel model(entity);
    model.refresh();

    ASSERT(model.update_capacity("base", 250) == true, "update capacity");
    ASSERT(model.entries()[0].resource_capacity == 250, "capacity updated");
    ASSERT(model.update_capacity("none", 1) == false, "nonexistent");

    ASSERT(model.update_max_health("base", 200) == true, "update max_health");
    ASSERT(model.entries()[0].max_health == 200, "max_health updated");
    ASSERT(model.update_max_health("none", 1) == false, "nonexistent");
    PASS();
}

// ============================================================
// Animation editor model tests
// ============================================================
static void test_animation_editor_create() {
    TEST("create and refresh");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalAnimationClip clip;
    clip.name = "idle";
    clip.loop = true;
    gspl::CanonicalAnimationClip::Track track;
    track.bone = "torso";
    track.keys.push_back({0, "0,0,0"});
    track.keys.push_back({30, "0,10,0"});
    clip.tracks.push_back(track);
    entity.clips.push_back(clip);

    gspl::CanonicalAnimationClip clip2;
    clip2.name = "attack";
    clip2.loop = false;
    entity.clips.push_back(clip2);

    gspl::studio::AnimationEditorModel model(entity);
    model.refresh();
    ASSERT(model.clips().size() == 2, "2 clips");
    ASSERT(model.clip(0) != nullptr, "clip 0");
    ASSERT(model.clip(2) == nullptr, "out of bounds");
    ASSERT(model.clips()[0].name == "idle" || model.clips()[0].name == "attack",
           "has one of the expected names");
    PASS();
}

static void test_animation_find_add_remove() {
    TEST("find_clip, add_clip, remove_clip");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::studio::AnimationEditorModel model(entity);
    model.refresh();
    ASSERT(model.clips().empty(), "empty initially");

    ASSERT(model.add_clip("idle", true) == true, "add idle");
    ASSERT(model.add_clip("idle", true) == false, "duplicate rejected");
    ASSERT(model.clips().size() == 1, "1 clip");
    ASSERT(model.find_clip("idle") != nullptr, "found by name");
    ASSERT(model.find_clip("none") == nullptr, "not found");

    ASSERT(model.remove_clip("idle") == true, "remove idle");
    ASSERT(model.clips().empty(), "empty after remove");
    ASSERT(model.remove_clip("none") == false, "remove nonexistent");
    PASS();
}

static void test_animation_set_looping() {
    TEST("set_looping");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalAnimationClip clip;
    clip.name = "walk";
    clip.loop = true;
    entity.clips.push_back(clip);

    gspl::studio::AnimationEditorModel model(entity);
    model.refresh();
    ASSERT(model.clips()[0].loop == true, "initially loop");
    ASSERT(model.set_looping("walk", false) == true, "set not loop");
    ASSERT(model.clips()[0].loop == false, "loop updated");
    ASSERT(model.set_looping("none", true) == false, "nonexistent");
    PASS();
}

// ============================================================
// Behavior editor model tests
// ============================================================
static void test_behavior_editor_create() {
    TEST("create and refresh");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    // No runtime set - should derive empty rules
    gspl::studio::BehaviorEditorModel model(entity);
    model.refresh();
    ASSERT(model.rules().empty(), "empty without runtime");

    // With runtime
    gspl::CanonicalRuntime rt;
    rt.aggression = 80;
    rt.curiosity = 10;
    rt.energy = 50;
    rt.loyalty = 50;
    entity.runtime = rt;
    model.refresh();
    ASSERT(model.rules().size() >= 2, "has derived rules");
    PASS();
}

static void test_behavior_rule_management() {
    TEST("add_rule, remove_rule, toggle_rule, update_priority");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::studio::BehaviorEditorModel model(entity);
    model.refresh();

    ASSERT(model.add_rule("threat_nearby", "flee", 10) == true, "add rule");
    ASSERT(model.add_rule("object_nearby", "investigate", 5) == true, "add second rule");
    ASSERT(model.rules().size() == 2, "2 rules");

    ASSERT(model.toggle_rule(0) == true, "toggle first");
    ASSERT(model.rules()[0].active == false, "first inactive");
    ASSERT(model.toggle_rule(0) == true, "toggle back");
    ASSERT(model.rules()[0].active == true, "first active again");

    ASSERT(model.update_priority(0, 20) == true, "update priority");
    ASSERT(model.rules()[0].priority == 20, "priority updated");
    ASSERT(model.update_priority(99, 1) == false, "invalid index");

    auto sorted = model.sorted_by_priority();
    ASSERT(sorted.size() == 2, "sorted size");
    ASSERT(sorted[0].priority >= sorted[1].priority, "sorted descending");

    ASSERT(model.remove_rule(0) == true, "remove rule");
    ASSERT(model.rules().size() == 1, "1 after remove");
    ASSERT(model.remove_rule(99) == false, "invalid index remove");
    PASS();
}

// ============================================================
// Combat editor model tests
// ============================================================
static void test_combat_editor_create() {
    TEST("create and refresh");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalAbility ab;
    ab.id = "fireball";
    ab.effect = "projectile<fire>";
    ab.cost = 30;
    ab.cooldown_ticks = 10;
    ab.speed_mm_per_tick = 8.0;
    entity.abilities.push_back(ab);

    gspl::CanonicalAbility storm;
    storm.id = "storm_beam";
    storm.effect = "beam<lightning>";
    entity.storm_abilities.push_back(storm);

    gspl::studio::CombatEditorModel model(entity);
    model.refresh();
    ASSERT(model.abilities().size() == 2, "2 abilities (1 normal + 1 storm)");
    ASSERT(model.ability(0) != nullptr, "ability 0");
    ASSERT(model.ability(2) == nullptr, "out of bounds");
    PASS();
}

static void test_combat_find_add_remove() {
    TEST("find_by_id, add_ability, remove_ability");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::studio::CombatEditorModel model(entity);
    model.refresh();
    ASSERT(model.abilities().empty(), "empty initially");

    ASSERT(model.add_ability("slash", "melee<slash>", 15) == true, "add slash");
    ASSERT(model.add_ability("slash", "melee<slash>", 15) == false, "duplicate rejected");
    ASSERT(model.find_by_id("slash") != nullptr, "found by id");
    ASSERT(model.find_by_id("none") == nullptr, "not found");

    ASSERT(model.remove_ability("slash") == true, "remove slash");
    ASSERT(model.abilities().empty(), "empty after remove");
    ASSERT(model.remove_ability("none") == false, "remove nonexistent");
    PASS();
}

static void test_combat_storm_abilities() {
    TEST("storm_abilities");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalAbility ab;
    ab.id = "normal_shot";
    entity.abilities.push_back(ab);

    gspl::CanonicalAbility storm;
    storm.id = "storm_blast";
    entity.storm_abilities.push_back(storm);

    gspl::studio::CombatEditorModel model(entity);
    model.refresh();
    auto storms = model.storm_abilities();
    ASSERT(storms.size() == 1, "1 storm ability");
    ASSERT(storms[0].ability_id == "storm_blast", "correct storm id");
    ASSERT(model.find_by_id("normal_shot") != nullptr, "find normal");
    ASSERT(model.find_by_id("storm_blast") != nullptr, "find storm");
    PASS();
}

static void test_combat_update_cooldown() {
    TEST("update_cooldown");
    gspl::CanonicalEntity entity;
    entity.stable_id = "test";
    entity.name = "Test";

    gspl::CanonicalAbility ab;
    ab.id = "spell";
    entity.abilities.push_back(ab);

    gspl::studio::CombatEditorModel model(entity);
    model.refresh();
    ASSERT(model.update_cooldown("spell", 50) == true, "update cooldown");
    ASSERT(model.ability(0) != nullptr && model.ability(0)->cooldown_ticks == 50, "cooldown set");
    ASSERT(model.update_cooldown("none", 1) == false, "nonexistent");
    PASS();
}

// ============================================================
// Graph editor model tests
// ============================================================
static void test_graph_editor_create() {
    TEST("add_node and basic access");
    gspl::studio::GraphEditorModel model;
    auto id1 = model.add_node("identity", "gene", 100, 200);
    ASSERT(!id1.empty(), "got node id");
    ASSERT(model.node(id1) != nullptr, "find by id");
    ASSERT(model.node(id1)->label == "identity", "label matches");
    ASSERT(model.node(id1)->node_type == "gene", "type matches");
    ASSERT(model.node(id1)->x == 100.0 && model.node(id1)->y == 200.0, "position matches");
    ASSERT(model.nodes().size() == 1, "1 node");
    PASS();
}

static void test_graph_remove_move_node() {
    TEST("remove_node and move_node");
    gspl::studio::GraphEditorModel model;
    auto id1 = model.add_node("A", "signal", 0, 0);
    auto id2 = model.add_node("B", "signal", 100, 0);
    model.add_edge(id1, "out", id2, "in");

    ASSERT(model.nodes().size() == 2, "2 nodes");
    ASSERT(model.edges().size() == 1, "1 edge after add");

    ASSERT(model.move_node(id1, 50, 50) == true, "move node");
    ASSERT(model.node(id1)->x == 50.0 && model.node(id1)->y == 50.0, "position updated");
    ASSERT(model.move_node("none", 0, 0) == false, "nonexistent");

    ASSERT(model.remove_node(id1) == true, "remove node");
    ASSERT(model.nodes().size() == 1, "1 after remove");
    ASSERT(model.edges().empty(), "edge also removed");
    ASSERT(model.remove_node("none") == false, "remove nonexistent");
    PASS();
}

static void test_graph_edge_management() {
    TEST("add_edge, remove_edge, connected_nodes");
    gspl::studio::GraphEditorModel model;
    auto a = model.add_node("A", "gene", 0, 0);
    auto b = model.add_node("B", "gene", 100, 0);

    auto eid = model.add_edge(a, "out", b, "in");
    ASSERT(!eid.empty(), "edge id not empty");
    ASSERT(model.edge(eid) != nullptr, "find edge by id");
    ASSERT(model.edges().size() == 1, "1 edge");

    ASSERT(model.add_edge("none", "out", b, "in").empty(), "invalid source");
    ASSERT(model.add_edge(a, "out", "none", "in").empty(), "invalid target");

    auto connected = model.connected_nodes(a);
    ASSERT(connected.size() == 1, "1 connected to A");
    ASSERT(connected[0].id == b, "B connected to A");

    ASSERT(model.remove_edge(eid) == true, "remove edge");
    ASSERT(model.edges().empty(), "0 edges after remove");
    ASSERT(model.remove_edge(eid) == false, "double remove");
    PASS();
}

static void test_graph_clear() {
    TEST("clear");
    gspl::studio::GraphEditorModel model;
    model.add_node("A", "gene", 0, 0);
    model.add_node("B", "signal", 100, 0);
    ASSERT(model.nodes().size() == 2, "2 nodes");
    model.clear();
    ASSERT(model.nodes().empty(), "empty after clear");
    ASSERT(model.edges().empty(), "edges after clear");
    PASS();
}

// ============================================================
int main() {
    // Gene editor
    test_gene_editor_create();
    test_gene_find_by_type();
    test_gene_toggle_selection();
    test_gene_resolve_conflict();

    // Morphology editor
    test_morphology_editor_create();
    test_morphology_find_and_children();
    test_morphology_add_remove();
    test_morphology_update();

    // Form editor
    test_form_editor_create();
    test_form_find_add_remove();
    test_form_update();

    // Animation editor
    test_animation_editor_create();
    test_animation_find_add_remove();
    test_animation_set_looping();

    // Behavior editor
    test_behavior_editor_create();
    test_behavior_rule_management();

    // Combat editor
    test_combat_editor_create();
    test_combat_find_add_remove();
    test_combat_storm_abilities();
    test_combat_update_cooldown();

    // Graph editor
    test_graph_editor_create();
    test_graph_remove_move_node();
    test_graph_edge_management();
    test_graph_clear();

    fprintf(stdout, "\n%d tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
