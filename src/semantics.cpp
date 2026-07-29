#include "gspl/semantics.hpp"
#include "gspl/ast.hpp"
#include "gspl/genes.hpp"
#include "gspl/json.hpp"
#include "gspl_sprites/core.hpp"
#include <algorithm>
#include <charconv>
#include <exception>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <stdexcept>
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
    out << "};form_morphology_overrides[" << entity.form_morphology_overrides.size() << "]{";
    for (auto const& [form_name, parts] : entity.form_morphology_overrides) {
        append_key_value(out, "form", form_name);
        out << "parts{";
        for (auto const& [part_name, part] : parts) {
            append_key_value(out, "name", part_name);
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

// ── JSON path helper ─────────────────────────────────────

static std::string json_path(std::string const& prefix, std::string_view field) {
    if (prefix.empty()) return std::string(field);
    return prefix + "." + std::string(field);
}

static std::string json_index_path(std::string const& prefix, std::size_t index) {
    return prefix + "[" + std::to_string(index) + "]";
}

// ── Array decoding helper ────────────────────────────────

template <typename T, typename Codec>
static JsonReadResult<std::vector<T>> decode_array(
    BoundedJsonReader& r, std::string const& path, Codec codec) {
    JsonReadResult<std::vector<T>> res;
    res.value.emplace();
    r.require('[', path + ": expected '['");
    if (r.has_error()) return res;

    std::size_t idx = 0;
    while (r.has_more() && !r.has_error()) {
        // Reject trailing comma: after consuming comma, next non-ws char != ]
        r.skip_ws();
        if (r.position() < r.source().size() && r.source()[r.position()] == ']') {
            if (idx > 0) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                    path + ": trailing comma in array", {});
                return res;
            }
            break;  // empty array, not a trailing comma
        }
        auto item_path = json_index_path(path, idx);
        auto item_res = codec(r, item_path);
        if (!item_res.ok()) {
            res.diagnostics.merge(item_res.diagnostics);
            return res;
        }
        if (item_res.value) res.value->push_back(std::move(*item_res.value));
        ++idx;
        if (!r.consume_if(',')) break;
    }
    r.require(']', path + ": expected ']'");
    return res;
}

// Specialization for string arrays
static JsonReadResult<std::vector<std::string>> decode_string_array(
    BoundedJsonReader& r, std::string const& path) {
    return decode_array<std::string>(r, path,
        [](BoundedJsonReader& rr, std::string const& /*path*/) {
            JsonReadResult<std::string> sr;
            auto s = rr.read_string_result();
            if (!s.ok()) { sr.diagnostics = s.diagnostics; return sr; }
            sr.value = std::move(*s.value);
            return sr;
        });
}

// ── Scalar helpers (result-returning, checked) ──────────

static JsonReadResult<std::string> decode_string_field(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<std::string> result;
    auto res = r.read_string_result();
    if (!res.ok()) {
        result.diagnostics = std::move(res.diagnostics);
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
            path + ": expected string", {});
        return result;
    }
    result.value = std::move(*res.value);
    return result;
}

static JsonReadResult<bool> decode_bool_field(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<bool> result;
    auto res = r.read_bool_result();
    if (!res.ok() || !res.value) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
            path + ": expected boolean", {});
        return result;
    }
    result.value = *res.value;
    return result;
}

static JsonReadResult<std::uint32_t> decode_uint32_field(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<std::uint32_t> result;
    auto res = r.read_uint64_result();
    if (!res.ok() || !res.value) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
            path + ": expected unsigned integer", {});
        return result;
    }
    if (*res.value > static_cast<std::uint64_t>(
            std::numeric_limits<std::uint32_t>::max())) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED,
            path + ": uint32 overflow (" + std::to_string(*res.value) + " > "
            + std::to_string(std::numeric_limits<std::uint32_t>::max()) + ")", {});
        return result;
    }
    result.value = static_cast<std::uint32_t>(*res.value);
    return result;
}

static JsonReadResult<double> decode_double_field(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<double> result;
    auto res = r.read_double_result();
    if (!res.ok() || !res.value) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
            path + ": expected number", {});
        return result;
    }
    double val = *res.value;
    if (!std::isfinite(val)) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
            path + ": non-finite double value not allowed", {});
        return result;
    }
    result.value = val;
    return result;
}

// ── Thin adapters (old API on top of checked result helpers) ───

static bool decode_string_field(BoundedJsonReader& r, std::string& out,
                                std::string const& path) {
    auto res = decode_string_field(r, path);
    if (!res.ok() || !res.value) return false;
    out = std::move(*res.value);
    return true;
}

static bool decode_bool_field(BoundedJsonReader& r, bool& out,
                              std::string const& path = "") {
    auto res = decode_bool_field(r, path);
    if (!res.ok() || !res.value) return false;
    out = *res.value;
    return true;
}

static bool decode_uint32_field(BoundedJsonReader& r, std::uint32_t& out,
                                std::string const& path = "") {
    auto res = decode_uint32_field(r, path);
    if (!res.ok() || !res.value) return false;
    out = *res.value;
    return true;
}

