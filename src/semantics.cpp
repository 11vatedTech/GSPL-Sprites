#include "gspl/semantics.hpp"
#include "gspl/ast.hpp"
#include "gspl/genes.hpp"
#include "gspl_sprites/core.hpp"
#include <algorithm>
#include <exception>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <string>

namespace gspl {
namespace {

std::string canonical_escape(std::string_view value) {
    std::ostringstream out;
    for (const unsigned char c : value) {
        switch (c) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned int>(c) << std::dec << std::setfill(' ');
            } else {
                out << static_cast<char>(c);
            }
        }
    }
    return out.str();
}

void append_key_value(std::ostringstream& out, std::string_view key, std::string_view value) {
    out << key << "=\"" << canonical_escape(value) << "\";";
}

void append_key_value(std::ostringstream& out, std::string_view key, std::string const& value) {
    append_key_value(out, key, std::string_view{value});
}

void append_key_value(std::ostringstream& out, std::string_view key, const char* value) {
    append_key_value(out, key, std::string_view{value});
}

void append_key_value(std::ostringstream& out, std::string_view key, bool value) {
    out << key << '=' << (value ? "true" : "false") << ';';
}

template <class T>
void append_key_value(std::ostringstream& out, std::string_view key, T value) {
    out << key << '=' << value << ';';
}

double gene_value_to_double(GeneValue const& value, double fallback = 0.0) {
    if (auto const* d = std::get_if<double>(&value)) return *d;
    if (auto const* i = std::get_if<std::int64_t>(&value)) return static_cast<double>(*i);
    if (auto const* u = std::get_if<std::uint64_t>(&value)) return static_cast<double>(*u);
    try { return std::stod(gene_value_to_string(value)); } catch (std::exception const&) { return fallback; }
}

bool gene_value_to_bool(GeneValue const& value, bool fallback = false) {
    if (auto const* b = std::get_if<bool>(&value)) return *b;
    const auto text = gene_value_to_string(value);
    if (text == "true") return true;
    if (text == "false") return false;
    return fallback;
}

void append_gene_identity_payload(std::ostringstream& out, std::vector<GeneInstance> const& genes) {
    out << "genes[" << genes.size() << "]{";
    for (auto const& gene : genes) {
        append_key_value(out, "kind", static_cast<std::uint32_t>(gene.descriptor.kind));
        append_key_value(out, "schema", gene.descriptor.schema_version);
        append_key_value(out, "type", gene.descriptor.type_id);
        append_key_value(out, "source", gene.source_module);
        out << "values{";
        std::map<std::string, std::string, std::less<>> sorted_values;
        for (auto const& [key, value] : gene.values) sorted_values.emplace(key, gene_value_to_string(value));
        for (auto const& [key, value] : sorted_values) append_key_value(out, key, value);
        out << "};";
    }
    out << "};";
}

