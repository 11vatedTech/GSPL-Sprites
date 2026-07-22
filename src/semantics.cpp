#include "gspl/semantics.hpp"
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
    out.transformations.push_back(std::move(ct));
}

void Canonicalizer::lower_morphology(MorphologyDecl const& morph, CanonicalEntity& out) {
    for (auto const& child : morph.parts) {
        (void)child;
        CanonicalPart cp;
        cp.name = "part_" + std::to_string(out.morphology.size() + 1);
        cp.color = "#888888";
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
        else if (auto const* r = dynamic_cast<RightsDecl const*>(child_ptr.get())) lower_rights(*r, out);
    }
}

void Canonicalizer::lower_gene_decl(GeneDecl const&, CanonicalEntity&) {}
void Canonicalizer::lower_resource(ResourceDecl const&, CanonicalEntity&) {}

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