static bool decode_double_field(BoundedJsonReader& r, double& out,
                                std::string const& path = "") {
    auto res = decode_double_field(r, path);
    if (!res.ok() || !res.value) return false;
    out = *res.value;
    return true;
}

// ── Object entry reading helper ─────────────────────────

static bool read_object_entry(BoundedJsonReader& r, std::string& key,
                              std::string const& path) {
    auto key_res = r.read_string_result();
    if (!key_res.ok() || !key_res.value) return false;
    key = std::move(*key_res.value);
    if (!r.require(':', json_path(path, key) + ": expected ':'")) return false;
    return !r.has_error();
}

// ── Type codecs ──────────────────────────────────────────

static JsonReadResult<CanonicalPart> decode_canonical_part(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalPart> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "parent")      decode_string_field(r, res.value->parent, fp);
        else if (key == "x")      decode_double_field(r, res.value->x);
        else if (key == "y")      decode_double_field(r, res.value->y);
        else if (key == "z")      decode_double_field(r, res.value->z);
        else if (key == "size_x") decode_double_field(r, res.value->size_x);
        else if (key == "size_y") decode_double_field(r, res.value->size_y);
        else if (key == "size_z") decode_double_field(r, res.value->size_z);
        else if (key == "color")  decode_string_field(r, res.value->color, fp);
        else if (key == "rotation_degrees") decode_double_field(r, res.value->rotation_degrees);
        else if (key == "emissive") decode_bool_field(r, res.value->emissive);
        else if (key == "electrical_marking") decode_bool_field(r, res.value->electrical_marking);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalForm> decode_canonical_form(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalForm> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id") decode_string_field(r, res.value->id, fp);
        else if (key == "resource_capacity") decode_uint32_field(r, res.value->resource_capacity);
        else if (key == "collision_scale") decode_double_field(r, res.value->collision_scale);
        else if (key == "ability_envelope") decode_double_field(r, res.value->ability_envelope);
        else if (key == "max_health") decode_uint32_field(r, res.value->max_health);
        else if (key == "transformation_ids") {
            auto arr = decode_string_array(r, fp);
            if (arr.ok() && arr.value) res.value->transformation_ids = std::move(*arr.value);
            else { res.diagnostics.merge(arr.diagnostics); return res; }
        } else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalTransformation> decode_transformation(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalTransformation> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id")               decode_string_field(r, res.value->id, fp);
        else if (key == "from_form")   decode_string_field(r, res.value->from_form, fp);
        else if (key == "to_form")     decode_string_field(r, res.value->to_form, fp);
        else if (key == "trigger_condition") decode_string_field(r, res.value->trigger_condition, fp);
        else if (key == "duration_ticks") decode_uint32_field(r, res.value->duration_ticks);
        else if (key == "resource_cost") decode_uint32_field(r, res.value->resource_cost);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalAbility> decode_ability(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalAbility> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id")             decode_string_field(r, res.value->id, fp);
        else if (key == "effect")    decode_string_field(r, res.value->effect, fp);
        else if (key == "cost")      decode_uint32_field(r, res.value->cost);
        else if (key == "cooldown_ticks") decode_uint32_field(r, res.value->cooldown_ticks);
        else if (key == "active_ticks") decode_uint32_field(r, res.value->active_ticks);
        else if (key == "origin_socket") decode_string_field(r, res.value->origin_socket, fp);
        else if (key == "speed_mm_per_tick") decode_double_field(r, res.value->speed_mm_per_tick);
        else if (key == "collision_radius_mm") decode_double_field(r, res.value->collision_radius_mm);
        else if (key == "status_id") decode_string_field(r, res.value->status_id, fp);
        else if (key == "status_duration_ticks") decode_uint32_field(r, res.value->status_duration_ticks);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalSkeletalBone> decode_bone(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalSkeletalBone> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id")       decode_string_field(r, res.value->id, fp);
        else if (key == "parent") decode_string_field(r, res.value->parent, fp);
        else if (key == "x")    decode_double_field(r, res.value->x);
        else if (key == "y")    decode_double_field(r, res.value->y);
        else if (key == "z")    decode_double_field(r, res.value->z);
        else if (key == "scale_x") decode_double_field(r, res.value->scale_x);
        else if (key == "scale_y") decode_double_field(r, res.value->scale_y);
        else if (key == "length_mm") decode_double_field(r, res.value->length_mm);
        else if (key == "min_rotation") decode_double_field(r, res.value->min_rotation);
        else if (key == "max_rotation") decode_double_field(r, res.value->max_rotation);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalSocket> decode_socket(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalSocket> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id")   decode_string_field(r, res.value->id, fp);
        else if (key == "bone") decode_string_field(r, res.value->bone, fp);
        else if (key == "x") decode_double_field(r, res.value->x);
        else if (key == "y") decode_double_field(r, res.value->y);
        else if (key == "z") decode_double_field(r, res.value->z);
        else if (key == "scale_x") decode_double_field(r, res.value->scale_x);
        else if (key == "scale_y") decode_double_field(r, res.value->scale_y);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalAnimationClip> decode_animation_clip(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalAnimationClip> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "name") decode_string_field(r, res.value->name, fp);
        else if (key == "loop") decode_bool_field(r, res.value->loop);
        else if (key == "tracks") {
            auto tracks = decode_array<CanonicalAnimationClip::Track>(r, fp,
                [](BoundedJsonReader& rr, std::string const& tp) {
                    JsonReadResult<CanonicalAnimationClip::Track> tr;
                    tr.value.emplace();
                    rr.require('{', tp + ": expected '{'");
                    if (rr.has_error()) return tr;
                    while (rr.has_more() && !rr.has_error()) {
                        std::string tk;
                        if (!read_object_entry(rr, tk, tp)) break;
                        auto tfp = json_path(tp, tk);
                        if (tk == "bone") decode_string_field(rr, tr.value->bone, tfp);
                        else if (tk == "keys") {
                            auto keys = decode_array<std::pair<std::uint32_t, std::string>>(rr, tfp,
                                [](BoundedJsonReader& kr, std::string const& kp) {
                                    JsonReadResult<std::pair<std::uint32_t, std::string>> kv;
                                    kv.value.emplace();
                                    kr.require('{', kp + ": expected '{'");
                                    if (kr.has_error()) return kv;
                                    while (kr.has_more() && !kr.has_error()) {
                                        std::string kk;
                                        if (!read_object_entry(kr, kk, kp)) break;
                                        auto kfp = json_path(kp, kk);
                                        if (kk == "tick") {
                                            auto v = kr.read_uint64_result();
                                            if (v.ok() && v.value) kv.value->first = static_cast<std::uint32_t>(*v.value);
                                        } else if (kk == "value") {
                                            auto v = kr.read_string_result();
                                            if (v.ok() && v.value) kv.value->second = std::move(*v.value);
                                        } else kr.skip_value();
                                        if (!kr.consume_if(',')) break;
                                    }
                                    kr.require('}', kp + ": expected '}'");
                                    return kv;
                                });
                            if (keys.ok() && keys.value) tr.value->keys = std::move(*keys.value);
                            else { tr.diagnostics.merge(keys.diagnostics); return tr; }
                        } else rr.skip_value();
                        if (!rr.consume_if(',')) break;
                    }
                    rr.require('}', tp + ": expected '}'");
                    return tr;
                });
            if (tracks.ok() && tracks.value) res.value->tracks = std::move(*tracks.value);
            else { res.diagnostics.merge(tracks.diagnostics); return res; }
        } else if (key == "events") {
            auto events = decode_array<std::pair<std::uint32_t, std::string>>(r, fp,
                [](BoundedJsonReader& er, std::string const& ep) {
                    JsonReadResult<std::pair<std::uint32_t, std::string>> ev;
                    ev.value.emplace();
                    er.require('{', ep + ": expected '{'");
                    if (er.has_error()) return ev;
                    while (er.has_more() && !er.has_error()) {
                        std::string ek;
                        if (!read_object_entry(er, ek, ep)) break;
                        auto efp = json_path(ep, ek);
                        if (ek == "tick") {
                            auto v = er.read_uint64_result();
                            if (v.ok() && v.value) ev.value->first = static_cast<std::uint32_t>(*v.value);
                        } else if (ek == "id") {
                            auto v = er.read_string_result();
                            if (v.ok() && v.value) ev.value->second = std::move(*v.value);
                        } else er.skip_value();
                        if (!er.consume_if(',')) break;
                    }
                    er.require('}', ep + ": expected '}'");
                    return ev;
                });
            if (events.ok() && events.value) res.value->clip_events = std::move(*events.value);
            else { res.diagnostics.merge(events.diagnostics); return res; }
        } else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalAnimationState> decode_animation_state(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalAnimationState> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "name")       decode_string_field(r, res.value->name, fp);
        else if (key == "clip_name") decode_string_field(r, res.value->clip_name, fp);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalTransition> decode_transition(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalTransition> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "from_state")    decode_string_field(r, res.value->from_state, fp);
        else if (key == "to_state") decode_string_field(r, res.value->to_state, fp);
        else if (key == "ability_id") decode_string_field(r, res.value->ability_id, fp);
        else if (key == "comparison") decode_string_field(r, res.value->comparison, fp);
        else if (key == "threshold") decode_uint32_field(r, res.value->threshold);
        else if (key == "resource_cost") decode_uint32_field(r, res.value->resource_cost);
        else if (key == "cooldown_ticks") decode_uint32_field(r, res.value->cooldown_ticks);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalCollisionShape> decode_collision_shape(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalCollisionShape> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id")         decode_string_field(r, res.value->id, fp);
        else if (key == "shape_type") decode_string_field(r, res.value->shape_type, fp);
        else if (key == "socket") decode_string_field(r, res.value->socket, fp);
        else if (key == "radius_mm") decode_double_field(r, res.value->radius_mm);
        else if (key == "offset_x") decode_double_field(r, res.value->offset_x);
        else if (key == "offset_y") decode_double_field(r, res.value->offset_y);
        else if (key == "scale_x") decode_double_field(r, res.value->scale_x);
        else if (key == "scale_y") decode_double_field(r, res.value->scale_y);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalCollisionWindow> decode_collision_window(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalCollisionWindow> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "ability_id")   decode_string_field(r, res.value->ability_id, fp);
        else if (key == "shape_id") decode_string_field(r, res.value->shape_id, fp);
        else if (key == "start_tick") decode_uint32_field(r, res.value->start_tick);
        else if (key == "duration_ticks") decode_uint32_field(r, res.value->duration_ticks);
        else if (key == "active") decode_bool_field(r, res.value->active);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalResource> decode_resource(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalResource> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "id")            decode_string_field(r, res.value->id, fp);
        else if (key == "resource_type") decode_string_field(r, res.value->resource_type, fp);
        else if (key == "min")      decode_uint32_field(r, res.value->min);
        else if (key == "max")      decode_uint32_field(r, res.value->max);
        else if (key == "initial")  decode_uint32_field(r, res.value->initial);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalAnimationIntent> decode_animation_intent(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalAnimationIntent> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "behavior_state") decode_string_field(r, res.value->behavior_state, fp);
        else if (key == "clip_name") decode_string_field(r, res.value->clip_name, fp);
        else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<CanonicalRuntime> decode_runtime(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<CanonicalRuntime> res;
    res.value.emplace();
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "aggression")       decode_uint32_field(r, res.value->aggression);
        else if (key == "curiosity")   decode_uint32_field(r, res.value->curiosity);
        else if (key == "energy")      decode_uint32_field(r, res.value->energy);
        else if (key == "loyalty")     decode_uint32_field(r, res.value->loyalty);
        else if (key == "animation_intents") {
            auto arr = decode_array<CanonicalAnimationIntent>(r, fp, decode_animation_intent);
            if (arr.ok() && arr.value) res.value->animation_intents = std::move(*arr.value);
            else { res.diagnostics.merge(arr.diagnostics); return res; }
        } else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

static JsonReadResult<GeneInstance> decode_gene_instance(
    BoundedJsonReader& r, std::string const& path) {
    JsonReadResult<GeneInstance> res;
    res.value.emplace();
    GeneRegistry registry;
    r.require('{', path + ": expected '{'");
    if (r.has_error()) return res;

    bool saw_kind = false, saw_schema = false, saw_type = false, saw_source = false;

    while (r.has_more() && !r.has_error()) {
        std::string key;
        if (!read_object_entry(r, key, path)) break;
        auto fp = json_path(path, key);

        if (key == "kind") {
            if (saw_kind) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                    fp + ": duplicate field 'kind'", {}); return res;
            }
            saw_kind = true;
            auto v = r.read_int64_result();
            if (!v.ok() || !v.value) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                    fp + ": expected GeneKind integer", {}); return res;
            }
            auto raw_kind = *v.value;
            if (raw_kind < 0 || raw_kind > 34) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    fp + ": invalid GeneKind value " + std::to_string(raw_kind), {});
                return res;
            }
            auto kind_val = static_cast<GeneKind>(raw_kind);
            auto const* desc = registry.lookup(kind_val);
            if (desc) res.value->descriptor = *desc;
            else res.value->descriptor.kind = kind_val;
        } else if (key == "schema") {
            if (saw_schema) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                    fp + ": duplicate field 'schema'", {}); return res;
            }
            saw_schema = true;
            auto v = r.read_uint64_result();
            if (v.ok() && v.value) {
                if (*v.value > std::numeric_limits<std::uint32_t>::max()) {
                    res.diagnostics.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED,
                        fp + ": schema version uint32 overflow", {}); return res;
                }
                res.value->descriptor.schema_version = static_cast<std::uint32_t>(*v.value);
            }
        } else if (key == "type") {
            if (saw_type) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                    fp + ": duplicate field 'type'", {}); return res;
            }
            saw_type = true;
            auto v = r.read_string_result();
            if (v.ok() && v.value) res.value->descriptor.type_id = std::move(*v.value);
        } else if (key == "source") {
            if (saw_source) {
                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                    fp + ": duplicate field 'source'", {}); return res;
            }
            saw_source = true;
            auto v = r.read_string_result();
            if (v.ok() && v.value) res.value->source_module = std::move(*v.value);
        } else if (key == "values") {
            r.require('{', fp + ": expected '{'");
            if (r.has_error()) break;
            while (r.has_more() && !r.has_error()) {
                auto vk_res = r.read_string_result();
                if (!vk_res.ok() || !vk_res.value) break;
                std::string vk = std::move(*vk_res.value);
                auto vfp = json_path(fp, vk);
                if (!r.require(':', vfp + ": expected ':'")) break;
                // Type-tagged format: { t: <tag>, v: <value> }
                if (r.require('{', vfp + ": expected '{'")) {
                    bool saw_tag = false, saw_value = false;
                    std::uint32_t tag_val = 0;
                    std::string raw_val;
                    while (r.has_more() && !r.has_error()) {
                        auto tk_res = r.read_string_result();
                        if (!tk_res.ok() || !tk_res.value) break;
                        auto tkfp = json_path(vfp, *tk_res.value);
                        if (!r.require(':', tkfp + ": expected ':'")) break;
                        if (*tk_res.value == "t") {
                            if (saw_tag) {
                                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                    tkfp + ": duplicate 't' in gene value wrapper", {});
                                return res;
                            }
                            saw_tag = true;
                            auto tv = r.read_int64_result();
                            if (!tv.ok() || !tv.value) {
                                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                    tkfp + ": expected GeneValueTag integer", {});
                                return res;
                            }
                            auto raw_tag = *tv.value;
                            if (raw_tag < 0 || raw_tag > 5) {
                                res.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                    tkfp + ": invalid GeneValueTag " + std::to_string(raw_tag), {});
                                return res;
                            }
                            tag_val = static_cast<std::uint32_t>(raw_tag);
                        } else if (*tk_res.value == "v") {
                            if (saw_value) {
                                res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                    tkfp + ": duplicate 'v' in gene value wrapper", {});
                                return res;
                            }
                            saw_value = true;
                            raw_val = r.read_typed_value();
                        } else r.skip_value();
                        if (!r.consume_if(',')) break;
                    }
                    r.require('}', vfp + ": expected '}'");
                    if (!saw_tag) {
                        res.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                            vfp + ": missing required 't' (GeneValueTag) in value wrapper", {});
                        return res;
                    }
                    if (!saw_value) {
                        res.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                            vfp + ": missing required 'v' (raw value) in value wrapper", {});
                        return res;
                    }
                    if (raw_val.empty()) {
                        res.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                            vfp + ": empty raw gene value", {});
                        return res;
                    }
                    auto gv = json_to_gene_value_result(raw_val, static_cast<GeneValueTag>(tag_val));
                    if (!gv.ok()) {
                        res.diagnostics.merge(gv.diagnostics);
                        return res;
                    }
                    res.value->values[vk] = std::move(*gv.value);
                } else {
                    // Current schema: reject bare-string values (no legacy fallback)
                    r.skip_value();
                }
                if (!r.consume_if(',')) break;
            }
            r.require('}', fp + ": expected '}'");
        } else r.skip_value();

        if (!r.consume_if(',')) break;
    }
    r.require('}', path + ": expected '}'");
    return res;
}

} // namespace

