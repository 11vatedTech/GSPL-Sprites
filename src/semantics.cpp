#include "gspl/semantics.hpp"
#include "gspl/ast.hpp"
#include "gspl/genes.hpp"
#include <algorithm>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <string>

namespace gspl {

// ── CanonicalEntitySerializer ──────────────────────────────────────
std::string CanonicalEntitySerializer::to_json(CanonicalEntity const& entity) {
    std::ostringstream os;
    os << "{\n";
    os << "  \"schema_version\": \"" << entity.schema_version << "\",\n";
    os << "  \"stable_id\": \"" << entity.stable_id << "\",\n";
    os << "  \"name\": \"" << entity.name << "\",\n";
    os << "  \"classification\": \"" << entity.classification << "\",\n";
    os << "  \"rights\": \"" << entity.rights << "\",\n";
    os << "  \"rights_allow_export\": " << (entity.rights_allow_export ? "true" : "false") << ",\n";
    os << "  \"entropy_root\": " << entity.entropy_root << ",\n";
    os << "  \"primary_color\": \"" << entity.primary_color << "\",\n";
    os << "  \"accent_color\": \"" << entity.accent_color << "\",\n";
    os << "  \"storm_primary_color\": \"" << entity.storm_primary_color << "\",\n";
    os << "  \"storm_accent_color\": \"" << entity.storm_accent_color << "\",\n";
    os << "  \"emissive_color\": \"" << entity.emissive_color << "\",\n";
    os << "  \"aura_color\": \"" << entity.aura_color << "\",\n";
    os << "  \"provenance_hash\": \"" << entity.provenance_hash << "\",\n";
    os << "  \"provenance_source\": \"" << entity.provenance_source << "\",\n";
    os << "  \"form_count\": " << entity.forms.size() << ",\n";
    os << "  \"transformation_count\": " << entity.transformations.size() << ",\n";
    os << "  \"morphology_part_count\": " << entity.morphology.size() << ",\n";
    os << "  \"ability_count\": " << entity.abilities.size() << ",\n";
    os << "  \"storm_ability_count\": " << entity.storm_abilities.size() << ",\n";
    os << "  \"bone_count\": " << entity.bones.size() << ",\n";
    os << "  \"socket_count\": " << entity.sockets.size() << ",\n";
    os << "  \"clip_count\": " << entity.clips.size() << ",\n";
    os << "  \"state_count\": " << entity.states.size() << ",\n";
    os << "  \"transition_count\": " << entity.transitions.size() << ",\n";
    os << "  \"collision_shape_count\": " << entity.collision_shapes.size() << ",\n";
    os << "  \"collision_window_count\": " << entity.collision_windows.size() << ",\n";
    os << "  \"gene_count\": " << entity.genes.size() << "\n";
    os << "}";
    return os.str();
}

std::optional<CanonicalEntity> CanonicalEntitySerializer::from_json(
    std::string_view json, DiagnosticResult&) {
    // Minimal deserialization; full parser would use a JSON library
    // Returns empty for now; full implementation would parse key-value pairs
    (void)json;
    return std::nullopt;
}

std::string CanonicalEntitySerializer::to_yaml(CanonicalEntity const& entity) {
    std::ostringstream os;
    os << "canonical_entity:\n";
    os << "  schema_version: " << entity.schema_version << "\n";
    os << "  stable_id: " << entity.stable_id << "\n";
    os << "  name: " << entity.name << "\n";
    os << "  classification: " << entity.classification << "\n";
    os << "  rights: " << entity.rights << "\n";
    os << "  rights_allow_export: " << (entity.rights_allow_export ? "true" : "false") << "\n";
    os << "  forms: " << entity.forms.size() << "\n";
    os << "  transformations: " << entity.transformations.size() << "\n";
    os << "  morphology_parts: " << entity.morphology.size() << "\n";
    os << "  bones: " << entity.bones.size() << "\n";
    os << "  sockets: " << entity.sockets.size() << "\n";
    os << "  clips: " << entity.clips.size() << "\n";
    os << "  states: " << entity.states.size() << "\n";
    os << "  collision_shapes: " << entity.collision_shapes.size() << "\n";
    return os.str();
}

// ── CanonicalEntityValidator ───────────────────────────────────────
DiagnosticResult CanonicalEntityValidator::validate(CanonicalEntity const& entity) const {
    DiagnosticResult result;
    auto add = [&](DiagnosticCode code, std::string const& msg) {
        result.add({code, DiagnosticSeverity::error, msg, {}, {}, {}});
    };
    if (entity.stable_id.empty()) add(DiagnosticCode::GSPL_NAME_UNKNOWN, "stable_id is required");
    if (entity.name.empty()) add(DiagnosticCode::GSPL_NAME_UNKNOWN, "name is required");
    if (entity.rights.empty()) add(DiagnosticCode::GSPL_RIGHTS_VIOLATION, "rights classification is required");
    return result;
}

// ── CanonicalEntityIdentity ────────────────────────────────────────
CanonicalEntityIdentity::CanonicalEntityIdentity(CanonicalEntity const& entity) {
    // Deterministic identity: hash of key semantic fields
    std::ostringstream os;
    os << "gspl.canonical-entity/1.0|" << entity.stable_id << "|" << entity.name
       << "|" << entity.classification << "|" << entity.rights
       << "|" << entity.primary_color << "|" << entity.accent_color
       << "|f:" << entity.forms.size() << "|t:" << entity.transformations.size()
       << "|m:" << entity.morphology.size() << "|b:" << entity.bones.size()
       << "|a:" << entity.abilities.size() << "|c:" << entity.clips.size()
       << "|s:" << entity.states.size() << "|cs:" << entity.collision_shapes.size();
    serialized_ = os.str();
    hash_ = std::to_string(std::hash<std::string>{}(serialized_));
}

std::string CanonicalEntityIdentity::compute(CanonicalEntity const& entity) const {
    return CanonicalEntitySerializer::to_json(entity);
}

// ── CanonicalEntityDiff ────────────────────────────────────────────
std::vector<CanonicalEntityDiff::Entry> CanonicalEntityDiff::diff(
    CanonicalEntity const& before, CanonicalEntity const& after) {
    std::vector<Entry> entries;
    auto check_str = [&](std::string const& field, auto const& b, auto const& a) {
        if (b != a) entries.push_back({field, b, a, true});
    };
    check_str("stable_id", before.stable_id, after.stable_id);
    check_str("name", before.name, after.name);
    check_str("classification", before.classification, after.classification);
    check_str("rights", before.rights, after.rights);
    check_str("primary_color", before.primary_color, after.primary_color);
    check_str("accent_color", before.accent_color, after.accent_color);
    if (before.forms.size() != after.forms.size())
        entries.push_back({"forms", std::to_string(before.forms.size()), std::to_string(after.forms.size()), true});
    if (before.transformations.size() != after.transformations.size())
        entries.push_back({"transformations", std::to_string(before.transformations.size()), std::to_string(after.transformations.size()), true});
    if (before.morphology.size() != after.morphology.size())
        entries.push_back({"morphology", std::to_string(before.morphology.size()), std::to_string(after.morphology.size()), true});
    if (before.abilities.size() != after.abilities.size())
        entries.push_back({"abilities", std::to_string(before.abilities.size()), std::to_string(after.abilities.size()), true});
    if (before.bones.size() != after.bones.size())
        entries.push_back({"bones", std::to_string(before.bones.size()), std::to_string(after.bones.size()), true});
    if (before.clips.size() != after.clips.size())
        entries.push_back({"clips", std::to_string(before.clips.size()), std::to_string(after.clips.size()), true});
    if (before.states.size() != after.states.size())
        entries.push_back({"states", std::to_string(before.states.size()), std::to_string(after.states.size()), true});
    if (before.collision_shapes.size() != after.collision_shapes.size())
        entries.push_back({"collision_shapes", std::to_string(before.collision_shapes.size()), std::to_string(after.collision_shapes.size()), true});
    return entries;
}

std::string CanonicalEntityDiff::to_text(std::vector<Entry> const& entries) {
    if (entries.empty()) return "IDENTICAL";
    std::ostringstream os;
    for (auto const& e : entries) {
        os << (e.changed ? "CHANGED" : "UNCHANGED") << " " << e.field
           << ": '" << e.before << "' -> '" << e.after << "'\n";
    }
    return os.str();
}

bool CanonicalEntityDiff::identical(std::vector<Entry> const& entries) {
    return entries.empty();
}

// ── Canonicalizer ──────────────────────────────────────────────────
Canonicalizer::Canonicalizer(SourceManager const& sources) : sources_(sources) {}

void Canonicalizer::lower_rights(RightsDecl const& rights, CanonicalEntity& out) {
    out.rights = rights.classification;
    out.rights_allow_export = !rights.classification.empty() &&
        rights.classification.find("PROHIBITED") == std::string::npos &&
        rights.classification.find("RESEARCH_ONLY") == std::string::npos;
}

void Canonicalizer::lower_form(FormDecl const& form, CanonicalEntity& out) {
    CanonicalForm cf;
    cf.id = form.name;
    if (form.extends) {
        auto it = std::ranges::find_if(out.forms, [&](auto const& f) { return f.id == *form.extends; });
        if (it != out.forms.end()) cf = *it;
    }
    for (auto const& child : form.body) {
        if (auto const* attr = dynamic_cast<AttributeNode const*>(child.get())) {
            if (attr->key == "resource_capacity" && attr->value) {
                if (auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get()))
                    cf.resource_capacity = static_cast<std::uint32_t>(std::stoul(lit->value));
            }
            if (attr->key == "collision_scale" && attr->value) {
                if (auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get()))
                    cf.collision_scale = std::stod(lit->value);
            }
            if (attr->key == "max_health" && attr->value) {
                if (auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get()))
                    cf.max_health = static_cast<std::uint32_t>(std::stoul(lit->value));
            }
            if (attr->key == "ability_envelope" && attr->value) {
                if (auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get()))
                    cf.ability_envelope = std::stod(lit->value);
            }
            if (attr->key == "transformations" && attr->value) {
                if (auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get())) {
                    std::string s = lit->value;
                    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                        s = s.substr(1, s.size() - 2);
                    std::size_t pos = 0;
                    while ((pos = s.find(',')) != std::string::npos) {
                        cf.transformation_ids.push_back(s.substr(0, pos));
                        s.erase(0, pos + 1);
                    }
                    if (!s.empty()) cf.transformation_ids.push_back(s);
                }
            }
        }
    }
    out.forms.push_back(std::move(cf));
}