std::string canonical_identity_payload(CanonicalEntity const& entity) {
    std::ostringstream out;
    append_key_value(out, "schema_version", entity.schema_version);
    append_key_value(out, "stable_id", entity.stable_id);
    append_key_value(out, "name", entity.name);
    append_key_value(out, "classification", entity.classification);
    append_key_value(out, "rights", entity.rights);
    append_key_value(out, "rights_allow_export", entity.rights_allow_export);
    append_key_value(out, "entropy_root", entity.entropy_root);
    append_key_value(out, "primary_color", entity.primary_color);
    append_key_value(out, "accent_color", entity.accent_color);
    append_key_value(out, "storm_primary_color", entity.storm_primary_color);
    append_key_value(out, "storm_accent_color", entity.storm_accent_color);
    append_key_value(out, "emissive_color", entity.emissive_color);
    append_key_value(out, "aura_color", entity.aura_color);
    append_key_value(out, "provenance_hash", entity.provenance_hash);
    append_key_value(out, "provenance_source", entity.provenance_source);
    append_gene_identity_payload(out, entity.genes);

    out << "forms[" << entity.forms.size() << "]{";
    for (auto const& form : entity.forms) {
        append_key_value(out, "id", form.id);
        append_key_value(out, "resource_capacity", form.resource_capacity);
        append_key_value(out, "collision_scale", form.collision_scale);
        append_key_value(out, "ability_envelope", form.ability_envelope);
        append_key_value(out, "max_health", form.max_health);
        out << "transformations{";
        for (auto const& id : form.transformation_ids) append_key_value(out, "id", id);
        out << "};";
    }
    out << "};";

    out << "transformations[" << entity.transformations.size() << "]{";
    for (auto const& transformation : entity.transformations) {
        append_key_value(out, "id", transformation.id);
        append_key_value(out, "from", transformation.from_form);
        append_key_value(out, "to", transformation.to_form);
        append_key_value(out, "trigger", transformation.trigger_condition);
        append_key_value(out, "duration", transformation.duration_ticks);
        append_key_value(out, "resource_cost", transformation.resource_cost);
    }
    out << "};";

    out << "morphology[" << entity.morphology.size() << "]{";
    for (auto const& [name, part] : entity.morphology) {
        append_key_value(out, "name", name);
        append_key_value(out, "parent", part.parent);
        append_key_value(out, "x", part.x);
        append_key_value(out, "y", part.y);
        append_key_value(out, "z", part.z);
        append_key_value(out, "size_x", part.size_x);
        append_key_value(out, "size_y", part.size_y);
        append_key_value(out, "size_z", part.size_z);
        append_key_value(out, "color", part.color);
        append_key_value(out, "rotation_degrees", part.rotation_degrees);
        append_key_value(out, "emissive", part.emissive);
        append_key_value(out, "electrical_marking", part.electrical_marking);
    }
    out << "};";

    auto append_abilities = [&](std::string_view label, std::vector<CanonicalAbility> const& abilities) {
        out << label << '[' << abilities.size() << "]{";
        for (auto const& ability : abilities) {
            append_key_value(out, "id", ability.id);
            append_key_value(out, "effect", ability.effect);
            append_key_value(out, "cost", ability.cost);
            append_key_value(out, "cooldown_ticks", ability.cooldown_ticks);
            append_key_value(out, "active_ticks", ability.active_ticks);
            append_key_value(out, "origin_socket", ability.origin_socket);
            append_key_value(out, "speed_mm_per_tick", ability.speed_mm_per_tick);
            append_key_value(out, "collision_radius_mm", ability.collision_radius_mm);
            append_key_value(out, "status_id", ability.status_id);
            append_key_value(out, "status_duration_ticks", ability.status_duration_ticks);
        }
        out << "};";
    };
    append_abilities("abilities", entity.abilities);
    append_abilities("storm_abilities", entity.storm_abilities);

    out << "bones[" << entity.bones.size() << "]{";
    for (auto const& bone : entity.bones) {
        append_key_value(out, "id", bone.id);
        append_key_value(out, "parent", bone.parent);
        append_key_value(out, "x", bone.x);
        append_key_value(out, "y", bone.y);
        append_key_value(out, "z", bone.z);
        append_key_value(out, "scale_x", bone.scale_x);
        append_key_value(out, "scale_y", bone.scale_y);
        append_key_value(out, "length_mm", bone.length_mm);
        append_key_value(out, "min_rotation", bone.min_rotation);
        append_key_value(out, "max_rotation", bone.max_rotation);
    }
    out << "};";

    out << "sockets[" << entity.sockets.size() << "]{";
    for (auto const& socket : entity.sockets) {
        append_key_value(out, "id", socket.id);
        append_key_value(out, "bone", socket.bone);
        append_key_value(out, "x", socket.x);
        append_key_value(out, "y", socket.y);
        append_key_value(out, "z", socket.z);
        append_key_value(out, "scale_x", socket.scale_x);
        append_key_value(out, "scale_y", socket.scale_y);
    }
    out << "};";

    out << "clips[" << entity.clips.size() << "]{";
    for (auto const& clip : entity.clips) {
        append_key_value(out, "name", clip.name);
        append_key_value(out, "loop", clip.loop);
        out << "tracks[" << clip.tracks.size() << "]{";
        for (auto const& track : clip.tracks) {
            append_key_value(out, "bone", track.bone);
            out << "keys{";
            for (auto const& [tick, value] : track.keys) {
                append_key_value(out, "tick", tick);
                append_key_value(out, "value", value);
            }
            out << "};";
        }
        out << "};events{";
        for (auto const& [tick, id] : clip.clip_events) {
            append_key_value(out, "tick", tick);
            append_key_value(out, "id", id);
        }
        out << "};";
    }
    out << "};";

    out << "states[" << entity.states.size() << "]{";
    append_key_value(out, "initial_state", entity.initial_state);
    for (auto const& state : entity.states) {
        append_key_value(out, "name", state.name);
        append_key_value(out, "clip", state.clip_name);
    }
    out << "};";

    out << "transitions[" << entity.transitions.size() << "]{";
    for (auto const& transition : entity.transitions) {
        append_key_value(out, "from", transition.from_state);
        append_key_value(out, "to", transition.to_state);
        append_key_value(out, "ability", transition.ability_id);
        append_key_value(out, "comparison", transition.comparison);
        append_key_value(out, "threshold", transition.threshold);
        append_key_value(out, "resource_cost", transition.resource_cost);
        append_key_value(out, "cooldown_ticks", transition.cooldown_ticks);
    }
    out << "};";

    out << "collision_shapes[" << entity.collision_shapes.size() << "]{";
    for (auto const& shape : entity.collision_shapes) {
        append_key_value(out, "id", shape.id);
        append_key_value(out, "type", shape.shape_type);
        append_key_value(out, "socket", shape.socket);
        append_key_value(out, "radius_mm", shape.radius_mm);
        append_key_value(out, "offset_x", shape.offset_x);
        append_key_value(out, "offset_y", shape.offset_y);
        append_key_value(out, "scale_x", shape.scale_x);
        append_key_value(out, "scale_y", shape.scale_y);
    }
    out << "};collision_windows[" << entity.collision_windows.size() << "]{";
    for (auto const& window : entity.collision_windows) {
        append_key_value(out, "ability", window.ability_id);
        append_key_value(out, "shape", window.shape_id);
        append_key_value(out, "start_tick", window.start_tick);
        append_key_value(out, "duration_ticks", window.duration_ticks);
        append_key_value(out, "active", window.active);
    }
    out << "};resources[" << entity.resources.size() << "]{";
    for (auto const& resource : entity.resources) {
        append_key_value(out, "id", resource.id);
        append_key_value(out, "type", resource.resource_type);
        append_key_value(out, "min", resource.min);
        append_key_value(out, "max", resource.max);
        append_key_value(out, "initial", resource.initial);
    }
    out << "};";

    if (entity.runtime) {
        out << "runtime{";
        append_key_value(out, "aggression", entity.runtime->aggression);
        append_key_value(out, "curiosity", entity.runtime->curiosity);
        append_key_value(out, "energy", entity.runtime->energy);
        append_key_value(out, "loyalty", entity.runtime->loyalty);
        out << "animation_intents{";
        for (auto const& intent : entity.runtime->animation_intents) {
            append_key_value(out, "behavior", intent.behavior_state);
            append_key_value(out, "clip", intent.clip_name);
        }
        out << "};};";
    }

    return out.str();
}

} // namespace