std::string CanonicalEntitySerializer::to_json(CanonicalEntity const& entity) {
    auto result = encode_canonical_entity(entity);
    if (!result.ok()) {
        throw std::invalid_argument("to_json: non-finite double value in entity — use to_json_result() for safe access");
    }
    return std::move(*result.value);
}

CanonicalSerializationResult CanonicalEntitySerializer::to_json_result(
    CanonicalEntity const& entity) {
    return encode_canonical_entity(entity);
}

// Pre-scan all doubles for nonfinite values before serialization
static void validate_double(double val, std::string const& json_path,
                            CanonicalSerializationResult& result) {
    if (!std::isfinite(val) && result.ok()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
            json_path + ": non-finite double value not allowed", {});
    }
}

CanonicalSerializationResult CanonicalEntitySerializer::encode_canonical_entity(
    CanonicalEntity const& entity) {
    CanonicalSerializationResult result;

    // Pre-validate all doubles — fail before writing any output
    for (std::size_t i = 0; i < entity.forms.size(); ++i) {
        auto const& f = entity.forms[i];
        auto prefix = "$.forms[" + std::to_string(i) + "]";
        validate_double(f.collision_scale, prefix + ".collision_scale", result);
        validate_double(f.ability_envelope, prefix + ".ability_envelope", result);
    }
    for (auto const& [name, part] : entity.morphology) {
        auto prefix = "$.morphology." + name;
        validate_double(part.x, prefix + ".x", result);
        validate_double(part.y, prefix + ".y", result);
        validate_double(part.z, prefix + ".z", result);
        validate_double(part.size_x, prefix + ".size_x", result);
        validate_double(part.size_y, prefix + ".size_y", result);
        validate_double(part.size_z, prefix + ".size_z", result);
        validate_double(part.rotation_degrees, prefix + ".rotation_degrees", result);
    }
    for (auto const& [form_name, parts] : entity.form_morphology_overrides) {
        for (auto const& [part_name, part] : parts) {
            auto prefix = "$.form_morphology_overrides." + form_name + "." + part_name;
            validate_double(part.x, prefix + ".x", result);
            validate_double(part.y, prefix + ".y", result);
            validate_double(part.z, prefix + ".z", result);
            validate_double(part.size_x, prefix + ".size_x", result);
            validate_double(part.size_y, prefix + ".size_y", result);
            validate_double(part.size_z, prefix + ".size_z", result);
            validate_double(part.rotation_degrees, prefix + ".rotation_degrees", result);
        }
    }
    for (std::size_t i = 0; i < entity.abilities.size(); ++i) {
        auto prefix = "$.abilities[" + std::to_string(i) + "]";
        validate_double(entity.abilities[i].speed_mm_per_tick, prefix + ".speed_mm_per_tick", result);
        validate_double(entity.abilities[i].collision_radius_mm, prefix + ".collision_radius_mm", result);
    }
    for (std::size_t i = 0; i < entity.storm_abilities.size(); ++i) {
        auto prefix = "$.storm_abilities[" + std::to_string(i) + "]";
        validate_double(entity.storm_abilities[i].speed_mm_per_tick, prefix + ".speed_mm_per_tick", result);
        validate_double(entity.storm_abilities[i].collision_radius_mm, prefix + ".collision_radius_mm", result);
    }
    for (std::size_t i = 0; i < entity.bones.size(); ++i) {
        auto prefix = "$.bones[" + std::to_string(i) + "]";
        auto const& b = entity.bones[i];
        validate_double(b.x, prefix + ".x", result);
        validate_double(b.y, prefix + ".y", result);
        validate_double(b.z, prefix + ".z", result);
        validate_double(b.scale_x, prefix + ".scale_x", result);
        validate_double(b.scale_y, prefix + ".scale_y", result);
        validate_double(b.length_mm, prefix + ".length_mm", result);
        validate_double(b.min_rotation, prefix + ".min_rotation", result);
        validate_double(b.max_rotation, prefix + ".max_rotation", result);
    }
    for (std::size_t i = 0; i < entity.sockets.size(); ++i) {
        auto prefix = "$.sockets[" + std::to_string(i) + "]";
        auto const& s = entity.sockets[i];
        validate_double(s.x, prefix + ".x", result);
        validate_double(s.y, prefix + ".y", result);
        validate_double(s.z, prefix + ".z", result);
        validate_double(s.scale_x, prefix + ".scale_x", result);
        validate_double(s.scale_y, prefix + ".scale_y", result);
    }
    for (std::size_t i = 0; i < entity.collision_shapes.size(); ++i) {
        auto prefix = "$.collision_shapes[" + std::to_string(i) + "]";
        auto const& cs = entity.collision_shapes[i];
        validate_double(cs.radius_mm, prefix + ".radius_mm", result);
        validate_double(cs.offset_x, prefix + ".offset_x", result);
        validate_double(cs.offset_y, prefix + ".offset_y", result);
        validate_double(cs.scale_x, prefix + ".scale_x", result);
        validate_double(cs.scale_y, prefix + ".scale_y", result);
    }
    if (!result.diagnostics.ok()) return result;

    try {
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
    // initial_state — must be parsed before any array to avoid cascading breakout
    os << "  \"initial_state\": \"" << canonical_escape(entity.initial_state) << "\",\n";
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
    // form_morphology_overrides — only if non-empty
    if (!entity.form_morphology_overrides.empty()) {
        os << "  \"form_morphology_overrides\": {";
    bool first_form = true;
    for (auto const& [form_name, parts] : entity.form_morphology_overrides) {
        if (!first_form) os << ", ";
        first_form = false;
        os << "\"" << canonical_escape(form_name) << "\": {";
        bool first_part = true;
        for (auto const& [part_name, part] : parts) {
            if (!first_part) os << ", ";
            first_part = false;
            os << "\"" << canonical_escape(part_name) << "\":{"
               << "\"parent\":\"" << canonical_escape(part.parent) << "\""
               << ",\"x\":" << part.x << ",\"y\":" << part.y << ",\"z\":" << part.z
               << ",\"size_x\":" << part.size_x << ",\"size_y\":" << part.size_y << ",\"size_z\":" << part.size_z
               << ",\"color\":\"" << canonical_escape(part.color) << "\""
               << ",\"rotation_degrees\":" << part.rotation_degrees
               << ",\"emissive\":" << (part.emissive ? "true" : "false")
               << ",\"electrical_marking\":" << (part.electrical_marking ? "true" : "false")
               << "}";
        }
        os << "}";
    }
    os << "},\n";
    }
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
    // genes — full typed gene instances
    if (!entity.genes.empty()) {
        os << "  \"genes\": [\n";
        for (std::size_t i = 0; i < entity.genes.size(); ++i) {
            if (i > 0) os << ",\n";
            auto const& g = entity.genes[i];
            os << "    {\"kind\":" << static_cast<std::uint32_t>(g.descriptor.kind)
               << ",\"schema\":" << g.descriptor.schema_version
               << ",\"type\":\"" << canonical_escape(g.descriptor.type_id) << "\""
               << ",\"source\":\"" << canonical_escape(g.source_module) << "\""
               << ",\"values\":{";
            bool first_val = true;
            for (auto const& [k, v] : g.values) {
                if (!first_val) os << ",";
                first_val = false;
                auto tag = static_cast<std::uint32_t>(gene_value_variant_index(v));
                os << "\"" << canonical_escape(k) << "\":{\"t\":" << tag
                   << ",\"v\":" << gene_value_to_json(v) << "}";
            }
            os << "}}";
        }
        os << "\n  ],\n";
    }
    os << "  \"gene_count\": " << entity.genes.size() << "\n";
    os << "}";
    result.value = os.str();
    return result;
    } catch (std::invalid_argument const& e) {
        // Nonfinite double detected — record diagnostic, return nullopt
        result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                     e.what(), {});
        return result;
    }
}