void Canonicalizer::lower_transformation(TransformationDecl const& trans, CanonicalEntity& out) {
    CanonicalTransformation ct;
    ct.id = trans.name;
    ct.from_form = trans.from_form;
    ct.to_form = trans.to_form;
    ct.trigger_condition = "MANUAL";
    if (trans.duration) {
        if (auto const* lit = dynamic_cast<LiteralNode const*>(trans.duration.get()))
            ct.duration_ticks = static_cast<std::uint32_t>(std::stoul(lit->value));
    }
    if (trans.resource_cost) {
        if (auto const* lit = dynamic_cast<LiteralNode const*>(trans.resource_cost.get()))
            ct.resource_cost = static_cast<std::uint32_t>(std::stoul(lit->value));
    }
    out.transformations.push_back(std::move(ct));
}

void Canonicalizer::lower_morphology(MorphologyDecl const& morph, CanonicalEntity& out) {
    for (auto const& child : morph.parts) {
        auto const* part = dynamic_cast<PartDecl const*>(child.get());
        if (!part) continue;
        CanonicalPart cp;
        cp.name = part->name;
        cp.color = "#888888";
        for (auto const& attr : part->attributes) {
            auto const* a = dynamic_cast<AttributeNode const*>(attr.get());
            if (!a || !a->value) continue;
            auto const* lit = dynamic_cast<LiteralNode const*>(a->value.get());
            if (!lit) continue;
            if (a->key == "x") cp.x = std::stod(lit->value);
            else if (a->key == "y") cp.y = std::stod(lit->value);
            else if (a->key == "z") cp.z = std::stod(lit->value);
            else if (a->key == "size_x") cp.size_x = std::stod(lit->value);
            else if (a->key == "size_y") cp.size_y = std::stod(lit->value);
            else if (a->key == "size_z") cp.size_z = std::stod(lit->value);
            else if (a->key == "color") cp.color = lit->value;
            else if (a->key == "rotation_degrees") cp.rotation_degrees = std::stod(lit->value);
            else if (a->key == "parent") cp.parent = lit->value;
            else if (a->key == "emissive") cp.emissive = (lit->value == "true");
            else if (a->key == "electrical_marking") cp.electrical_marking = (lit->value == "true");
        }
        out.morphology[cp.name] = std::move(cp);
    }
}

