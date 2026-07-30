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
        if (!part.bone_id.empty()) append_key_value(out, "bone", part.bone_id);
        if (!part.primitive.empty() && part.primitive != "ellipse") append_key_value(out, "primitive", part.primitive);
        if (!part.semantic_role.empty()) append_key_value(out, "semantic_role", part.semantic_role);
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
        if (part.z_order != 0) append_key_value(out, "z_order", part.z_order);
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
            for (auto const& kf : track.keys) {
                append_key_value(out, "tick", kf.tick);
                if (kf.x != 0.0) out << "x=" << kf.x << ";";
                if (kf.y != 0.0) out << "y=" << kf.y << ";";
                if (kf.rotation_degrees != 0.0) out << "rotation_degrees=" << kf.rotation_degrees << ";";
                if (kf.scale_x != 1.0) out << "scale_x=" << kf.scale_x << ";";
                if (kf.scale_y != 1.0) out << "scale_y=" << kf.scale_y << ";";
                if (!kf.legacy_transform.empty()) append_key_value(out, "value", kf.legacy_transform);
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
            append_key_value(out, "part_id", part.part_id);
            if (part.x) append_key_value(out, "x", *part.x);
            if (part.y) append_key_value(out, "y", *part.y);
            if (part.z) append_key_value(out, "z", *part.z);
            if (part.size_x) append_key_value(out, "size_x", *part.size_x);
            if (part.size_y) append_key_value(out, "size_y", *part.size_y);
            if (part.size_z) append_key_value(out, "size_z", *part.size_z);
            if (part.color) append_key_value(out, "color", *part.color);
            if (part.rotation_degrees) append_key_value(out, "rotation_degrees", *part.rotation_degrees);
            if (part.emissive) append_key_value(out, "emissive", *part.emissive);
            if (part.electrical_marking) append_key_value(out, "electrical_marking", *part.electrical_marking);
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

// ── Reader failure helper ───────────────────────────────

static void reader_failure(BoundedJsonReader const& r, std::string const& path,
                           DiagnosticResult& diags, DiagnosticCode code) {
    auto pos = r.source_position();
    auto msg = r.error_message();
    if (msg.empty()) msg = "parser error";
    diags.add(Diagnostic{code, DiagnosticSeverity::error,
        path + ": " + msg,
        {pos.byte_offset,
         static_cast<std::uint32_t>(pos.line),
         static_cast<std::uint32_t>(pos.column), 0, 0}, {}, {}});
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

// ── Generic object decoder ──────────────────────────────

// Structural grammar for governed JSON objects:
// - duplicate field detection via seen_keys
// - trailing comma rejection
// - required field enforcement
// - T candidate pattern (value assigned only after success)
// - error propagation from handler

template <typename T, typename Handler>
static JsonReadResult<T> decode_object(BoundedJsonReader& r,
                                       std::string const& path,
                                       std::set<std::string, std::less<>> required,
                                       Handler&& handler) {
    JsonReadResult<T> res;
    T candidate;
    std::set<std::string, std::less<>> seen;

    r.require('{', path + ": expected '{'");
    if (r.has_error()) { reader_failure(r, path, res.diagnostics, DiagnosticCode::GSPL_TYPE_MISMATCH); return res; }

    while (r.has_more() && !r.has_error()) {
        r.skip_ws();
        if (r.position() < r.source().size() && r.source()[r.position()] == '}') break;

        std::string key;
        if (!read_object_entry(r, key, path)) break;

        // Duplicate detection
        if (!seen.insert(key).second) {
            res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                path + ".\"" + key + "\": duplicate field", {});
            return res;
        }

        auto fp = json_path(path, key);
        handler(key, fp, candidate, res.diagnostics, r);
        if (!res.diagnostics.ok()) return res;

        // After field value: expect ',' or '}'
        r.skip_ws();
        if (r.position() < r.source().size() && r.source()[r.position()] == '}') break;
        r.require(',', fp + ": expected ',' or '}' after field");
        if (r.has_error()) { reader_failure(r, fp, res.diagnostics, DiagnosticCode::GSPL_TYPE_MISMATCH); return res; }
    }

    r.require('}', path + ": expected '}'");
    if (r.has_error()) { reader_failure(r, path, res.diagnostics, DiagnosticCode::GSPL_TYPE_MISMATCH); return res; }

    // Required field check
    for (auto const& req : required) {
        if (!seen.contains(req)) {
            res.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                path + ": missing required field '" + req + "'", {});
            return res;
        }
    }

    res.value = std::move(candidate);
    return res;
}

// ── Type codecs ──────────────────────────────────────────

// Helper: decode a field that must be a string, abort on failure
#define DECODE_STRING_FIELD(r, key, fp, candidate, member, diags) \
    do { \
        if (key == #member) { \
            auto _v = decode_string_field(r, fp); \
            if (!_v.ok()) { diags.merge(_v.diagnostics); return; } \
            candidate.member = std::move(*_v.value); \
            return; \
        } \
    } while(0)

#define DECODE_UINT32_FIELD(r, key, fp, candidate, member, diags) \
    do { \
        if (key == #member) { \
            auto _v = decode_uint32_field(r, fp); \
            if (!_v.ok()) { diags.merge(_v.diagnostics); return; } \
            candidate.member = *_v.value; \
            return; \
        } \
    } while(0)

#define DECODE_DOUBLE_FIELD(r, key, fp, candidate, member, diags) \
    do { \
        if (key == #member) { \
            auto _v = decode_double_field(r, fp); \
            if (!_v.ok()) { diags.merge(_v.diagnostics); return; } \
            candidate.member = *_v.value; \
            return; \
        } \
    } while(0)