// ── CanonicalEntitySerializer ──────────────────────────────────────
std::string CanonicalEntitySerializer::to_json(CanonicalEntity const& entity) {
    std::ostringstream os;
    os << "{\n";
    os << "  \"schema_version\": \"" << entity.schema_version << "\",\n";
    os << "  \"stable_id\": \"" << canonical_escape(entity.stable_id) << "\",\n";
    os << "  \"name\": \"" << canonical_escape(entity.name) << "\",\n";
    os << "  \"classification\": \"" << canonical_escape(entity.classification) << "\",\n";
    os << "  \"rights\": \"" << canonical_escape(entity.rights) << "\",\n";
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
    // forms
    os << "  \"forms\": [";
    for (std::size_t i = 0; i < entity.forms.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& f = entity.forms[i];
        os << "{\"id\":\"" << canonical_escape(f.id) << "\""
           << ",\"resource_capacity\":" << f.resource_capacity
           << ",\"collision_scale\":" << f.collision_scale
           << ",\"ability_envelope\":" << f.ability_envelope
           << ",\"max_health\":" << f.max_health
           << ",\"transformation_ids\":[";
        for (std::size_t j = 0; j < f.transformation_ids.size(); ++j) {
            if (j > 0) os << ",";
            os << "\"" << canonical_escape(f.transformation_ids[j]) << "\"";
        }
        os << "]}";
    }
    os << "],\n";
    // transformations
    os << "  \"transformations\": [";
    for (std::size_t i = 0; i < entity.transformations.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& t = entity.transformations[i];
        os << "{\"id\":\"" << canonical_escape(t.id) << "\""
           << ",\"from_form\":\"" << canonical_escape(t.from_form) << "\""
           << ",\"to_form\":\"" << canonical_escape(t.to_form) << "\""
           << ",\"trigger_condition\":\"" << canonical_escape(t.trigger_condition) << "\""
           << ",\"duration_ticks\":" << t.duration_ticks
           << ",\"resource_cost\":" << t.resource_cost << "}";
    }
    os << "],\n";
    // morphology
    os << "  \"morphology\": {";
    bool first_morph = true;
    for (auto const& [name, part] : entity.morphology) {
        if (!first_morph) os << ", ";
        first_morph = false;
        os << "\"" << canonical_escape(name) << "\":{"
           << "\"parent\":\"" << canonical_escape(part.parent) << "\""
           << ",\"x\":" << part.x << ",\"y\":" << part.y << ",\"z\":" << part.z
           << ",\"size_x\":" << part.size_x << ",\"size_y\":" << part.size_y << ",\"size_z\":" << part.size_z
           << ",\"color\":\"" << canonical_escape(part.color) << "\""
           << ",\"rotation_degrees\":" << part.rotation_degrees
           << ",\"emissive\":" << (part.emissive ? "true" : "false")
           << ",\"electrical_marking\":" << (part.electrical_marking ? "true" : "false")
           << "}";
    }
    os << "},\n";
    // abilities
    auto append_abilities_json = [&](std::string_view key, std::vector<CanonicalAbility> const& abilities) {
        os << "  \"" << key << "\": [";
        for (std::size_t i = 0; i < abilities.size(); ++i) {
            if (i > 0) os << ", ";
            auto const& a = abilities[i];
            os << "{\"id\":\"" << canonical_escape(a.id) << "\""
               << ",\"effect\":\"" << canonical_escape(a.effect) << "\""
               << ",\"cost\":" << a.cost
               << ",\"cooldown_ticks\":" << a.cooldown_ticks
               << ",\"active_ticks\":" << a.active_ticks
               << ",\"origin_socket\":\"" << canonical_escape(a.origin_socket) << "\""
               << ",\"speed_mm_per_tick\":" << a.speed_mm_per_tick
               << ",\"collision_radius_mm\":" << a.collision_radius_mm
               << ",\"status_id\":\"" << canonical_escape(a.status_id) << "\""
               << ",\"status_duration_ticks\":" << a.status_duration_ticks << "}";
        }
        os << "],\n";
    };
    append_abilities_json("abilities", entity.abilities);
    append_abilities_json("storm_abilities", entity.storm_abilities);
    // bones
    os << "  \"bones\": [";
    for (std::size_t i = 0; i < entity.bones.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& b = entity.bones[i];
        os << "{\"id\":\"" << canonical_escape(b.id) << "\""
           << ",\"parent\":\"" << canonical_escape(b.parent) << "\""
           << ",\"x\":" << b.x << ",\"y\":" << b.y << ",\"z\":" << b.z
           << ",\"scale_x\":" << b.scale_x << ",\"scale_y\":" << b.scale_y
           << ",\"length_mm\":" << b.length_mm
           << ",\"min_rotation\":" << b.min_rotation
           << ",\"max_rotation\":" << b.max_rotation << "}";
    }
    os << "],\n";
    // sockets
    os << "  \"sockets\": [";
    for (std::size_t i = 0; i < entity.sockets.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& s = entity.sockets[i];
        os << "{\"id\":\"" << canonical_escape(s.id) << "\""
           << ",\"bone\":\"" << canonical_escape(s.bone) << "\""
           << ",\"x\":" << s.x << ",\"y\":" << s.y << ",\"z\":" << s.z
           << ",\"scale_x\":" << s.scale_x << ",\"scale_y\":" << s.scale_y << "}";
    }
    os << "],\n";
    // clips
    os << "  \"clips\": [";
    for (std::size_t i = 0; i < entity.clips.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& c = entity.clips[i];
        os << "{\"name\":\"" << canonical_escape(c.name) << "\""
           << ",\"loop\":" << (c.loop ? "true" : "false")
           << ",\"tracks\":[";
        for (std::size_t j = 0; j < c.tracks.size(); ++j) {
            if (j > 0) os << ",";
            auto const& tr = c.tracks[j];
            os << "{\"bone\":\"" << canonical_escape(tr.bone) << "\",\"keys\":[";
            for (std::size_t k = 0; k < tr.keys.size(); ++k) {
                if (k > 0) os << ",";
                os << "{\"tick\":" << tr.keys[k].first
                   << ",\"value\":\"" << canonical_escape(tr.keys[k].second) << "\"}";
            }
            os << "]}";
        }
        os << "],\"events\":[";
        for (std::size_t j = 0; j < c.clip_events.size(); ++j) {
            if (j > 0) os << ",";
            os << "{\"tick\":" << c.clip_events[j].first
               << ",\"id\":\"" << canonical_escape(c.clip_events[j].second) << "\"}";
        }
        os << "]}";
    }
    os << "],\n";
    // states
    os << "  \"states\": [";
    for (std::size_t i = 0; i < entity.states.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& s = entity.states[i];
        os << "{\"name\":\"" << canonical_escape(s.name) << "\""
           << ",\"clip_name\":\"" << canonical_escape(s.clip_name) << "\"}";
    }
    os << "],\n";
    os << "  \"initial_state\": \"" << canonical_escape(entity.initial_state) << "\",\n";
    // transitions
    os << "  \"transitions\": [";
    for (std::size_t i = 0; i < entity.transitions.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& t = entity.transitions[i];
        os << "{\"from_state\":\"" << canonical_escape(t.from_state) << "\""
           << ",\"to_state\":\"" << canonical_escape(t.to_state) << "\""
           << ",\"ability_id\":\"" << canonical_escape(t.ability_id) << "\""
           << ",\"comparison\":\"" << canonical_escape(t.comparison) << "\""
           << ",\"threshold\":" << t.threshold
           << ",\"resource_cost\":" << t.resource_cost
           << ",\"cooldown_ticks\":" << t.cooldown_ticks << "}";
    }
    os << "],\n";
    // collision shapes
    os << "  \"collision_shapes\": [";
    for (std::size_t i = 0; i < entity.collision_shapes.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& cs = entity.collision_shapes[i];
        os << "{\"id\":\"" << canonical_escape(cs.id) << "\""
           << ",\"shape_type\":\"" << canonical_escape(cs.shape_type) << "\""
           << ",\"socket\":\"" << canonical_escape(cs.socket) << "\""
           << ",\"radius_mm\":" << cs.radius_mm
           << ",\"offset_x\":" << cs.offset_x << ",\"offset_y\":" << cs.offset_y
           << ",\"scale_x\":" << cs.scale_x << ",\"scale_y\":" << cs.scale_y << "}";
    }
    os << "],\n";
    // collision windows
    os << "  \"collision_windows\": [";
    for (std::size_t i = 0; i < entity.collision_windows.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& cw = entity.collision_windows[i];
        os << "{\"ability_id\":\"" << canonical_escape(cw.ability_id) << "\""
           << ",\"shape_id\":\"" << canonical_escape(cw.shape_id) << "\""
           << ",\"start_tick\":" << cw.start_tick
           << ",\"duration_ticks\":" << cw.duration_ticks
           << ",\"active\":" << (cw.active ? "true" : "false") << "}";
    }
    os << "],\n";
    // resources
    os << "  \"resources\": [";
    for (std::size_t i = 0; i < entity.resources.size(); ++i) {
        if (i > 0) os << ", ";
        auto const& r = entity.resources[i];
        os << "{\"id\":\"" << canonical_escape(r.id) << "\""
           << ",\"resource_type\":\"" << canonical_escape(r.resource_type) << "\""
           << ",\"min\":" << r.min << ",\"max\":" << r.max
           << ",\"initial\":" << r.initial << "}";
    }
    os << "],\n";
    // runtime
    if (entity.runtime) {
        os << "  \"runtime\": {"
           << "\"aggression\":" << entity.runtime->aggression
           << ",\"curiosity\":" << entity.runtime->curiosity
           << ",\"energy\":" << entity.runtime->energy
           << ",\"loyalty\":" << entity.runtime->loyalty
           << ",\"animation_intents\":[";
        for (std::size_t i = 0; i < entity.runtime->animation_intents.size(); ++i) {
            if (i > 0) os << ",";
            os << "{\"behavior_state\":\"" << canonical_escape(entity.runtime->animation_intents[i].behavior_state) << "\""
               << ",\"clip_name\":\"" << canonical_escape(entity.runtime->animation_intents[i].clip_name) << "\"}";
        }
        os << "]},\n";
    }
    // genes (count only — full gene data is in canonical_identity_payload)
    os << "  \"gene_count\": " << entity.genes.size() << "\n";
    os << "}";
    return os.str();
}