void Canonicalizer::lower_ability(AbilityDecl const& ability, CanonicalEntity& out) {
    CanonicalAbility ca;
    ca.id = ability.name;
    ca.effect = "generic.projectile";
    ca.cost = 20;
    ca.cooldown_ticks = 10;
    ca.active_ticks = 4;
    for (auto const& child : ability.body) {
        auto const* attr = dynamic_cast<AttributeNode const*>(child.get());
        if (!attr || !attr->value) continue;
        auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get());
        if (!lit) continue;
        if (attr->key == "effect") ca.effect = lit->value;
        else if (attr->key == "cost") ca.cost = static_cast<std::uint32_t>(std::stoul(lit->value));
        else if (attr->key == "cooldown_ticks") ca.cooldown_ticks = static_cast<std::uint32_t>(std::stoul(lit->value));
        else if (attr->key == "active_ticks") ca.active_ticks = static_cast<std::uint32_t>(std::stoul(lit->value));
        else if (attr->key == "origin_socket") ca.origin_socket = lit->value;
        else if (attr->key == "speed_mm_per_tick") ca.speed_mm_per_tick = std::stod(lit->value);
        else if (attr->key == "collision_radius_mm") ca.collision_radius_mm = std::stod(lit->value);
        else if (attr->key == "status_id") ca.status_id = lit->value;
        else if (attr->key == "status_duration_ticks") ca.status_duration_ticks = static_cast<std::uint32_t>(std::stoul(lit->value));
    }
    out.abilities.push_back(std::move(ca));
}