CanonicalEntityDeserializeResult CanonicalEntitySerializer::from_json(
    std::string_view json,
    BoundedJsonConfig config) {
    CanonicalEntityDeserializeResult result;

    if (json.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: empty input", {});
        return result;
    }

    BoundedJsonReader r(json, config);
    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                     r.error_message(), {});
        return result;
    }

    r.require('{', "$: expected '{'");
    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                     r.error_message(), {});
        return result;
    }

    CanonicalEntity ce;
    bool parsed_any = false;

    while (r.has_more() && !r.has_error()) {
        auto key_res = r.read_string_result();
        if (!key_res.ok() || !key_res.value) break;
        std::string key = std::move(*key_res.value);
        r.require(':', "$." + key + ": expected ':'");
        if (r.has_error()) break;
        parsed_any = true;

        if (key == "schema_version") {
            auto v = r.read_string_result();
            if (v.ok() && v.value) ce.schema_version = std::move(*v.value);
        } else if (key == "stable_id") {
            auto v = r.read_string_result();
            if (v.ok() && v.value) ce.stable_id = std::move(*v.value);
        } else if (key == "name") {
            auto v = r.read_string_result();
            if (v.ok() && v.value) ce.name = std::move(*v.value);
        } else if (key == "classification") {
            auto v = r.read_string_result();
            if (v.ok() && v.value) ce.classification = std::move(*v.value);
        } else if (key == "rights") {
            auto v = r.read_string_result();
            if (v.ok() && v.value) ce.rights = std::move(*v.value);
        } else if (key == "rights_allow_export") {
            auto v = r.read_bool_result();
            if (v.ok() && v.value) ce.rights_allow_export = *v.value;
        } else if (key == "entropy_root") {
            auto v = r.read_uint64_result();
            if (v.ok() && v.value) ce.entropy_root = *v.value;
        } else if (key == "primary_color") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.primary_color = std::move(*v.value);
        } else if (key == "accent_color") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.accent_color = std::move(*v.value);
        } else if (key == "storm_primary_color") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.storm_primary_color = std::move(*v.value);
        } else if (key == "storm_accent_color") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.storm_accent_color = std::move(*v.value);
        } else if (key == "emissive_color") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.emissive_color = std::move(*v.value);
        } else if (key == "aura_color") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.aura_color = std::move(*v.value);
        } else if (key == "provenance_hash") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.provenance_hash = std::move(*v.value);
        } else if (key == "provenance_source") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.provenance_source = std::move(*v.value);
        } else if (key == "initial_state") {
            auto v = r.read_string_result(); if (v.ok() && v.value) ce.initial_state = std::move(*v.value);
        } else if (key == "forms") {
            auto arr = decode_array<CanonicalForm>(r, "$.forms", decode_canonical_form);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.forms = std::move(*arr.value);
        } else if (key == "transformations") {
            auto arr = decode_array<CanonicalTransformation>(r, "$.transformations", decode_transformation);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.transformations = std::move(*arr.value);
        } else if (key == "morphology") {
            r.require('{', "$.morphology: expected '{'");
            if (r.has_error()) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, r.error_message(), {}); return result; }
            while (r.has_more() && !r.has_error()) {
                auto part_name_res = r.read_string_result();
                if (!part_name_res.ok() || !part_name_res.value) break;
                std::string part_name = std::move(*part_name_res.value);
                r.require(':', "$.morphology." + part_name + ": expected ':'");
                if (r.has_error()) break;
                auto part = decode_canonical_part(r, "$.morphology." + part_name);
                if (!part.ok()) { result.diagnostics.merge(part.diagnostics); return result; }
                if (part.value) { part.value->name = part_name; ce.morphology[part_name] = std::move(*part.value); }
                if (!r.consume_if(',')) break;
            }
            r.require('}', "$.morphology: expected '}'");
            if (r.has_error()) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, r.error_message(), {}); return result; }
        } else if (key == "form_morphology_overrides") {
            r.require('{', "$.form_morphology_overrides: expected '{'");
            if (r.has_error()) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, r.error_message(), {}); return result; }
            while (r.has_more() && !r.has_error()) {
                auto form_name_res = r.read_string_result();
                if (!form_name_res.ok() || !form_name_res.value) break;
                std::string form_name = std::move(*form_name_res.value);
                r.require(':', "$.form_morphology_overrides." + form_name + ": expected ':'");
                if (r.has_error()) break;
                r.require('{', "$.form_morphology_overrides." + form_name + ": expected '{'");
                if (r.has_error()) break;
                while (r.has_more() && !r.has_error()) {
                    auto part_name_res = r.read_string_result();
                    if (!part_name_res.ok() || !part_name_res.value) break;
                    std::string part_name = std::move(*part_name_res.value);
                    r.require(':', "$.form_morphology_overrides." + form_name + "." + part_name + ": expected ':'");
                    if (r.has_error()) break;
                    auto part = decode_canonical_part(r, "$.form_morphology_overrides." + form_name + "." + part_name);
                    if (!part.ok()) { result.diagnostics.merge(part.diagnostics); return result; }
                    if (part.value) { part.value->name = part_name; ce.form_morphology_overrides[form_name][part_name] = std::move(*part.value); }
                    if (!r.consume_if(',')) break;
                }
                r.require('}', "$.form_morphology_overrides." + form_name + ": expected '}'");
                if (r.has_error()) break;
                if (!r.consume_if(',')) break;
            }
            r.require('}', "$.form_morphology_overrides: expected '}'");
            if (r.has_error()) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, r.error_message(), {}); return result; }
        } else if (key == "abilities") {
            auto arr = decode_array<CanonicalAbility>(r, "$.abilities", decode_ability);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.abilities = std::move(*arr.value);
        } else if (key == "storm_abilities") {
            auto arr = decode_array<CanonicalAbility>(r, "$.storm_abilities", decode_ability);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.storm_abilities = std::move(*arr.value);
        } else if (key == "bones") {
            auto arr = decode_array<CanonicalSkeletalBone>(r, "$.bones", decode_bone);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.bones = std::move(*arr.value);
        } else if (key == "sockets") {
            auto arr = decode_array<CanonicalSocket>(r, "$.sockets", decode_socket);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.sockets = std::move(*arr.value);
        } else if (key == "clips") {
            auto arr = decode_array<CanonicalAnimationClip>(r, "$.clips", decode_animation_clip);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.clips = std::move(*arr.value);
        } else if (key == "states") {
            auto arr = decode_array<CanonicalAnimationState>(r, "$.states", decode_animation_state);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.states = std::move(*arr.value);
        } else if (key == "transitions") {
            auto arr = decode_array<CanonicalTransition>(r, "$.transitions", decode_transition);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.transitions = std::move(*arr.value);
        } else if (key == "collision_shapes") {
            auto arr = decode_array<CanonicalCollisionShape>(r, "$.collision_shapes", decode_collision_shape);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.collision_shapes = std::move(*arr.value);
        } else if (key == "collision_windows") {
            auto arr = decode_array<CanonicalCollisionWindow>(r, "$.collision_windows", decode_collision_window);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.collision_windows = std::move(*arr.value);
        } else if (key == "resources") {
            auto arr = decode_array<CanonicalResource>(r, "$.resources", decode_resource);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.resources = std::move(*arr.value);
        } else if (key == "runtime") {
            auto rt = decode_runtime(r, "$.runtime");
            if (!rt.ok()) { result.diagnostics.merge(rt.diagnostics); return result; }
            if (rt.value) ce.runtime = std::move(*rt.value);
        } else if (key == "genes") {
            auto arr = decode_array<GeneInstance>(r, "$.genes", decode_gene_instance);
            if (!arr.ok()) { result.diagnostics.merge(arr.diagnostics); return result; }
            if (arr.value) ce.genes = std::move(*arr.value);
        } else if (key == "gene_count") {
            r.skip_value(); // metadata field, not structural
        } else {
            r.skip_value();
        }

        if (!r.consume_if(',')) break;
    }

    // Document closure
    if (!r.has_error()) {
        r.skip_ws();
        if (!r.consume_if('}')) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                         "from_json: missing closing '}' of root object", {});
            return result;
        }
        r.skip_ws();
        if (r.position() < r.source().size()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                         "from_json: trailing data after root '}'", {});
            return result;
        }
    }

    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                     r.error_message(), {});
        return result;
    }
    if (!parsed_any) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: no fields parsed from input", {});
        return result;
    }

    // Required fields
    if (ce.stable_id.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: missing required field 'stable_id'", {});
        return result;
    }
    if (ce.name.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                       "from_json: missing required field 'name'", {});
        return result;
    }

    // Enforce current schema version
    if (ce.schema_version != "gspl.canonical-entity/1.0") {
        if (ce.schema_version.empty()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                           "from_json: missing required field 'schema_version'", {});
        } else {
            result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                "from_json: unsupported schema version '" + ce.schema_version + "' (expected 'gspl.canonical-entity/1.0')",
                {});
        }
        return result;
    }

    // Post-deserialization structural validation
    CanonicalEntityValidator validator;
    auto validation_diags = validator.validate(ce);
    if (!validation_diags.ok()) {
        result.diagnostics.merge(validation_diags);
        return result;
    }

    result.value = std::move(ce);
    return result;
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
    auto result = CanonicalEntitySerializer::to_json_result(entity);
    return result.ok() ? *result.value : std::string{};
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
