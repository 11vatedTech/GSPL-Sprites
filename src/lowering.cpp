#include "gspl/lowering.hpp"
#include <charconv>
#include <ranges>
#include <sstream>

namespace gspl {

// ── SpriteIrLowering: Direct CanonicalEntity → production SpriteIr ─
SpriteIrLowering::Result SpriteIrLowering::lower(CanonicalEntity const& entity) {
    Result result;
    auto& ir = result.sprite_ir;

    ir.seed_identity = entity.provenance_hash.empty() ? "gspl." + entity.stable_id : entity.provenance_hash;
    ir.entity_id = entity.stable_id;
    ir.name = entity.name;
    ir.classification = entity.classification;
    ir.rights = lower_rights(entity.rights);
    if (ir.rights == gspl::sprites::RightsClass::unknown) {
        result.diagnostics.push_back({LoweringDiagnostic::Code::RIGHTS_INVALID,
            "Unrecognized rights classification: " + entity.rights});
    }
    ir.provenance_hash = entity.provenance_hash;
    ir.schema_version = "gspl.canonical-entity/1.0";
    ir.primary_color = entity.primary_color;
    ir.accent_color = entity.accent_color;
    ir.storm_primary_color = entity.storm_primary_color;
    ir.storm_accent_color = entity.storm_accent_color;
    ir.emissive_color = entity.emissive_color;
    ir.aura_color = entity.aura_color;

    // Lower abilities
    for (auto const& a : entity.abilities) ir.abilities.push_back(lower_ability(a));
    for (auto const& a : entity.storm_abilities) ir.storm_abilities.push_back(lower_ability(a));

    // Lower forms → FormDefinitions
    for (auto const& f : entity.forms) {
        gspl::sprites::FormDefinition fd;
        fd.name = f.id;
        fd.transformation_names = f.transformation_ids;
        ir.form_definitions.push_back(std::move(fd));
        ir.form_attributes[f.id] = lower_form_attributes(f);
    }

    // Lower transformations → TransformationDeltas
    for (auto const& t : entity.transformations) {
        gspl::sprites::TransformationDelta td;
        td.name = t.id;
        td.from_form = t.from_form;
        td.to_form = t.to_form;
        td.trigger_condition = t.trigger_condition;
        ir.transformation_deltas.push_back(std::move(td));
    }

    // Lower morphology
    for (auto const& [name, part] : entity.morphology)
        ir.morphology[name] = lower_part(part);
    for (auto const& [form_name, overrides] : entity.form_morphology_overrides)
        for (auto const& [name, part] : overrides)
            ir.form_morphology_overrides[form_name][name] = lower_part(part);

    // Lower rig (bones + sockets)
    if (!entity.bones.empty() || !entity.sockets.empty()) {
        gspl::sprites::RigDefinition rig;
        rig.id = entity.stable_id + ".rig";
        for (auto const& b : entity.bones) rig.bones.push_back(lower_bone(b));
        for (auto const& s : entity.sockets) rig.sockets.push_back(lower_socket(s));
        ir.rig = std::move(rig);
    }

    // Lower animation clips
    for (auto const& c : entity.clips) ir.clips.push_back(lower_clip(c));

    // Lower animation state graph
    if (!entity.states.empty()) {
        gspl::sprites::AnimationStateGraph g;
        g.initial_state = entity.initial_state;
        for (auto const& s : entity.states) g.states.push_back(lower_state(s));
        for (auto const& t : entity.transitions) {
            if (g.states.empty()) break;
            auto& last_state = g.states.back();
            last_state.transitions.push_back(lower_transition(t));
        }
        ir.animation_graph = std::move(g);
    }

    // Lower collision
    for (auto const& s : entity.collision_shapes) ir.collision_shapes.push_back(lower_collision_shape(s));
    for (auto const& w : entity.collision_windows) ir.collision_windows.push_back(lower_collision_window(w));

    // Lower runtime
    if (entity.runtime) {
        ir.runtime = lower_runtime(*entity.runtime);
        for (auto const& intent : entity.runtime->animation_intents) {
            ir.animation_intents.push_back({intent.behavior_state, intent.clip_name});
        }
    }

    return result;
}

gspl::sprites::RightsClass SpriteIrLowering::lower_rights(std::string const& classification) {
    if (classification.find("ORIGINAL_USER_CREATION") != std::string::npos)
        return gspl::sprites::RightsClass::original_user_creation;
    if (classification.find("USER_OWNED") != std::string::npos)
        return gspl::sprites::RightsClass::user_owned;
    if (classification.find("LICENSED") != std::string::npos)
        return gspl::sprites::RightsClass::licensed;
    if (classification.find("PUBLIC_DOMAIN") != std::string::npos)
        return gspl::sprites::RightsClass::public_domain;
    if (classification.find("PERMISSIVE") != std::string::npos)
        return gspl::sprites::RightsClass::permissive;
    if (classification.find("RESEARCH_ONLY") != std::string::npos)
        return gspl::sprites::RightsClass::research_only;
    if (classification.find("RESTRICTED") != std::string::npos)
        return gspl::sprites::RightsClass::restricted;
    if (classification.find("PROHIBITED") != std::string::npos)
        return gspl::sprites::RightsClass::prohibited;
    return gspl::sprites::RightsClass::unknown;
}

gspl::sprites::AbilitySeed SpriteIrLowering::lower_ability(CanonicalAbility const& ability) {
    gspl::sprites::AbilitySeed as;
    as.id = ability.id;
    as.effect = ability.effect;
    as.cost = ability.cost;
    as.cooldown_ticks = ability.cooldown_ticks;
    as.active_ticks = ability.active_ticks;
    return as;
}

gspl::sprites::MorphologyPart SpriteIrLowering::lower_part(CanonicalPart const& part) {
    gspl::sprites::MorphologyPart mp;
    mp.x = part.x; mp.y = part.y; mp.z = part.z;
    mp.size_x = part.size_x; mp.size_y = part.size_y; mp.size_z = part.size_z;
    mp.color = part.color;
    mp.rotation_degrees = part.rotation_degrees;
    mp.parent = part.parent;
    mp.emissive = part.emissive;
    mp.electrical_marking = part.electrical_marking;
    return mp;
}

gspl::sprites::FormAttributes SpriteIrLowering::lower_form_attributes(CanonicalForm const& form) {
    gspl::sprites::FormAttributes fa;
    fa.resource_capacity = form.resource_capacity;
    fa.collision_scale = form.collision_scale;
    fa.ability_envelope = form.ability_envelope;
    fa.max_health = form.max_health;
    return fa;
}

gspl::sprites::RuntimeAttributes SpriteIrLowering::lower_runtime(CanonicalRuntime const& runtime) {
    gspl::sprites::RuntimeAttributes ra;
    ra.aggression = runtime.aggression;
    ra.curiosity = runtime.curiosity;
    ra.energy = runtime.energy;
    ra.loyalty = runtime.loyalty;
    return ra;
}

gspl::sprites::BoneDefinition SpriteIrLowering::lower_bone(CanonicalSkeletalBone const& bone) {
    gspl::sprites::BoneDefinition bd;
    bd.id = bone.id;
    if (!bone.parent.empty()) bd.parent_id = bone.parent;
    bd.rest.x = bone.x;
    bd.rest.y = bone.y;
    bd.length = bone.length_mm;
    bd.limit.minimum_degrees = bone.min_rotation;
    bd.limit.maximum_degrees = bone.max_rotation;
    return bd;
}

gspl::sprites::SocketDefinition SpriteIrLowering::lower_socket(CanonicalSocket const& socket) {
    gspl::sprites::SocketDefinition sd;
    sd.id = socket.id;
    sd.bone_id = socket.bone;
    sd.local.x = socket.x;
    sd.local.y = socket.y;
    return sd;
}

gspl::sprites::SkeletalClip SpriteIrLowering::lower_clip(CanonicalAnimationClip const& clip) {
    gspl::sprites::SkeletalClip sc;
    sc.id = clip.name;
    sc.looping = clip.loop;
    std::uint32_t max_tick = 0;
    for (auto const& track : clip.tracks) {
        gspl::sprites::BoneTrack bt;
        bt.bone_id = track.bone;
        for (auto const& [tick, value] : track.keys) {
            (void)value;
            gspl::sprites::BoneKeyframe kf;
            kf.tick = tick;
            max_tick = (std::max)(max_tick, tick);
            bt.keys.push_back(std::move(kf));
        }
        sc.tracks.push_back(std::move(bt));
    }
    sc.duration_ticks = max_tick + 1;
    for (auto const& [tick, event_id] : clip.clip_events)
        sc.events.push_back({event_id, tick});
    return sc;
}

gspl::sprites::AnimationState SpriteIrLowering::lower_state(CanonicalAnimationState const& state) {
    gspl::sprites::AnimationState as;
    as.id = state.name;
    as.clip_id = state.clip_name;
    return as;
}

gspl::sprites::AnimationTransition SpriteIrLowering::lower_transition(CanonicalTransition const& trans) {
    gspl::sprites::AnimationTransition at;
    at.target_state = trans.to_state;
    at.parameter = trans.ability_id;
    at.comparison = lower_comparison(trans.comparison);
    at.threshold = static_cast<double>(trans.threshold);
    return at;
}

gspl::sprites::CollisionShape SpriteIrLowering::lower_collision_shape(CanonicalCollisionShape const& shape) {
    gspl::sprites::CollisionShape cs;
    cs.id = shape.id;
    cs.kind = (shape.shape_type.find("CIRCLE") != std::string::npos)
        ? gspl::sprites::CollisionKind::circle
        : gspl::sprites::CollisionKind::axis_aligned_box;
    cs.bone_id = shape.socket;
    cs.offset_x = shape.offset_x;
    cs.offset_y = shape.offset_y;
    cs.extent_x = shape.scale_x;
    cs.extent_y = shape.scale_y;
    return cs;
}

gspl::sprites::CollisionWindow SpriteIrLowering::lower_collision_window(CanonicalCollisionWindow const& window) {
    gspl::sprites::CollisionWindow cw;
    cw.shape_id = window.shape_id;
    cw.start_tick = window.start_tick;
    cw.end_tick = window.start_tick + window.duration_ticks;
    cw.deals_damage = window.active;
    cw.ability_id = window.ability_id;
    return cw;
}

gspl::sprites::Comparison SpriteIrLowering::lower_comparison(std::string const& comp) {
    if (comp == "EQUAL") return gspl::sprites::Comparison::equal;
    if (comp == "NOT_EQUAL") return gspl::sprites::Comparison::not_equal;
    if (comp == "LESS") return gspl::sprites::Comparison::less;
    if (comp == "LESS_EQUAL") return gspl::sprites::Comparison::less_equal;
    if (comp == "GREATER") return gspl::sprites::Comparison::greater;
    if (comp == "GREATER_EQUAL") return gspl::sprites::Comparison::greater_equal;
    return gspl::sprites::Comparison::equal;
}

std::uint64_t SpriteIrLowering::parse_hex_color(std::string const& hex, std::vector<LoweringDiagnostic>&) {
    if (hex.empty() || hex[0] != '#') return 0;
    std::uint64_t val = 0;
    std::from_chars(hex.data() + 1, hex.data() + hex.size(), val, 16);
    return val;
}

// ── SpriteSeedLowering: Compatibility lowering via SpriteSeed ─────
gspl::sprites::RightsClass SpriteSeedLowering::rights_to_enum(std::string const& classification) {
    return SpriteIrLowering::lower_rights(classification);
}

std::uint64_t SpriteSeedLowering::parse_hex_color(std::string const& hex) {
    std::vector<LoweringDiagnostic> diags;
    return SpriteIrLowering::parse_hex_color(hex, diags);
}

gspl::sprites::AbilitySeed SpriteSeedLowering::lower_ability(CanonicalAbility const& ability) {
    return SpriteIrLowering::lower_ability(ability);
}

gspl::sprites::FormSeed SpriteSeedLowering::lower_form(CanonicalForm const& form) {
    gspl::sprites::FormSeed fs;
    fs.id = form.id;
    fs.transformation_ids = form.transformation_ids;
    return fs;
}

gspl::sprites::TransformationSeed SpriteSeedLowering::lower_transformation(CanonicalTransformation const& trans) {
    gspl::sprites::TransformationSeed ts;
    ts.id = trans.id;
    ts.from_form = trans.from_form;
    ts.to_form = trans.to_form;
    ts.trigger_condition = trans.trigger_condition;
    ts.duration_ticks = trans.duration_ticks;
    ts.resource_cost = trans.resource_cost;
    return ts;
}

gspl::sprites::MorphologyPart SpriteSeedLowering::lower_part(CanonicalPart const& part) {
    return SpriteIrLowering::lower_part(part);
}

gspl::sprites::FormAttributes SpriteSeedLowering::lower_form_attributes(CanonicalForm const& form) {
    return SpriteIrLowering::lower_form_attributes(form);
}

gspl::sprites::RuntimeAttributes SpriteSeedLowering::lower_runtime(CanonicalRuntime const& runtime) {
    return SpriteIrLowering::lower_runtime(runtime);
}

gspl::sprites::SpriteSeed SpriteSeedLowering::lower(CanonicalEntity const& entity) {
    gspl::sprites::SpriteSeed seed;
    seed.schema = "gspl.sprite-seed/0.2";
    seed.stable_id = entity.stable_id;
    seed.name = entity.name;
    seed.classification = entity.classification;
    seed.rights = entity.rights_allow_export
        ? rights_to_enum(entity.rights)
        : gspl::sprites::RightsClass::research_only;
    seed.entropy_root = entity.entropy_root;
    seed.primary_color = entity.primary_color;
    seed.accent_color = entity.accent_color;
    seed.storm_primary_color = entity.storm_primary_color;
    seed.storm_accent_color = entity.storm_accent_color;
    seed.emissive_color = entity.emissive_color;
    seed.aura_color = entity.aura_color;

    for (auto const& ability : entity.abilities)
        seed.abilities.push_back(lower_ability(ability));
    for (auto const& ability : entity.storm_abilities)
        seed.storm_abilities.push_back(lower_ability(ability));

    for (auto const& form : entity.forms) {
        seed.forms.push_back(lower_form(form));
        seed.form_attributes[form.id] = lower_form_attributes(form);
    }

    for (auto const& trans : entity.transformations)
        seed.transformations.push_back(lower_transformation(trans));

    for (auto const& [name, part] : entity.morphology)
        seed.morphology[name] = lower_part(part);

    for (auto const& [form_name, overrides] : entity.form_morphology_overrides) {
        for (auto const& [name, part] : overrides)
            seed.form_morphology_overrides[form_name][name] = lower_part(part);
    }

    if (entity.runtime)
        seed.runtime = lower_runtime(*entity.runtime);

    return seed;
}

} // namespace gspl