void Canonicalizer::lower_entity(EntityDecl const& entity, CanonicalEntity& out) {
    out.stable_id = entity.name;
    out.name = entity.name;
    out.classification = "fictional";
    out.primary_color = "#112233";
    out.accent_color = "#AABBCC";

    for (auto const& child_ptr : entity.body) {
        if (auto const* f = dynamic_cast<FormDecl const*>(child_ptr.get())) lower_form(*f, out);
        else if (auto const* g = dynamic_cast<GeneDecl const*>(child_ptr.get())) lower_gene_decl(*g, out);
        else if (auto const* t = dynamic_cast<TransformationDecl const*>(child_ptr.get())) lower_transformation(*t, out);
        else if (auto const* m = dynamic_cast<MorphologyDecl const*>(child_ptr.get())) lower_morphology(*m, out);
        else if (auto const* a = dynamic_cast<AbilityDecl const*>(child_ptr.get())) lower_ability(*a, out);
        else if (auto const* ri = dynamic_cast<RightsDecl const*>(child_ptr.get())) lower_rights(*ri, out);
        else if (auto const* rs = dynamic_cast<ResourceDecl const*>(child_ptr.get())) lower_resource(*rs, out);
        else if (auto const* gb = dynamic_cast<GenericBlock const*>(child_ptr.get())) lower_generic_block(*gb, out);
    }
}