std::optional<CanonicalEntity> CanonicalEntitySerializer::from_json(
    std::string_view json, DiagnosticResult& diag) {
    if (json.empty()) {
        diag.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: empty input", {});
        return std::nullopt;
    }

    // minimal bounded JSON key-value parser for canonical entity format
    auto skip_ws = [&](std::size_t& pos) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' ||
               json[pos] == '\r' || json[pos] == '\t')) ++pos;
    };

    auto read_string = [&](std::size_t& pos) -> std::string {
        skip_ws(pos);
        if (pos >= json.size() || json[pos] != '"') return {};
        ++pos;
        std::string out;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                ++pos;
                switch (json[pos]) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                default: out += json[pos]; break;
                }
            } else {
                out += json[pos];
            }
            ++pos;
        }
        if (pos < json.size()) ++pos;
        return out;
    };

    auto read_bool = [&](std::size_t& pos) -> bool {
        skip_ws(pos);
        if (pos + 4 <= json.size() && json.substr(pos, 4) == "true") { pos += 4; return true; }
        if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") { pos += 5; return false; }
        return false;
    };

    auto read_uint64 = [&](std::size_t& pos) -> std::uint64_t {
        skip_ws(pos);
        std::string num;
        while (pos < json.size() && std::isdigit(static_cast<unsigned char>(json[pos]))) {
            num += json[pos];
            ++pos;
        }
        if (num.empty()) return 0;
        std::uint64_t v{};
        auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), v);
        if (ec != std::errc{}) return 0;
        return v;
    };

    std::size_t pos = 0;
    skip_ws(pos);
    if (pos >= json.size() || json[pos] != '{') {
        diag.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: expected '{'", {});
        return std::nullopt;
    }
    ++pos;

    CanonicalEntity ce;
    bool parsed_any = false;

    while (pos < json.size()) {
        skip_ws(pos);
        if (pos >= json.size()) break;
        if (json[pos] == '}') { ++pos; break; }
        if (json[pos] == ',') { ++pos; continue; }

        auto key = read_string(pos);
        if (key.empty()) break;
        skip_ws(pos);
        if (pos >= json.size() || json[pos] != ':') break;
        ++pos; // skip ':'

        parsed_any = true;

        if (key == "schema_version") {
            ce.schema_version = read_string(pos);
        } else if (key == "stable_id") {
            ce.stable_id = read_string(pos);
        } else if (key == "name") {
            ce.name = read_string(pos);
        } else if (key == "classification") {
            ce.classification = read_string(pos);
        } else if (key == "rights") {
            ce.rights = read_string(pos);
        } else if (key == "rights_allow_export") {
            skip_ws(pos);
            ce.rights_allow_export = read_bool(pos);
        } else if (key == "entropy_root") {
            ce.entropy_root = read_uint64(pos);
        } else if (key == "primary_color") {
            ce.primary_color = read_string(pos);
        } else if (key == "accent_color") {
            ce.accent_color = read_string(pos);
        } else if (key == "storm_primary_color") {
            ce.storm_primary_color = read_string(pos);
        } else if (key == "storm_accent_color") {
            ce.storm_accent_color = read_string(pos);
        } else if (key == "emissive_color") {
            ce.emissive_color = read_string(pos);
        } else if (key == "aura_color") {
            ce.aura_color = read_string(pos);
        } else if (key == "provenance_hash") {
            ce.provenance_hash = read_string(pos);
        } else if (key == "provenance_source") {
            ce.provenance_source = read_string(pos);
        } else {
            // skip unknown values (counts, arrays, nested objects)
            skip_ws(pos);
            if (pos < json.size()) {
                if (json[pos] == '"') { read_string(pos); }
                else if (json[pos] == '{') {
                    int depth = 1; ++pos;
                    while (pos < json.size() && depth > 0) {
                        if (json[pos] == '{') ++depth;
                        else if (json[pos] == '}') --depth;
                        else if (json[pos] == '"') read_string(pos);
                        else ++pos;
                    }
                } else if (json[pos] == '[') {
                    int depth = 1; ++pos;
                    while (pos < json.size() && depth > 0) {
                        if (json[pos] == '[') ++depth;
                        else if (json[pos] == ']') --depth;
                        else if (json[pos] == '"') read_string(pos);
                        else ++pos;
                    }
                } else {
                    while (pos < json.size() && json[pos] != ',' && json[pos] != '}' &&
                           json[pos] != '\n') ++pos;
                }
            }
        }
    }

    if (!parsed_any) {
        diag.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: no fields parsed from input", {});
        return std::nullopt;
    }

    // Validate required identity fields
    if (ce.stable_id.empty()) {
        diag.add_error(DiagnosticCode::GSPL_NAME_UNKNOWN,
                       "from_json: deserialized entity missing required stable_id", {});
        return std::nullopt;
    }
    if (ce.name.empty()) {
        ce.name = ce.stable_id; // fallback: name defaults to stable_id
    }
    if (ce.rights.empty()) {
        ce.rights = "UNCLASSIFIED";
    }

    return ce;
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
    serialized_ = canonical_identity_payload(entity);
    hash_ = gspl::sprites::sha256(serialized_);
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