#define DECODE_BOOL_FIELD(r, key, fp, candidate, member, diags) \
    do { \
        if (key == #member) { \
            auto _v = decode_bool_field(r, fp); \
            if (!_v.ok()) { diags.merge(_v.diagnostics); return; } \
            candidate.member = *_v.value; \
            return; \
        } \
    } while(0)

static JsonReadResult<CanonicalMorphologyPartOverride> decode_canonical_override(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalMorphologyPartOverride>(r, path, {},
        [](std::string const& key, std::string const& /*fp*/,
           CanonicalMorphologyPartOverride& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            if (key == "x") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.x = *v.value; return; }
            if (key == "y") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.y = *v.value; return; }
            if (key == "z") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.z = *v.value; return; }
            if (key == "size_x") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.size_x = *v.value; return; }
            if (key == "size_y") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.size_y = *v.value; return; }
            if (key == "size_z") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.size_z = *v.value; return; }
            if (key == "rotation_degrees") { auto v = rr.read_double_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.rotation_degrees = *v.value; return; }
            if (key == "color") { auto v = rr.read_string_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.color = std::move(*v.value); return; }
            if (key == "bone_id") { auto v = rr.read_string_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.bone_id = std::move(*v.value); return; }
            if (key == "primitive") { auto v = rr.read_string_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.primitive = std::move(*v.value); return; }
            if (key == "semantic_role") { auto v = rr.read_string_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.semantic_role = std::move(*v.value); return; }
            if (key == "z_order") { auto v = rr.read_int32_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.z_order = *v.value; return; }
            if (key == "emissive") { auto v = rr.read_bool_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.emissive = *v.value; return; }
            if (key == "electrical_marking") { auto v = rr.read_bool_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.electrical_marking = *v.value; return; }
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalPart> decode_canonical_part(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalPart>(r, path, {},
        [](std::string const& key, std::string const& fp,
           CanonicalPart& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, parent, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, y, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, z, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, size_x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, size_y, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, size_z, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, color, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, rotation_degrees, diags);
            DECODE_BOOL_FIELD(rr, key, fp, c, emissive, diags);
            DECODE_BOOL_FIELD(rr, key, fp, c, electrical_marking, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, bone_id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, primitive, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, semantic_role, diags);
            if (key == "z_order") { auto v = rr.read_int32_result(); if (!v.ok()) { diags.merge(v.diagnostics); return; } if (v.value) c.z_order = *v.value; return; }
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalForm> decode_canonical_form(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalForm>(r, path, {"id"},
        [](std::string const& key, std::string const& fp,
           CanonicalForm& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, resource_capacity, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, collision_scale, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, ability_envelope, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, max_health, diags);
            if (key == "transformation_ids") {
                auto arr = decode_string_array(rr, fp);
                if (!arr.ok()) { diags.merge(arr.diagnostics); return; }
                if (arr.value) c.transformation_ids = std::move(*arr.value);
                return;
            }
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalTransformation> decode_transformation(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalTransformation>(r, path, {"id", "from_form", "to_form"},
        [](std::string const& key, std::string const& fp,
           CanonicalTransformation& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, from_form, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, to_form, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, trigger_condition, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, duration_ticks, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, resource_cost, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalAbility> decode_ability(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalAbility>(r, path, {"id"},
        [](std::string const& key, std::string const& fp,
           CanonicalAbility& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, effect, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, cost, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, cooldown_ticks, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, active_ticks, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, origin_socket, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, speed_mm_per_tick, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, collision_radius_mm, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, status_id, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, status_duration_ticks, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalSkeletalBone> decode_bone(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalSkeletalBone>(r, path, {"id"},
        [](std::string const& key, std::string const& fp,
           CanonicalSkeletalBone& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, parent, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, y, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, z, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, scale_x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, scale_y, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, length_mm, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, min_rotation, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, max_rotation, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalSocket> decode_socket(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalSocket>(r, path, {"id", "bone"},
        [](std::string const& key, std::string const& fp,
           CanonicalSocket& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, bone, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, y, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, z, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, scale_x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, scale_y, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalAnimationClip> decode_animation_clip(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalAnimationClip>(r, path, {"name"},
        [](std::string const& key, std::string const& fp,
           CanonicalAnimationClip& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, name, diags);
            DECODE_BOOL_FIELD(rr, key, fp, c, loop, diags);
            if (key == "tracks") {
                auto arr = decode_array<CanonicalAnimationClip::Track>(rr, fp,
                    [](BoundedJsonReader& tr, std::string const& tp) {
                        return decode_object<CanonicalAnimationClip::Track>(tr, tp, {"bone", "keys"},
                            [](std::string const& tk, std::string const& tfp,
                               CanonicalAnimationClip::Track& t, DiagnosticResult& td, BoundedJsonReader& trr) {
                                DECODE_STRING_FIELD(trr, tk, tfp, t, bone, td);
                                if (tk == "keys") {
                                    auto keys = decode_array<CanonicalKeyframe>(trr, tfp,
                                        [](BoundedJsonReader& kr, std::string const& kp) {
                                            return decode_object<CanonicalKeyframe>(kr, kp, {"tick"},
                                                [](std::string const& kk, std::string const& kfp,
                                                   CanonicalKeyframe& kv, DiagnosticResult& kd, BoundedJsonReader& krr) {
                                                    if (kk == "tick") {
                                                        auto v = decode_uint32_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.tick = *v.value;
                                                    } else if (kk == "x") {
                                                        auto v = decode_double_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.x = *v.value;
                                                    } else if (kk == "y") {
                                                        auto v = decode_double_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.y = *v.value;
                                                    } else if (kk == "rotation_degrees") {
                                                        auto v = decode_double_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.rotation_degrees = *v.value;
                                                    } else if (kk == "scale_x") {
                                                        auto v = decode_double_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.scale_x = *v.value;
                                                    } else if (kk == "scale_y") {
                                                        auto v = decode_double_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.scale_y = *v.value;
                                                    } else if (kk == "value") {
                                                        auto v = decode_string_field(krr, kfp);
                                                        if (!v.ok()) { kd.merge(v.diagnostics); return; }
                                                        kv.legacy_transform = std::move(*v.value);
                                                    } else krr.skip_value();
                                                });
                                        });
                                    if (!keys.ok()) { td.merge(keys.diagnostics); return; }
                                    if (keys.value) t.keys = std::move(*keys.value);
                                    return;
                                }
                                trr.skip_value();
                            });
                    });
                if (!arr.ok()) { diags.merge(arr.diagnostics); return; }
                if (arr.value) c.tracks = std::move(*arr.value);
                return;
            }
            if (key == "events") {
                auto arr = decode_array<std::pair<std::uint32_t, std::string>>(rr, fp,
                    [](BoundedJsonReader& er, std::string const& ep) {
                        return decode_object<std::pair<std::uint32_t, std::string>>(er, ep, {"tick", "id"},
                            [](std::string const& ek, std::string const& efp,
                               std::pair<std::uint32_t, std::string>& ev, DiagnosticResult& ed, BoundedJsonReader& err) {
                                if (ek == "tick") {
                                    auto v = err.read_uint64_result();
                                    if (!v.ok() || !v.value) { ed.merge(v.diagnostics); return; }
                                    if (*v.value > 0xFFFFFFFFULL) { ed.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED, efp + ": tick overflow", {}); return; }
                                    ev.first = static_cast<std::uint32_t>(*v.value);
                                } else if (ek == "id") {
                                    auto v = err.read_string_result();
                                    if (!v.ok() || !v.value) { ed.merge(v.diagnostics); return; }
                                    ev.second = std::move(*v.value);
                                } else err.skip_value();
                            });
                    });
                if (!arr.ok()) { diags.merge(arr.diagnostics); return; }
                if (arr.value) c.clip_events = std::move(*arr.value);
                return;
            }
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalAnimationState> decode_animation_state(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalAnimationState>(r, path, {"name", "clip_name"},
        [](std::string const& key, std::string const& fp,
           CanonicalAnimationState& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, name, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, clip_name, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalTransition> decode_transition(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalTransition>(r, path, {"from_state", "to_state"},
        [](std::string const& key, std::string const& fp,
           CanonicalTransition& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, from_state, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, to_state, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, ability_id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, comparison, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, threshold, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, resource_cost, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, cooldown_ticks, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalCollisionShape> decode_collision_shape(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalCollisionShape>(r, path, {"id", "shape_type"},
        [](std::string const& key, std::string const& fp,
           CanonicalCollisionShape& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, shape_type, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, socket, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, radius_mm, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, offset_x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, offset_y, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, scale_x, diags);
            DECODE_DOUBLE_FIELD(rr, key, fp, c, scale_y, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalCollisionWindow> decode_collision_window(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalCollisionWindow>(r, path, {"ability_id", "shape_id"},
        [](std::string const& key, std::string const& fp,
           CanonicalCollisionWindow& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, ability_id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, shape_id, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, start_tick, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, duration_ticks, diags);
            DECODE_BOOL_FIELD(rr, key, fp, c, active, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalResource> decode_resource(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalResource>(r, path, {"id", "resource_type"},
        [](std::string const& key, std::string const& fp,
           CanonicalResource& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, id, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, resource_type, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, min, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, max, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, initial, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalAnimationIntent> decode_animation_intent(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalAnimationIntent>(r, path, {"behavior_state", "clip_name"},
        [](std::string const& key, std::string const& fp,
           CanonicalAnimationIntent& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_STRING_FIELD(rr, key, fp, c, behavior_state, diags);
            DECODE_STRING_FIELD(rr, key, fp, c, clip_name, diags);
            rr.skip_value();
        });
}

static JsonReadResult<CanonicalRuntime> decode_runtime(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<CanonicalRuntime>(r, path, {},
        [](std::string const& key, std::string const& fp,
           CanonicalRuntime& c, DiagnosticResult& diags, BoundedJsonReader& rr) {
            DECODE_UINT32_FIELD(rr, key, fp, c, aggression, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, curiosity, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, energy, diags);
            DECODE_UINT32_FIELD(rr, key, fp, c, loyalty, diags);
            if (key == "animation_intents") {
                auto arr = decode_array<CanonicalAnimationIntent>(rr, fp, decode_animation_intent);
                if (!arr.ok()) { diags.merge(arr.diagnostics); return; }
                if (arr.value) c.animation_intents = std::move(*arr.value);
                return;
            }
            rr.skip_value();
        });
}

static JsonReadResult<GeneInstance> decode_gene_instance(
    BoundedJsonReader& r, std::string const& path) {
    return decode_object<GeneInstance>(r, path, {"kind", "schema", "type", "values"},
        [](std::string const& key, std::string const& fp,
           GeneInstance& gi, DiagnosticResult& diags, BoundedJsonReader& rr) {
            GeneRegistry registry;
            if (key == "kind") {
                auto v = rr.read_int64_result();
                if (!v.ok() || !v.value) { diags.merge(v.diagnostics); return; }
                auto raw_kind = *v.value;
                if (raw_kind < 0 || raw_kind > static_cast<std::int64_t>(GeneKind::provenance)) {
                    diags.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                        fp + ": invalid GeneKind " + std::to_string(raw_kind), {}); return;
                }
                auto kind_val = static_cast<GeneKind>(raw_kind);
                auto const* desc = registry.lookup(kind_val);
                if (desc) gi.descriptor = *desc;
                else gi.descriptor.kind = kind_val;
                return;
            }
            if (key == "schema") {
                auto v = rr.read_uint64_result();
                if (!v.ok() || !v.value) { diags.merge(v.diagnostics); return; }
                if (*v.value > 0xFFFFFFFFULL) { diags.add_error(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED, fp + ": schema uint32 overflow", {}); return; }
                gi.descriptor.schema_version = static_cast<std::uint32_t>(*v.value);
                return;
            }
            if (key == "type") {
                auto v = rr.read_string_result();
                if (!v.ok() || !v.value) { diags.merge(v.diagnostics); return; }
                gi.descriptor.type_id = std::move(*v.value);
                return;
            }
            if (key == "source") {
                auto v = rr.read_string_result();
                if (!v.ok() || !v.value) { diags.merge(v.diagnostics); return; }
                gi.source_module = std::move(*v.value);
                return;
            }
            if (key == "values") {
                rr.require('{', fp + ": expected '{'");
                if (rr.has_error()) { reader_failure(rr, fp, diags, DiagnosticCode::GSPL_TYPE_MISMATCH); return; }
                while (rr.has_more() && !rr.has_error()) {
                    auto vk = rr.read_string_result();
                    if (!vk.ok() || !vk.value) break;
                    std::string vk_str = std::move(*vk.value);
                    auto vfp = json_path(fp, vk_str);
                    if (!rr.require(':', vfp + ": expected ':'")) break;
                    if (rr.peek() == '{') {
                        bool saw_tag = false, saw_value = false;
                        std::uint32_t tag_val = 0;
                        std::string raw_val;
                        rr.require('{', vfp + ": expected '{'");
                        while (rr.has_more() && !rr.has_error()) {
                            auto tk = rr.read_string_result();
                            if (!tk.ok() || !tk.value) break;
                            auto tkfp = json_path(vfp, *tk.value);
                            if (!rr.require(':', tkfp + ": expected ':'")) break;
                            if (*tk.value == "t") {
                                if (saw_tag) { diags.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, tkfp + ": duplicate 't'", {}); return; }
                                saw_tag = true;
                                auto tv = rr.read_int64_result();
                                if (!tv.ok() || !tv.value) { diags.merge(tv.diagnostics); return; }
                                if (*tv.value < 0 || *tv.value > static_cast<std::int64_t>(GeneValueTag::string_list_val)) {
                                    diags.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE, tkfp + ": invalid GeneValueTag", {}); return;
                                }
                                tag_val = static_cast<std::uint32_t>(*tv.value);
                            } else if (*tk.value == "v") {
                                if (saw_value) { diags.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, tkfp + ": duplicate 'v'", {}); return; }
                                saw_value = true;
                                raw_val = rr.read_typed_value();
                            } else rr.skip_value();
                            if (!rr.consume_if(',')) break;
                        }
                        rr.require('}', vfp + ": expected '}'");
                        if (!saw_tag) { diags.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE, vfp + ": missing 't'", {}); return; }
                        if (!saw_value || raw_val.empty()) { diags.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE, vfp + ": missing/empty 'v'", {}); return; }
                        auto gv = json_to_gene_value_result(raw_val, static_cast<GeneValueTag>(tag_val));
                        if (!gv.ok()) { diags.merge(gv.diagnostics); return; }
                        gi.values[vk_str] = std::move(*gv.value);
                    } else rr.skip_value();
                    if (!rr.consume_if(',')) break;
                }
                rr.require('}', fp + ": expected '}'");
                return;
            }
            rr.skip_value();
        });
}

} // namespace

std::string CanonicalEntitySerializer::to_json(CanonicalEntity const& entity) {
    auto result = encode_canonical_entity(entity);
    if (!result.ok()) {
        throw std::invalid_argument("to_json: entity failed validation or contains non-finite values — use to_json_result() for safe access");
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
    if (!std::isfinite(val)) {
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
            if (part.x) validate_double(*part.x, prefix + ".x", result);
            if (part.y) validate_double(*part.y, prefix + ".y", result);
            if (part.z) validate_double(*part.z, prefix + ".z", result);
            if (part.size_x) validate_double(*part.size_x, prefix + ".size_x", result);
            if (part.size_y) validate_double(*part.size_y, prefix + ".size_y", result);
            if (part.size_z) validate_double(*part.size_z, prefix + ".size_z", result);
            if (part.rotation_degrees) validate_double(*part.rotation_degrees, prefix + ".rotation_degrees", result);
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

    // Semantic validation before serialization
    CanonicalEntityValidator validator;
    auto validation_diags = validator.validate(entity);
    if (!validation_diags.ok()) {
        result.diagnostics.merge(validation_diags);
        return result;
    }

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
           << ",\"z_order\":" << part.z_order;
        if (!part.bone_id.empty())
           os << ",\"bone_id\":\"" << canonical_escape(part.bone_id) << "\"";
        if (!part.primitive.empty())
           os << ",\"primitive\":\"" << canonical_escape(part.primitive) << "\"";
        if (!part.semantic_role.empty())
           os << ",\"semantic_role\":\"" << canonical_escape(part.semantic_role) << "\"";
        os << "}";
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
            os << "\"" << canonical_escape(part_name) << "\":{";
            // Serialize only fields that are present (optional overrides)
            if (part.x) os << "\"x\":" << *part.x << ",";
            if (part.y) os << "\"y\":" << *part.y << ",";
            if (part.z) os << "\"z\":" << *part.z << ",";
            if (part.size_x) os << "\"size_x\":" << *part.size_x << ",";
            if (part.size_y) os << "\"size_y\":" << *part.size_y << ",";
            if (part.size_z) os << "\"size_z\":" << *part.size_z << ",";
            if (part.color) os << "\"color\":\"" << canonical_escape(*part.color) << "\",";
            if (part.rotation_degrees) os << "\"rotation_degrees\":" << *part.rotation_degrees << ",";
            if (part.emissive) os << "\"emissive\":" << (*part.emissive ? "true" : "false") << ",";
            if (part.electrical_marking) os << "\"electrical_marking\":" << (*part.electrical_marking ? "true" : "false") << ",";
            if (part.z_order) os << "\"z_order\":" << *part.z_order << ",";
            if (part.bone_id && !part.bone_id->empty()) os << "\"bone_id\":\"" << canonical_escape(*part.bone_id) << "\",";
            if (part.primitive && !part.primitive->empty()) os << "\"primitive\":\"" << canonical_escape(*part.primitive) << "\",";
            if (part.semantic_role && !part.semantic_role->empty()) os << "\"semantic_role\":\"" << canonical_escape(*part.semantic_role) << "\",";
            os << "\"part_id\":\"" << canonical_escape(part.part_id) << "\"";
            os << "}";
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
                auto const& kf = tr.keys[k];
                os << "{\"tick\":" << kf.tick
                   << ",\"x\":" << kf.x << ",\"y\":" << kf.y
                   << ",\"rotation_degrees\":" << kf.rotation_degrees
                   << ",\"scale_x\":" << kf.scale_x << ",\"scale_y\":" << kf.scale_y;
                if (!kf.legacy_transform.empty())
                    os << ",\"value\":\"" << canonical_escape(kf.legacy_transform) << "\"";
                os << "}";
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
    std::set<std::string, std::less<>> seen_root;

    while (r.has_more() && !r.has_error()) {
        auto key_res = r.read_string_result();
        if (!key_res.ok() || !key_res.value) break;
        std::string key = std::move(*key_res.value);
        r.require(':', "$." + key + ": expected ':'");
        if (r.has_error()) break;

        // Duplicate field detection
        if (!seen_root.insert(key).second) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                "$.\"" + key + "\": duplicate field in root object", {});
            return result;
        }

        // Scalar fields with fail-closed propagation
        if (key == "schema_version") {
            auto v = r.read_string_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.schema_version: expected string", {}); return result; }
            ce.schema_version = std::move(*v.value);
        } else if (key == "stable_id") {
            auto v = r.read_string_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.stable_id: expected string", {}); return result; }
            ce.stable_id = std::move(*v.value);
        } else if (key == "name") {
            auto v = r.read_string_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.name: expected string", {}); return result; }
            ce.name = std::move(*v.value);
        } else if (key == "classification") {
            auto v = r.read_string_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.classification: expected string", {}); return result; }
            ce.classification = std::move(*v.value);
        } else if (key == "rights") {
            auto v = r.read_string_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.rights: expected string", {}); return result; }
            ce.rights = std::move(*v.value);
        } else if (key == "rights_allow_export") {
            auto v = r.read_bool_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.rights_allow_export: expected boolean", {}); return result; }
            ce.rights_allow_export = *v.value;
        } else if (key == "entropy_root") {
            auto v = r.read_uint64_result();
            if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.entropy_root: expected unsigned integer", {}); return result; }
            ce.entropy_root = *v.value;
        } else if (key == "primary_color") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.primary_color: expected string", {}); return result; }
            ce.primary_color = std::move(*v.value);
        } else if (key == "accent_color") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.accent_color: expected string", {}); return result; }
            ce.accent_color = std::move(*v.value);
        } else if (key == "storm_primary_color") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.storm_primary_color: expected string", {}); return result; }
            ce.storm_primary_color = std::move(*v.value);
        } else if (key == "storm_accent_color") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.storm_accent_color: expected string", {}); return result; }
            ce.storm_accent_color = std::move(*v.value);
        } else if (key == "emissive_color") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.emissive_color: expected string", {}); return result; }
            ce.emissive_color = std::move(*v.value);
        } else if (key == "aura_color") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.aura_color: expected string", {}); return result; }
            ce.aura_color = std::move(*v.value);
        } else if (key == "provenance_hash") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.provenance_hash: expected string", {}); return result; }
            ce.provenance_hash = std::move(*v.value);
        } else if (key == "provenance_source") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.provenance_source: expected string", {}); return result; }
            ce.provenance_source = std::move(*v.value);
        } else if (key == "initial_state") {
            auto v = r.read_string_result(); if (!v.ok() || !v.value) { result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH, "$.initial_state: expected string", {}); return result; }
            ce.initial_state = std::move(*v.value);
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
                    auto part = decode_canonical_override(r, "$.form_morphology_overrides." + form_name + "." + part_name);
                    if (!part.ok()) { result.diagnostics.merge(part.diagnostics); return result; }
                    if (part.value) { part.value->part_id = part_name; ce.form_morphology_overrides[form_name][part_name] = std::move(*part.value); }
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

    // Identity
    if (entity.stable_id.empty()) add(DiagnosticCode::GSPL_NAME_UNKNOWN, "stable_id is required");
    if (entity.name.empty()) add(DiagnosticCode::GSPL_NAME_UNKNOWN, "name is required");
    if (entity.rights.empty()) add(DiagnosticCode::GSPL_RIGHTS_VIOLATION, "rights classification is required");

    // Schema version
    if (entity.schema_version != "gspl.canonical-entity/1.0") {
        add(DiagnosticCode::GSPL_IR_UNSUPPORTED_VERSION,
            "unsupported schema_version: " + entity.schema_version);
    }

    // Forms — unique IDs, collision_scale nonfinite
    {
        std::set<std::string, std::less<>> ids;
        for (auto const& f : entity.forms) {
            if (f.id.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "form missing required 'id'"); continue; }
            if (!ids.insert(f.id).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate form id: " + f.id);
            if (!std::isfinite(f.collision_scale)) add(DiagnosticCode::GSPL_GENE_INVALID_VALUE, "form " + f.id + ": non-finite collision_scale");
            if (!std::isfinite(f.ability_envelope)) add(DiagnosticCode::GSPL_GENE_INVALID_VALUE, "form " + f.id + ": non-finite ability_envelope");
        }
    }

    // Transformations — unique IDs, valid from/to references
    {
        std::set<std::string, std::less<>> ids, form_ids;
        for (auto const& f : entity.forms) form_ids.insert(f.id);
        for (auto const& t : entity.transformations) {
            if (t.id.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "transformation missing required 'id'"); continue; }
            if (!ids.insert(t.id).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate transformation id: " + t.id);
            if (!t.from_form.empty() && !form_ids.contains(t.from_form))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "transformation " + t.id + " references unknown from_form: " + t.from_form);
            if (!t.to_form.empty() && !form_ids.contains(t.to_form))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "transformation " + t.id + " references unknown to_form: " + t.to_form);
        }
        // Form transformation_ids validity
        for (auto const& f : entity.forms) {
            for (auto const& tid : f.transformation_ids) {
                if (!ids.contains(tid))
                    add(DiagnosticCode::GSPL_NAME_UNKNOWN, "form " + f.id + " references unknown transformation: " + tid);
            }
        }
    }

    // Bones — unique IDs, valid parent references, no cycles
    {
        std::set<std::string, std::less<>> ids;
        for (auto const& b : entity.bones) {
            if (b.id.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "bone missing required 'id'"); continue; }
            if (!ids.insert(b.id).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate bone id: " + b.id);
            if (!b.parent.empty() && !ids.contains(b.parent))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "bone " + b.id + " references unknown parent: " + b.parent);
            if (b.id == b.parent) add(DiagnosticCode::GSPL_GENE_DEPENDENCY_CYCLE, "bone " + b.id + " references itself as parent");
        }
    }

    // Sockets — unique IDs, valid bone references
    {
        std::set<std::string, std::less<>> ids, bone_ids;
        for (auto const& b : entity.bones) bone_ids.insert(b.id);
        for (auto const& s : entity.sockets) {
            if (s.id.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "socket missing required 'id'"); continue; }
            if (!ids.insert(s.id).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate socket id: " + s.id);
            if (!s.bone.empty() && !bone_ids.contains(s.bone))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "socket " + s.id + " references unknown bone: " + s.bone);
        }
    }

    // Clips — unique names, track bone references
    {
        std::set<std::string, std::less<>> names;
        std::set<std::string, std::less<>> bone_ids;
        for (auto const& b : entity.bones) bone_ids.insert(b.id);
        for (auto const& c : entity.clips) {
            if (c.name.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "clip missing required 'name'"); continue; }
            if (!names.insert(c.name).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate clip name: " + c.name);
            for (auto const& t : c.tracks) {
                if (!t.bone.empty() && !bone_ids.contains(t.bone))
                    add(DiagnosticCode::GSPL_NAME_UNKNOWN, "clip " + c.name + " track references unknown bone: " + t.bone);
            }
        }
    }

    // States — unique names, initial_state existence, clip references
    {
        std::set<std::string, std::less<>> names, clip_names;
        for (auto const& c : entity.clips) clip_names.insert(c.name);
        for (auto const& s : entity.states) {
            if (s.name.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "state missing required 'name'"); continue; }
            if (!names.insert(s.name).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate state name: " + s.name);
            if (!s.clip_name.empty() && !clip_names.contains(s.clip_name))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "state " + s.name + " references unknown clip: " + s.clip_name);
        }
        if (!entity.initial_state.empty() && !names.contains(entity.initial_state))
            add(DiagnosticCode::GSPL_NAME_UNKNOWN, "initial_state references unknown state: " + entity.initial_state);
    }

    // Transitions — valid state/ability references
    {
        std::set<std::string, std::less<>> state_names, ability_ids;
        for (auto const& s : entity.states) state_names.insert(s.name);
        for (auto const& a : entity.abilities) ability_ids.insert(a.id);
        for (auto const& t : entity.transitions) {
            if (!t.from_state.empty() && !state_names.contains(t.from_state))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "transition references unknown from_state: " + t.from_state);
            if (!t.to_state.empty() && !state_names.contains(t.to_state))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "transition references unknown to_state: " + t.to_state);
            if (!t.ability_id.empty() && !ability_ids.contains(t.ability_id))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "transition references unknown ability: " + t.ability_id);
        }
    }

    // Collision shapes — unique IDs
    {
        std::set<std::string, std::less<>> ids;
        for (auto const& cs : entity.collision_shapes) {
            if (cs.id.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "collision_shape missing required 'id'"); continue; }
            if (!ids.insert(cs.id).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate collision_shape id: " + cs.id);
        }
    }

    // Collision windows — valid shape/ability references
    {
        std::set<std::string, std::less<>> shape_ids, ability_ids;
        for (auto const& cs : entity.collision_shapes) shape_ids.insert(cs.id);
        for (auto const& a : entity.abilities) ability_ids.insert(a.id);
        for (auto const& cw : entity.collision_windows) {
            if (!cw.shape_id.empty() && !shape_ids.contains(cw.shape_id))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "collision_window references unknown shape: " + cw.shape_id);
            if (!cw.ability_id.empty() && !ability_ids.contains(cw.ability_id))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "collision_window references unknown ability: " + cw.ability_id);
        }
    }

    // Resources — unique IDs, min/max/initial invariants
    {
        std::set<std::string, std::less<>> ids;
        for (auto const& r : entity.resources) {
            if (r.id.empty()) { add(DiagnosticCode::GSPL_NAME_UNKNOWN, "resource missing required 'id'"); continue; }
            if (!ids.insert(r.id).second) add(DiagnosticCode::GSPL_NAME_DUPLICATE, "duplicate resource id: " + r.id);
            if (r.min > r.max) add(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED, "resource " + r.id + ": min > max");
            if (r.initial < r.min || r.initial > r.max)
                add(DiagnosticCode::GSPL_CONSTRAINT_UNSATISFIED, "resource " + r.id + ": initial out of [min,max] range");
        }
    }

    // Runtime — animation intent state/clip references
    if (entity.runtime) {
        std::set<std::string, std::less<>> state_names, clip_names;
        for (auto const& s : entity.states) state_names.insert(s.name);
        for (auto const& c : entity.clips) clip_names.insert(c.name);
        for (auto const& ai : entity.runtime->animation_intents) {
            if (!ai.behavior_state.empty() && !state_names.contains(ai.behavior_state))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "runtime animation_intent references unknown state: " + ai.behavior_state);
            if (!ai.clip_name.empty() && !clip_names.contains(ai.clip_name))
                add(DiagnosticCode::GSPL_NAME_UNKNOWN, "runtime animation_intent references unknown clip: " + ai.clip_name);
        }
    }

    // Genes — unique kind+type combination, valid kind
    {
        std::set<std::string, std::less<>> gene_keys;
        for (auto const& g : entity.genes) {
            auto key = std::to_string(static_cast<std::uint32_t>(g.descriptor.kind)) + ":" + g.descriptor.type_id;
            if (!gene_keys.insert(key).second)
                add(DiagnosticCode::GSPL_GENE_DUPLICATE, "duplicate gene: kind=" + std::to_string(static_cast<std::uint32_t>(g.descriptor.kind)) + " type=" + g.descriptor.type_id);
        }
    }

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
        std::set<std::string> seen;
        for (auto const& attr : part->attributes) {
            auto const* a = dynamic_cast<AttributeNode const*>(attr.get());
            if (!a || !a->value) continue;
            auto const* lit = dynamic_cast<LiteralNode const*>(a->value.get());
            if (!lit) continue;
            if (!seen.insert(a->key).second) {
                out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    "part " + cp.name + ": duplicate field '" + a->key + "'", SourceSpan{});
                continue;
            }
            if (a->key == "x") cp.x = std::stod(lit->value);
            else if (a->key == "y") cp.y = std::stod(lit->value);
            else if (a->key == "z") cp.z = std::stod(lit->value);
            else if (a->key == "size_x") cp.size_x = std::stod(lit->value);
            else if (a->key == "size_y") cp.size_y = std::stod(lit->value);
            else if (a->key == "size_z") cp.size_z = std::stod(lit->value);
            else if (a->key == "color") cp.color = lit->value;
            else if (a->key == "rotation_degrees") cp.rotation_degrees = std::stod(lit->value);
            else if (a->key == "parent") cp.parent = lit->value;
            else if (a->key == "bone") cp.bone_id = lit->value;
            else if (a->key == "primitive") cp.primitive = lit->value;
            else if (a->key == "semantic_role") cp.semantic_role = lit->value;
            else if (a->key == "emissive") cp.emissive = (lit->value == "true");
            else if (a->key == "electrical_marking") cp.electrical_marking = (lit->value == "true");
            else if (a->key == "z_order") cp.z_order = static_cast<std::int32_t>(std::stoi(lit->value));
            else
                out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    "part " + cp.name + ": unsupported field '" + a->key + "'", SourceSpan{});
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

struct Uint32ParseResult { std::optional<std::uint32_t> value; DiagnosticResult diagnostics; bool ok() const { return value.has_value() && diagnostics.ok(); } };
static Uint32ParseResult parse_uint32_diagnostic(std::string const& s, std::string const& context) {
    Uint32ParseResult r;
    if (s.empty()) {
        r.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
            context + ": empty tick name", SourceSpan{});
        return r;
    }
    // Reject sign prefix
    if (s[0] == '-' || s[0] == '+') {
        r.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
            context + ": tick must be unsigned, got '" + s + "'", SourceSpan{});
        return r;
    }
    // Reject non-decimal characters
    for (char c : s) {
        if (c < '0' || c > '9') {
            r.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                context + ": tick must be unsigned decimal, got '" + s + "'", SourceSpan{});
            return r;
        }
    }
    // Parse with full token consumption
    std::uint32_t result = 0;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), result);
    if (ec == std::errc::result_out_of_range) {
        r.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
            context + ": tick overflow, got '" + s + "'", SourceSpan{});
        return r;
    }
    if (ec != std::errc{} || ptr != s.data() + s.size()) {
        r.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
            context + ": invalid tick '" + s + "'", SourceSpan{});
        return r;
    }
    r.value = result;
    return r;
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
        // Parse loop flag from attribute
        auto loop_str = get_attr("loop");
        if (!loop_str.empty()) {
            if (loop_str == "true") clip.loop = true;
            else if (loop_str == "false") clip.loop = false;
            else out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                "clip=" + clip.name + ": invalid loop value '" + loop_str + "' (expected true/false)", SourceSpan{});
        }
        // Parse event sub-blocks
        // Track used event names to reject duplicates
        std::set<std::string> used_event_names;
        std::set<std::pair<std::uint32_t, std::string>> used_event_tick_names;
        auto parse_event_block = [&](GenericBlock const& ev_block) {
            if (ev_block.block_type != "event" && ev_block.block_type != "Event") return;
            std::string event_id = ev_block.name;
            if (event_id.empty()) {
                out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    "clip=" + clip.name + ": event block has empty name", SourceSpan{});
                return;
            }
            // Reject duplicate event names
            if (!used_event_names.insert(event_id).second) {
                out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    "clip=" + clip.name + ": duplicate event '" + event_id + "'", SourceSpan{});
                return;
            }
            // Extract tick attribute
            std::uint32_t ev_tick = 0;
            bool has_tick = false;
            SourceSpan tick_span;
            for (auto const& ev_attr : ev_block.attributes) {
                auto const* attr = dynamic_cast<AttributeNode const*>(ev_attr.get());
                if (attr && attr->key == "tick" && attr->value) {
                    auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get());
                    if (lit) {
                        tick_span = lit->span;
                        auto tick_result = parse_uint32_diagnostic(lit->value,
                            "clip=" + clip.name + " event=" + event_id);
                        if (tick_result.ok()) {
                            ev_tick = *tick_result.value;
                            has_tick = true;
                        } else {
                            out.diagnostics.merge(tick_result.diagnostics);
                        }
                    }
                }
            }
            if (!has_tick) {
                out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    "clip=" + clip.name + " event=" + event_id + ": missing required field 'tick'", SourceSpan{});
                return;
            }
            // Reject duplicate tick+name pair
            if (!used_event_tick_names.insert({ev_tick, event_id}).second) {
                out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                    "clip=" + clip.name + ": duplicate tick+event pair '" + event_id + "' at tick " + std::to_string(ev_tick), tick_span);
                return;
            }
            clip.clip_events.push_back({ev_tick, event_id});
        };
        for (auto const& attr_ptr : block.attributes) {
            auto const* gb = dynamic_cast<GenericBlock const*>(attr_ptr.get());
            if (gb) {
                if (gb->block_type == "event" || gb->block_type == "Event") {
                    parse_event_block(*gb);
                    continue;
                }
                if (gb->block_type == "track" || gb->block_type == "Track") {
                    CanonicalAnimationClip::Track track;
                    track.bone = gb->name;
                    // Process key blocks first (typed keyframe syntax)
                    for (auto const& child_ptr : gb->attributes) {
                        auto const* child_gb = dynamic_cast<GenericBlock const*>(child_ptr.get());
                        if (child_gb && (child_gb->block_type == "key")) {
                            // Fail-closed tick parsing: validate name as uint32
                            auto tick_result = parse_uint32_diagnostic(child_gb->name,
                                "clip=" + clip.name + " track=" + track.bone);
                            if (!tick_result.ok()) {
                                out.diagnostics.merge(tick_result.diagnostics);
                                continue;
                            }
                            CanonicalKeyframe kf;
                            kf.tick = *tick_result.value;
                        auto get_key_attr = [&](std::string const& key) -> std::string {
                            for (auto const& a : child_gb->attributes) {
                                auto const* attr = dynamic_cast<AttributeNode const*>(a.get());
                                if (attr && attr->key == key && attr->value) {
                                    auto const* lit = dynamic_cast<LiteralNode const*>(attr->value.get());
                                    if (lit) return lit->value;
                                }
                            }
                            return "";
                        };
                        // Fail-closed numeric parsing: produce diagnostics on malformed input
                        auto fail = [&](std::string const& msg) {
                            out.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                "clip=" + clip.name + " track=" + track.bone +
                                " tick=" + child_gb->name + ": " + msg, SourceSpan{});
                        };
                        auto pd = [&](std::string const& key, double def, bool required) -> std::optional<double> {
                            auto v = get_key_attr(key);
                            if (v.empty()) {
                                if (required) fail("missing required field '" + key + "'");
                                return required ? std::nullopt : std::optional<double>(def);
                            }
                            double result = 0.0;
                            auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), result);
                            if (ec != std::errc{}) {
                                fail("invalid numeric value for '" + key + "': " + v);
                                return std::nullopt;
                            }
                            if (ptr != v.data() + v.size()) {
                                fail("trailing characters in '" + key + "': " + v);
                                return std::nullopt;
                            }
                            if (!std::isfinite(result)) {
                                fail("non-finite value for '" + key + "'");
                                return std::nullopt;
                            }
                            return result;
                        };
                        auto ox = pd("x", 0.0, true);
                        auto oy = pd("y", 0.0, true);
                        auto orot = pd("rotation_degrees", 0.0, true);
                        auto osx = pd("scale_x", 1.0, true);
                        auto osy = pd("scale_y", 1.0, true);
                        if (!ox || !oy || !orot || !osx || !osy) continue;
                        if (*osx <= 0.0) { fail("scale_x must be strictly positive"); continue; }
                        if (*osy <= 0.0) { fail("scale_y must be strictly positive"); continue; }
                        // Check for duplicate/unsupported fields
                        std::set<std::string> seen;
                        for (auto const& a : child_gb->attributes) {
                            auto const* attr = dynamic_cast<AttributeNode const*>(a.get());
                            if (attr) {
                                if (!seen.insert(attr->key).second)
                                    fail("duplicate field '" + attr->key + "'");
                                else if (attr->key != "x" && attr->key != "y" &&
                                         attr->key != "rotation_degrees" &&
                                         attr->key != "scale_x" && attr->key != "scale_y")
                                    fail("unsupported keyframe field '" + attr->key + "'");
                            }
                        }
                        kf.x = *ox; kf.y = *oy;
                        kf.rotation_degrees = *orot;
                        kf.scale_x = *osx; kf.scale_y = *osy;
                        track.keys.push_back(std::move(kf));
                    }
                }
                // Legacy tick-only fallback removed — all tracks must use typed keyframe syntax
                clip.tracks.push_back(std::move(track));
            }
            }  // close if (gb)
        }
        out.clips.push_back(std::move(clip));
    } else if (block.block_type == "state" || block.block_type == "State") {
        CanonicalAnimationState state;
        state.name = block.name;
        state.clip_name = get_attr("clip");
        if (out.initial_state.empty()) out.initial_state = state.name;
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