void Canonicalizer::lower_gene_decl(GeneDecl const&, CanonicalEntity&) {}

void Canonicalizer::lower_resource(ResourceDecl const& resource, CanonicalEntity& out) {
    CanonicalResource cr;
    cr.id = resource.name;
    cr.resource_type = resource.resource_type;
    for (auto const& child : resource.body) {
        auto const* attr = dynamic_cast<AttributeNode const*>(child.get());
        if (!attr || !attr->value) continue;
        auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get());
        if (!lit) continue;
        if (attr->key == "min") cr.min = static_cast<std::uint32_t>(std::stoul(lit->value));
        else if (attr->key == "max") cr.max = static_cast<std::uint32_t>(std::stoul(lit->value));
        else if (attr->key == "initial") cr.initial = static_cast<std::uint32_t>(std::stoul(lit->value));
    }
    out.resources.push_back(std::move(cr));
}

static std::string strip_quotes(std::string const& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return s.substr(1, s.size() - 2);
    return s;
}

static double parse_double(std::string const& s) {
    try { return std::stod(s); } catch (...) { return 0.0; }
}

static std::uint32_t parse_uint(std::string const& s) {
    try { return static_cast<std::uint32_t>(std::stoul(s)); } catch (...) { return 0; }
}

void Canonicalizer::lower_generic_block(GenericBlock const& block, CanonicalEntity& out) {
    auto get_attr = [&](std::string const& key) -> std::string {
        for (auto const& attr_ptr : block.attributes) {
            auto const* a = dynamic_cast<AttributeNode const*>(attr_ptr.get());
            if (a && a->key == key && a->value) {
                auto const* lit = dynamic_cast<LiteralNode const*>(a->value.get());
                if (lit) return strip_quotes(lit->value);
            }
        }
        return {};
    };

    if (block.block_type == "bone" || block.block_type == "Bone") {
        CanonicalSkeletalBone bone;
        bone.id = block.name;
        bone.parent = get_attr("parent");
        bone.x = parse_double(get_attr("x"));
        bone.y = parse_double(get_attr("y"));
        bone.z = parse_double(get_attr("z"));
        bone.scale_x = parse_double(get_attr("scale_x"));
        if (bone.scale_x == 0.0) bone.scale_x = 1.0;
        bone.scale_y = parse_double(get_attr("scale_y"));
        if (bone.scale_y == 0.0) bone.scale_y = 1.0;
        bone.length_mm = parse_double(get_attr("length_mm"));
        bone.min_rotation = parse_double(get_attr("min_rotation"));
        bone.max_rotation = parse_double(get_attr("max_rotation"));
        out.bones.push_back(std::move(bone));
    } else if (block.block_type == "socket" || block.block_type == "Socket") {
        CanonicalSocket socket;
        socket.id = block.name;
        socket.bone = get_attr("bone");
        socket.x = parse_double(get_attr("x"));
        socket.y = parse_double(get_attr("y"));
        socket.z = parse_double(get_attr("z"));
        socket.scale_x = parse_double(get_attr("scale_x"));
        if (socket.scale_x == 0.0) socket.scale_x = 1.0;
        socket.scale_y = parse_double(get_attr("scale_y"));
        if (socket.scale_y == 0.0) socket.scale_y = 1.0;
        out.sockets.push_back(std::move(socket));
    } else if (block.block_type == "clip" || block.block_type == "Clip") {
        CanonicalAnimationClip clip;
        clip.name = block.name;
        for (auto const& attr_ptr : block.attributes) {
            auto const* gb = dynamic_cast<GenericBlock const*>(attr_ptr.get());
            if (gb && (gb->block_type == "track" || gb->block_type == "Track")) {
                CanonicalAnimationClip::Track track;
                track.bone = gb->name;
                for (auto const& key_attr_ptr : gb->attributes) {
                    auto const* ka = dynamic_cast<AttributeNode const*>(key_attr_ptr.get());
                    if (ka && ka->key == "tick" && ka->value) {
                        auto const* kl = dynamic_cast<LiteralNode const*>(ka->value.get());
                        if (kl) {
                            auto tick = parse_uint(kl->value);
                            track.keys.push_back({tick, strip_quotes(get_attr("transform"))});
                        }
                    }
                }
                clip.tracks.push_back(std::move(track));
            }
        }
        out.clips.push_back(std::move(clip));
    } else if (block.block_type == "state" || block.block_type == "State") {
        CanonicalAnimationState state;
        state.name = block.name;
        state.clip_name = get_attr("clip");
        out.states.push_back(std::move(state));
    } else if (block.block_type == "transition" || block.block_type == "Transition") {
        CanonicalTransition transition;
        transition.from_state = block.name;
        transition.to_state = get_attr("to");
        transition.ability_id = get_attr("ability");
        transition.threshold = parse_uint(get_attr("threshold"));
        transition.resource_cost = parse_uint(get_attr("resource_cost"));
        transition.cooldown_ticks = parse_uint(get_attr("cooldown_ticks"));
        out.transitions.push_back(std::move(transition));
    } else if (block.block_type == "collision" || block.block_type == "Collision") {
        CanonicalCollisionShape shape;
        shape.id = block.name;
        shape.socket = get_attr("socket");
        shape.shape_type = get_attr("type");
        if (shape.shape_type.empty()) shape.shape_type = "CIRCLE";
        shape.radius_mm = parse_double(get_attr("radius_mm"));
        shape.offset_x = parse_double(get_attr("offset_x"));
        shape.offset_y = parse_double(get_attr("offset_y"));
        out.collision_shapes.push_back(std::move(shape));
    } else if (block.block_type == "window" || block.block_type == "Window") {
        CanonicalCollisionWindow window;
        window.ability_id = block.name;
        window.shape_id = get_attr("shape");
        window.start_tick = parse_uint(get_attr("start_tick"));
        window.duration_ticks = parse_uint(get_attr("duration_ticks"));
        window.active = (get_attr("active") != "false");
        out.collision_windows.push_back(std::move(window));
    } else if (block.block_type == "runtime" || block.block_type == "Runtime") {
        CanonicalRuntime rt;
        rt.aggression = parse_uint(get_attr("aggression"));
        rt.curiosity = parse_uint(get_attr("curiosity"));
        rt.energy = parse_uint(get_attr("energy"));
        rt.loyalty = parse_uint(get_attr("loyalty"));
        out.runtime = rt;
    } else if (block.block_type == "rig" || block.block_type == "Rig") {
        // Rig is a container for bones/sockets; nested blocks are already parsed as children
    }
}