void Canonicalizer::lower_gene_decl(GeneDecl const&, CanonicalEntity&) {
    // Gene declarations are collected and validated by GeneCompositionPhase, then
    // applied as typed GeneInstance values in apply_genes(). Keeping this method
    // side-effect free prevents duplicate lowering when entity traversal sees the
    // original AST declarations.
}

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
            if (it != g.values.end()) out.stable_id = gene_value_to_string(it->second);
            it = g.values.find("name");
            if (it != g.values.end()) out.name = gene_value_to_string(it->second);
        }
        if (g.descriptor.kind == GeneKind::classification) {
            auto it = g.values.find("taxonomy");
            if (it != g.values.end()) out.classification = gene_value_to_string(it->second);
            it = g.values.find("classification");
            if (it != g.values.end()) out.classification = gene_value_to_string(it->second);
        }
        if (g.descriptor.kind == GeneKind::appearance) {
            auto it = g.values.find("primary_color");
            if (it != g.values.end()) out.primary_color = gene_value_to_string(it->second);
            it = g.values.find("accent_color");
            if (it != g.values.end()) out.accent_color = gene_value_to_string(it->second);
            it = g.values.find("storm_primary_color");
            if (it != g.values.end()) out.storm_primary_color = gene_value_to_string(it->second);
            it = g.values.find("storm_accent_color");
            if (it != g.values.end()) out.storm_accent_color = gene_value_to_string(it->second);
            it = g.values.find("emissive_color");
            if (it != g.values.end()) out.emissive_color = gene_value_to_string(it->second);
            it = g.values.find("aura_color");
            if (it != g.values.end()) out.aura_color = gene_value_to_string(it->second);
        }
        if (g.descriptor.kind == GeneKind::morphology) {
            auto part_it = g.values.find("part");
            if (part_it != g.values.end()) {
                CanonicalPart part;
                part.name = gene_value_to_string(part_it->second);
                if (auto it = g.values.find("parent"); it != g.values.end()) part.parent = gene_value_to_string(it->second);
                if (auto it = g.values.find("x"); it != g.values.end()) part.x = gene_value_to_double(it->second);
                if (auto it = g.values.find("y"); it != g.values.end()) part.y = gene_value_to_double(it->second);
                if (auto it = g.values.find("z"); it != g.values.end()) part.z = gene_value_to_double(it->second);
                if (auto it = g.values.find("size_x"); it != g.values.end()) part.size_x = gene_value_to_double(it->second, 1.0);
                if (auto it = g.values.find("size_y"); it != g.values.end()) part.size_y = gene_value_to_double(it->second, 1.0);
                if (auto it = g.values.find("size_z"); it != g.values.end()) part.size_z = gene_value_to_double(it->second, 1.0);
                if (auto it = g.values.find("color"); it != g.values.end()) part.color = gene_value_to_string(it->second);
                if (auto it = g.values.find("rotation_degrees"); it != g.values.end()) part.rotation_degrees = gene_value_to_double(it->second);
                if (auto it = g.values.find("emissive"); it != g.values.end()) part.emissive = gene_value_to_bool(it->second);
                if (auto it = g.values.find("electrical_marking"); it != g.values.end()) part.electrical_marking = gene_value_to_bool(it->second);
                out.morphology[part.name] = std::move(part);
            }
        }
        if (g.descriptor.kind == GeneKind::rights) {
            auto it = g.values.find("classification");
            if (it != g.values.end()) {
                out.rights = gene_value_to_string(it->second);
                out.rights_allow_export = out.rights.find("PROHIBITED") == std::string::npos &&
                    out.rights.find("RESEARCH_ONLY") == std::string::npos;
            }
            it = g.values.find("allow_export");
            if (it != g.values.end()) {
                if (auto const* b = std::get_if<bool>(&it->second)) out.rights_allow_export = *b;
            }
        }
        if (g.descriptor.kind == GeneKind::provenance) {
            auto it = g.values.find("hash");
            if (it != g.values.end()) out.provenance_hash = gene_value_to_string(it->second);
            it = g.values.find("source");
            if (it != g.values.end()) out.provenance_source = gene_value_to_string(it->second);
        }
        if (g.descriptor.kind == GeneKind::optimization) {
            auto it = g.values.find("entropy_root");
            if (it != g.values.end()) {
                if (auto const* u = std::get_if<std::uint64_t>(&it->second)) out.entropy_root = *u;
                else if (auto const* s = std::get_if<std::int64_t>(&it->second); s != nullptr && *s >= 0) out.entropy_root = static_cast<std::uint64_t>(*s);
                else out.entropy_root = static_cast<std::uint64_t>(std::stoull(gene_value_to_string(it->second)));
            }
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