void Canonicalizer::apply_genes(std::vector<GeneInstance> const& genes, CanonicalEntity& out) {
    for (auto const& g : genes) {
        if (g.descriptor.kind == GeneKind::identity) {
            auto it = g.values.find("stable_id");
            if (it != g.values.end()) out.stable_id = it->second;
            it = g.values.find("name");
            if (it != g.values.end()) out.name = it->second;
        }
        if (g.descriptor.kind == GeneKind::classification) {
            auto it = g.values.find("taxonomy");
            if (it != g.values.end()) out.classification = it->second;
        }
        if (g.descriptor.kind == GeneKind::appearance) {
            auto it = g.values.find("primary_color");
            if (it != g.values.end()) out.primary_color = it->second;
            it = g.values.find("accent_color");
            if (it != g.values.end()) out.accent_color = it->second;
        }
    }
    out.genes = genes;
}

CanonicalEntity Canonicalizer::lower(ModuleDecl const& module, std::vector<GeneInstance> const& genes) {
    CanonicalEntity entity;
    entity.stable_id = module.name;
    entity.name = module.name;

    for (auto const& decl : module.declarations) {
        if (auto const* e = dynamic_cast<EntityDecl const*>(decl.get())) lower_entity(*e, entity);
    }

    apply_genes(genes, entity);
    return entity;
}

} // namespace gspl
