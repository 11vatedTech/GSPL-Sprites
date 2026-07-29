#include "gspl/ir.hpp"
#include "gspl/genes.hpp"
#include "gspl/json.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <set>
#include <sstream>

namespace gspl {
namespace {

// ── minimal bounded JSON helpers for serialization ─────────

std::string ir_json_escape(std::string_view sv) {
    std::ostringstream out;
    for (auto c : sv) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default: out << c;
        }
    }
    return out.str();
}

void json_append(std::ostringstream& ss, std::string_view key, std::string_view value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << ir_json_escape(key) << "\": \"" << ir_json_escape(value) << "\"";
}

void json_append_int(std::ostringstream& ss, std::string_view key, auto value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << ir_json_escape(key) << "\": " << value;
}

void json_append_bool(std::ostringstream& ss, std::string_view key, bool value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << ir_json_escape(key) << "\": " << (value ? "true" : "false");
}

std::string serialize_entity_ir(EntityIr const& entity) {
    std::ostringstream ss;
    ss << "{\n";
    bool first = true;
    json_append_int(ss, "kind", static_cast<std::uint32_t>(entity.kind), first);
    json_append(ss, "identity", entity.identity, first);
    json_append_int(ss, "schema_version", entity.schema_version, first);
    json_append(ss, "entity_id", entity.entity_id, first);

    // genes
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"genes\": [";
    bool first_gene = true;
    for (auto const& g : entity.genes) {
        if (!first_gene) ss << ", ";
        first_gene = false;
        ss << "{\"kind\":" << static_cast<std::uint32_t>(g.descriptor.kind)
           << ",\"schema\":" << g.descriptor.schema_version
           << ",\"type\":\"" << ir_json_escape(g.descriptor.type_id) << "\""
           << ",\"source\":\"" << ir_json_escape(g.source_module) << "\""
           << ",\"values\":{";
        bool first_val = true;
        for (auto const& [k, v] : g.values) {
            if (!first_val) ss << ",";
            first_val = false;
            auto tag = static_cast<std::uint32_t>(gene_value_variant_index(v));
            ss << "\"" << ir_json_escape(k) << "\":{\"t\":" << tag
               << ",\"v\":" << gene_value_to_json(v) << "}";
        }
        ss << "}}";
    }
    ss << "]";

    // properties
    ss << ",\n  \"properties\": {";
    bool first_prop = true;
    for (auto const& [k, v] : entity.properties) {
        if (!first_prop) ss << ", ";
        first_prop = false;
        ss << "\"" << ir_json_escape(k) << "\":\"" << ir_json_escape(v) << "\"";
    }
    ss << "}";

    // dependency_ids
    ss << ",\n  \"dependency_ids\": [";
    bool first_dep = true;
    for (auto const& d : entity.dependency_ids) {
        if (!first_dep) ss << ", ";
        first_dep = false;
        ss << "\"" << ir_json_escape(d) << "\"";
    }
    ss << "]";

    // children
    ss << ",\n  \"children\": [";
    bool first_child = true;
    for (auto const& child : entity.children) {
        if (!first_child) ss << ",\n  ";
        first_child = false;
        ss << "{\"kind\":" << static_cast<std::uint32_t>(child->kind)
           << ",\"identity\":\"" << ir_json_escape(child->identity) << "\""
           << ",\"schema_version\":" << child->schema_version;
        if (!child->properties.empty()) {
            ss << ",\"properties\":{";
            bool fp = true;
            for (auto const& [k, v] : child->properties) {
                if (!fp) ss << ",";
                fp = false;
                ss << "\"" << ir_json_escape(k) << "\":\"" << ir_json_escape(v) << "\"";
            }
            ss << "}";
        }
        if (!child->dependency_ids.empty()) {
            ss << ",\"dependency_ids\":[";
            bool fd = true;
            for (auto const& d : child->dependency_ids) {
                if (!fd) ss << ",";
                fd = false;
                ss << "\"" << ir_json_escape(d) << "\"";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]\n";
    ss << "}";
    return ss.str();
}

} // namespace

// ── IrSerializer ────────────────────────────────────────────

std::string IrSerializer::serialize(SpriteIr const& ir) {
    std::ostringstream ss;
    ss << "{\n";
    bool first = true;
    json_append(ss, "ir_version", ir.ir_version, first);
    json_append(ss, "entity_id", ir.entity_id, first);
    json_append(ss, "seed_identity", ir.seed_identity, first);
    json_append_int(ss, "schema_version", 1, first);

    if (ir.entity) {
        ss << ",\n  \"entity\": " << serialize_entity_ir(*ir.entity);
    }

    // representations - serialize full node structure
    ss << ",\n  \"representations\": [";
    for (std::size_t i = 0; i < ir.representations.size(); ++i) {
        if (i > 0) ss << ",";
        auto const& rep = ir.representations[i];
        ss << "{\"kind\":" << static_cast<std::uint32_t>(rep->kind)
           << ",\"identity\":\"" << ir_json_escape(rep->identity) << "\""
           << ",\"schema_version\":" << rep->schema_version;
        if (!rep->properties.empty()) {
            ss << ",\"properties\":{";
            bool fp = true;
            for (auto const& [k, v] : rep->properties) {
                if (!fp) ss << ",";
                fp = false;
                ss << "\"" << ir_json_escape(k) << "\":\"" << ir_json_escape(v) << "\"";
            }
            ss << "}";
        }
        if (!rep->dependency_ids.empty()) {
            ss << ",\"dependency_ids\":[";
            bool fd = true;
            for (auto const& d : rep->dependency_ids) {
                if (!fd) ss << ",";
                fd = false;
                ss << "\"" << ir_json_escape(d) << "\"";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]";

    ss << ",\n  \"runtime_plans\": [";
    for (std::size_t i = 0; i < ir.runtime_plans.size(); ++i) {
        if (i > 0) ss << ",";
        auto const& plan = ir.runtime_plans[i];
        ss << "{\"kind\":" << static_cast<std::uint32_t>(plan->kind)
           << ",\"identity\":\"" << ir_json_escape(plan->identity) << "\""
           << ",\"schema_version\":" << plan->schema_version;
        if (!plan->properties.empty()) {
            ss << ",\"properties\":{";
            bool fp = true;
            for (auto const& [k, v] : plan->properties) {
                if (!fp) ss << ",";
                fp = false;
                ss << "\"" << ir_json_escape(k) << "\":\"" << ir_json_escape(v) << "\"";
            }
            ss << "}";
        }
        if (!plan->dependency_ids.empty()) {
            ss << ",\"dependency_ids\":[";
            bool fd = true;
            for (auto const& d : plan->dependency_ids) {
                if (!fd) ss << ",";
                fd = false;
                ss << "\"" << ir_json_escape(d) << "\"";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]";

    ss << ",\n  \"package_plans\": [";
    for (std::size_t i = 0; i < ir.package_plans.size(); ++i) {
        if (i > 0) ss << ",";
        auto const& plan = ir.package_plans[i];
        ss << "{\"kind\":" << static_cast<std::uint32_t>(plan->kind)
           << ",\"identity\":\"" << ir_json_escape(plan->identity) << "\""
           << ",\"schema_version\":" << plan->schema_version;
        if (!plan->properties.empty()) {
            ss << ",\"properties\":{";
            bool fp = true;
            for (auto const& [k, v] : plan->properties) {
                if (!fp) ss << ",";
                fp = false;
                ss << "\"" << ir_json_escape(k) << "\":\"" << ir_json_escape(v) << "\"";
            }
            ss << "}";
        }
        if (!plan->dependency_ids.empty()) {
            ss << ",\"dependency_ids\":[";
            bool fd = true;
            for (auto const& d : plan->dependency_ids) {
                if (!fd) ss << ",";
                fd = false;
                ss << "\"" << ir_json_escape(d) << "\"";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]";
    ss << "\n}";
    return ss.str();
}

// ── Enum and kind validation ──────────────────────────────

namespace {
// Compile-time coverage: update these when enums change
static_assert(static_cast<std::uint32_t>(GeneKind::provenance) == 34,
              "GeneKind::provenance must be the terminal enumerator");
static_assert(static_cast<std::uint32_t>(GeneValueTag::string_list_val) == 5,
              "GeneValueTag::string_list_val must be the terminal enumerator");
static_assert(static_cast<std::uint32_t>(IrNodeKind::target_requirement) == 16,
              "IrNodeKind::target_requirement must be the terminal enumerator");

bool is_valid_gene_kind(std::int64_t raw) noexcept {
    return raw >= 0 &&
           raw <= static_cast<std::int64_t>(GeneKind::provenance);
}

bool is_valid_gene_value_tag(std::int64_t raw) noexcept {
    return raw >= 0 &&
           raw <= static_cast<std::int64_t>(GeneValueTag::string_list_val);
}

bool is_valid_ir_node_kind(std::int64_t raw) noexcept {
    return raw >= 0 &&
           raw <= static_cast<std::int64_t>(IrNodeKind::target_requirement);
}

// Collection-specific kind rules
bool is_allowed_in_representations(IrNodeKind kind) noexcept {
    return kind == IrNodeKind::representation_plan;
}

bool is_allowed_in_runtime_plans(IrNodeKind kind) noexcept {
    return kind == IrNodeKind::runtime_plan;
}

bool is_allowed_in_package_plans(IrNodeKind kind) noexcept {
    return kind == IrNodeKind::package_plan;
}

bool is_allowed_as_entity_child(IrNodeKind kind) noexcept {
    switch (kind) {
    case IrNodeKind::gene:
    case IrNodeKind::structure:
    case IrNodeKind::form:
    case IrNodeKind::transformation:
    case IrNodeKind::material:
    case IrNodeKind::animation:
    case IrNodeKind::behavior:
    case IrNodeKind::ability:
    case IrNodeKind::projectile:
    case IrNodeKind::effect:
    case IrNodeKind::collision:
    case IrNodeKind::resource:
        return true;
    default:
        return false;
    }
}
} // namespace

SpriteIrDeserializeResult IrSerializer::deserialize(std::string_view json, BoundedJsonConfig config) {
    SpriteIrDeserializeResult result;
    SpriteIr ir;

    if (json.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     "deserialize: empty input", {});
        return result;
    }

    BoundedJsonReader r(json, config);
    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     r.error_message(), {});
        return result;
    }

    // ── Helper lambdas for fail-closed parsing ────────────
    auto fail = [&](const char* msg) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, msg, {});
    };

    auto read_str = [&](const char* ctx) -> std::optional<std::string> {
        auto res = r.read_string_result();
        if (!res.ok()) {
            result.diagnostics.merge(res.diagnostics);
            fail((std::string("deserialize: ") + ctx + " — failed to read string").c_str());
            return std::nullopt;
        }
        return res.value;
    };

    auto read_i64 = [&](const char* ctx) -> std::optional<std::int64_t> {
        auto res = r.read_int64_result();
        if (!res.ok()) {
            result.diagnostics.merge(res.diagnostics);
            fail((std::string("deserialize: ") + ctx + " — failed to read int64").c_str());
            return std::nullopt;
        }
        return res.value;
    };

    auto read_u32 = [&](const char* ctx) -> std::optional<std::uint32_t> {
        auto res = r.read_uint32_result();
        if (!res.ok()) {
            result.diagnostics.merge(res.diagnostics);
            fail((std::string("deserialize: ") + ctx + " — failed to read uint32").c_str());
            return std::nullopt;
        }
        return res.value;
    };

    auto require = [&](char expected, const char* ctx) {
        if (!r.require(expected, ctx)) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                         r.error_message(), {});
        }
    };

    // Separator handling now uses governed r.next_object_member() / r.next_array_element()
    // which reject trailing commas and missing commas with diagnostic errors.

    auto parse_ir_node = [&](IrNode& node, const char* label) -> bool {
        if (!r.begin_object(label)) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                         r.error_message(), {});
            return false;
        }
        bool saw_kind = false, saw_identity = false, saw_schema = false, saw_props = false, saw_deps = false;
        while (r.has_more() && !r.has_error()) {
            auto key_opt = read_str(label);
            if (!key_opt) return false;
            auto& key = *key_opt;
            require(':', label);
            if (r.has_error()) return false;

            if (key == "kind") {
                if (saw_kind) { fail(("deserialize: duplicate 'kind' in " + std::string(label)).c_str()); return false; }
                auto kind_opt = read_i64(label);
                if (!kind_opt) return false;
                if (!is_valid_ir_node_kind(*kind_opt)) {
                    fail(("deserialize: unknown " + std::string(label) + " node kind: " + std::to_string(*kind_opt)).c_str());
                    return false;
                }
                node.kind = static_cast<IrNodeKind>(*kind_opt);
                saw_kind = true;
            } else if (key == "identity") {
                if (saw_identity) { fail(("deserialize: duplicate 'identity' in " + std::string(label)).c_str()); return false; }
                auto id_opt = read_str(label);
                if (!id_opt) return false;
                node.identity = std::move(*id_opt);
                saw_identity = true;
            } else if (key == "schema_version") {
                if (saw_schema) { fail(("deserialize: duplicate 'schema_version' in " + std::string(label)).c_str()); return false; }
                auto sv_opt = read_u32(label);
                if (!sv_opt) return false;
                if (*sv_opt == 0) { fail(("deserialize: zero schema_version in " + std::string(label)).c_str()); return false; }
                node.schema_version = *sv_opt;
                saw_schema = true;
            } else if (key == "properties") {
                if (saw_props) { fail(("deserialize: duplicate 'properties' in " + std::string(label)).c_str()); return false; }
                if (!r.begin_object(label)) { result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {}); return false; }
                while (r.has_more() && !r.has_error()) {
                    auto pk_opt = read_str(label);
                    if (!pk_opt) return false;
                    require(':', label);
                    if (r.has_error()) return false;
                    auto pv_opt = read_str(label);
                    if (!pv_opt) return false;
                    node.properties[*pk_opt] = std::move(*pv_opt);
                    r.record_object_member(label);
                    if (!r.next_object_member(label)) break;
                }
                if (!r.end_object(label)) { result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {}); return false; }
                saw_props = true;
            } else if (key == "dependency_ids") {
                if (saw_deps) { fail(("deserialize: duplicate 'dependency_ids' in " + std::string(label)).c_str()); return false; }
                if (!r.begin_array(label)) { result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {}); return false; }
                while (r.has_more() && !r.has_error()) {
                    auto dep_opt = read_str(label);
                    if (!dep_opt) return false;
                    node.dependency_ids.push_back(std::move(*dep_opt));
                    r.record_array_element(label);
                    if (!r.next_array_element(label)) break;
                }
                if (!r.end_array(label)) { result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {}); return false; }
                saw_deps = true;
            } else {
                r.skip_value();
            }
            if (!r.next_object_member(label)) break;
        }
        if (!r.end_object(label)) { result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {}); return false; }
        // Required field checks for generic nodes
        if (!saw_kind) { fail(("deserialize: missing required 'kind' in " + std::string(label)).c_str()); return false; }
        if (!saw_identity || node.identity.empty()) { fail(("deserialize: missing or empty 'identity' in " + std::string(label)).c_str()); return false; }
        if (!saw_schema) { fail(("deserialize: missing required 'schema_version' in " + std::string(label)).c_str()); return false; }
        return !r.has_error();
    };

    // ── Root object ──────────────────────────────────────
    if (!r.begin_object("deserialize: root")) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     r.error_message(), {});
        return result;
    }

    bool parsed_any = false;
    bool saw_ir_version = false, saw_entity_id = false, saw_seed_identity = false;
    std::uint32_t root_schema_version = 0;

    while (r.has_more() && !r.has_error()) {
        auto key_opt = read_str("deserialize: root key");
        if (!key_opt) return result;
        auto& key = *key_opt;
        require(':', "deserialize: ':' after root key");
        if (r.has_error()) return result;

        parsed_any = true;

        if (key == "ir_version") {
            if (saw_ir_version) { fail("deserialize: duplicate 'ir_version' in root"); return result; }
            saw_ir_version = true;
            auto v_opt = read_str("ir_version");
            if (!v_opt) return result;
            ir.ir_version = std::move(*v_opt);
        } else if (key == "entity_id") {
            if (saw_entity_id) { fail("deserialize: duplicate 'entity_id' in root"); return result; }
            saw_entity_id = true;
            auto v_opt = read_str("entity_id");
            if (!v_opt) return result;
            ir.entity_id = std::move(*v_opt);
        } else if (key == "seed_identity") {
            if (saw_seed_identity) { fail("deserialize: duplicate 'seed_identity' in root"); return result; }
            saw_seed_identity = true;
            auto v_opt = read_str("seed_identity");
            if (!v_opt) return result;
            ir.seed_identity = std::move(*v_opt);
        } else if (key == "schema_version") {
            auto v_opt = read_u32("root.schema_version");
            if (!v_opt) return result;
            root_schema_version = *v_opt;
        } else if (key == "entity") {
            if (!r.begin_object("entity")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                             r.error_message(), {});
                return result;
            }
            ir.entity = std::make_unique<EntityIr>();
            bool saw_ent_kind = false, saw_ent_id = false, saw_ent_schema = false, saw_ent_identity = false;
            while (r.has_more() && !r.has_error()) {
                auto ek_opt = read_str("entity key");
                if (!ek_opt) return result;
                auto& ek = *ek_opt;
                require(':', ("deserialize: ':' after entity key '" + ek + "'").c_str());
                if (r.has_error()) return result;

                if (ek == "kind") {
                    if (saw_ent_kind) { fail("deserialize: duplicate 'kind' in entity"); return result; }
                    auto kind_opt = read_i64("entity.kind");
                    if (!kind_opt) return result;
                    if (static_cast<IrNodeKind>(*kind_opt) != IrNodeKind::entity) {
                        fail(("deserialize: entity kind must be 'entity' (0), got: " + std::to_string(*kind_opt)).c_str());
                        return result;
                    }
                    ir.entity->kind = IrNodeKind::entity;
                    saw_ent_kind = true;
                } else                if (ek == "identity") {
                    if (saw_ent_identity) { fail("deserialize: duplicate 'identity' in entity"); return result; }
                    auto v_opt = read_str("entity.identity");
                    if (!v_opt) return result;
                    ir.entity->identity = std::move(*v_opt);
                    saw_ent_identity = true;
                } else if (ek == "entity_id") {
                    if (saw_ent_id) { fail("deserialize: duplicate 'entity_id' in entity"); return result; }
                    auto v_opt = read_str("entity.entity_id");
                    if (!v_opt) return result;
                    ir.entity->entity_id = std::move(*v_opt);
                    saw_ent_id = true;
                } else if (ek == "schema_version") {
                    if (saw_ent_schema) { fail("deserialize: duplicate 'schema_version' in entity"); return result; }
                    auto v_opt = read_u32("entity.schema_version");
                    if (!v_opt) return result;
                    if (*v_opt == 0) { fail("deserialize: entity schema_version must be >= 1"); return result; }
                    ir.entity->schema_version = *v_opt;
                    saw_ent_schema = true;
                } else if (ek == "dependency_ids") {
                    if (!r.begin_array("entity.dependency_ids")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                    while (r.has_more() && !r.has_error()) {
                        auto dep_opt = read_str("entity.dependency_ids[]");
                        if (!dep_opt) return result;
                        ir.entity->dependency_ids.push_back(std::move(*dep_opt));
                        r.record_array_element("entity.dependency_ids");
                        if (!r.next_array_element("entity.dependency_ids")) break;
                    }
                    if (!r.end_array("entity.dependency_ids")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                } else if (ek == "properties") {
                    if (!r.begin_object("entity.properties")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                    while (r.has_more() && !r.has_error()) {
                        auto pk_opt = read_str("entity.properties key");
                        if (!pk_opt) return result;
                        require(':', ("entity.properties['" + *pk_opt + "']").c_str());
                        if (r.has_error()) return result;
                        auto pv_opt = read_str("entity.properties value");
                        if (!pv_opt) return result;
                        ir.entity->properties[*pk_opt] = std::move(*pv_opt);
                        r.record_object_member("entity.properties");
                        if (!r.next_object_member("entity.properties")) break;
                    }
                    if (!r.end_object("entity.properties")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                } else if (ek == "children") {
                    if (!r.begin_array("entity.children")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                    while (r.has_more() && !r.has_error()) {
                        auto child = std::make_unique<IrNode>();
                        if (!parse_ir_node(*child, "entity.children[]")) return result;
                        if (!is_allowed_as_entity_child(child->kind)) {
                            fail(("deserialize: node kind " + std::to_string(static_cast<std::uint32_t>(child->kind)) + " not allowed as entity child").c_str());
                            return result;
                        }
                        ir.entity->children.push_back(std::move(child));
                        r.record_array_element("entity.children");
                        if (!r.next_array_element("entity.children")) break;
                    }
                    if (!r.end_array("entity.children")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                } else if (ek == "genes") {
                    if (!r.begin_array("entity.genes")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                    GeneRegistry registry;
                    while (r.has_more() && !r.has_error()) {
                        if (!r.begin_object("entity.genes[]")) {
                            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                            return result;
                        }
                        GeneInstance gi;
                        bool saw_kind = false, saw_schema = false, saw_type = false, saw_source = false, saw_values = false;
                        while (r.has_more() && !r.has_error()) {
                            auto gk_opt = read_str("gene key");
                            if (!gk_opt) return result;
                            auto& gk = *gk_opt;
                            require(':', ("gene.'" + gk + "'").c_str());
                            if (r.has_error()) return result;

                            if (gk == "kind") {
                                if (saw_kind) { fail("deserialize: duplicate 'kind' in gene"); return result; }
                                auto kv_opt = read_i64("gene.kind");
                                if (!kv_opt) return result;
                                if (!is_valid_gene_kind(*kv_opt)) {
                                    fail(("deserialize: unknown gene kind: " + std::to_string(*kv_opt)).c_str()); return result;
                                }
                                auto kind_val = static_cast<GeneKind>(*kv_opt);
                                auto const* desc = registry.lookup(kind_val);
                                if (!desc) {
                                    fail(("deserialize: unregistered gene kind " + std::to_string(*kv_opt) + " — not in GeneRegistry").c_str());
                                    return result;
                                }
                                gi.descriptor = *desc;
                                saw_kind = true;
                            } else if (gk == "schema") {
                                if (saw_schema) { fail("deserialize: duplicate 'schema' in gene"); return result; }
                                auto sv_opt = read_u32("gene.schema");
                                if (!sv_opt) return result;
                                gi.descriptor.schema_version = *sv_opt;
                                saw_schema = true;
                            } else if (gk == "type") {
                                if (saw_type) { fail("deserialize: duplicate 'type' in gene"); return result; }
                                auto t_opt = read_str("gene.type");
                                if (!t_opt) return result;
                                if (t_opt->empty()) { fail("deserialize: gene type_id must not be empty"); return result; }
                                gi.descriptor.type_id = std::move(*t_opt);
                                saw_type = true;
                            } else if (gk == "source") {
                                if (saw_source) { fail("deserialize: duplicate 'source' in gene"); return result; }
                                auto s_opt = read_str("gene.source");
                                if (!s_opt) return result;
                                gi.source_module = std::move(*s_opt);
                                saw_source = true;
                            } else if (gk == "values") {
                                if (saw_values) { fail("deserialize: duplicate 'values' in gene"); return result; }
                                if (!r.begin_object("gene.values")) {
                                    result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                                    return result;
                                }
                                while (r.has_more() && !r.has_error()) {
                                    auto vk_opt = read_str("gene.value key");
                                    if (!vk_opt) return result;
                                    require(':', ("gene.values['" + *vk_opt + "']").c_str());
                                    if (r.has_error()) return result;
                                    // Gene values must be typed {t, v} wrappers in current schema
                                    if (!r.begin_object("gene.value")) {
                                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                            (std::string("gene value '") + *vk_opt + "' must be a typed {t, v} object — " + r.error_message()).c_str(), {});
                                        return result;
                                    }
                                    bool saw_tag = false, saw_value = false;
                                    std::uint32_t tv_opt_val = 0;
                                    std::string raw_val;
                                    while (r.has_more() && !r.has_error()) {
                                        auto tk_opt = read_str("gene.value tagged key");
                                        if (!tk_opt) return result;
                                        auto& tk = *tk_opt;
                                        require(':', ("gene.value.'" + tk + "'").c_str());
                                        if (r.has_error()) return result;
                                        if (tk == "t") {
                                            if (saw_tag) { fail("deserialize: duplicate 't' in gene value wrapper"); return result; }
                                            auto tv_opt = read_u32("gene.value.t");
                                            if (!tv_opt) return result;
                                            tv_opt_val = *tv_opt;
                                            saw_tag = true;
                                        } else if (tk == "v") {
                                            if (saw_value) { fail("deserialize: duplicate 'v' in gene value wrapper"); return result; }
                                            raw_val = r.read_typed_value();
                                            if (raw_val.empty()) { fail("deserialize: empty gene value raw capture"); return result; }
                                            saw_value = true;
                                        } else {
                                            fail(("deserialize: unknown field '" + tk + "' in gene value wrapper").c_str());
                                            return result;
                                        }
                                        if (!r.next_object_member("gene.value")) break;
                                    }
                                    if (!r.end_object("gene.value")) {
                                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                                        return result;
                                    }
                                    if (!saw_tag) { fail("deserialize: gene value missing required 't' field"); return result; }
                                    if (!saw_value) { fail("deserialize: gene value missing required 'v' field"); return result; }
                                    if (!is_valid_gene_value_tag(tv_opt_val)) {
                                        fail(("deserialize: unknown gene value tag: " + std::to_string(tv_opt_val)).c_str()); return result;
                                    }
                                    auto tag = static_cast<GeneValueTag>(tv_opt_val);
                                    auto gv_result = json_to_gene_value_result(raw_val, tag);
                                    if (!gv_result.ok()) {
                                        result.diagnostics = gv_result.diagnostics;
                                        return result;
                                    }
                                    // Reject duplicate gene value names
                                    if (gi.values.find(*vk_opt) != gi.values.end()) {
                                        fail(("deserialize: duplicate gene value name '" + *vk_opt + "'").c_str());
                                        return result;
                                    }
                                    gi.values[*vk_opt] = *gv_result.value;
                                    r.record_object_member("gene.values");
                                    if (!r.next_object_member("gene.values")) break;
                                }
                                if (!r.end_object("gene.values")) {
                                    result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                                    return result;
                                }
                                saw_values = true;
                            } else {
                                r.skip_value();
                            }
                            if (!r.next_object_member("gene")) break;
                        }
                        if (!r.end_object("gene")) {
                            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                            return result;
                        }
                        // Reject incomplete genes — all 5 fields required
                        if (!saw_kind) { fail("deserialize: gene missing required 'kind'"); return result; }
                        if (!saw_schema) { fail("deserialize: gene missing required 'schema'"); return result; }
                        if (!saw_type) { fail("deserialize: gene missing required 'type'"); return result; }
                        if (!saw_source) { fail("deserialize: gene missing required 'source'"); return result; }
                        if (!saw_values) { fail("deserialize: gene missing required 'values'"); return result; }
                        ir.entity->genes.push_back(std::move(gi));
                        r.record_array_element("entity.genes");
                        if (!r.next_array_element("entity.genes")) break;
                    }
                    if (!r.end_array("entity.genes")) {
                        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                        return result;
                    }
                } else {
                    r.skip_value();
                }
                if (!r.next_object_member("entity")) break;
            }
            if (!r.end_object("entity")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                             r.error_message(), {});
                return result;
            }
            // Require entity kind to be present and correct
            if (!saw_ent_kind) { fail("deserialize: missing required 'kind' in entity"); return result; }
            if (ir.entity->kind != IrNodeKind::entity) {
                fail("deserialize: entity kind must be 'entity' (0)"); return result;
            }
            if (ir.entity->identity.empty()) { fail("deserialize: missing required entity identity"); return result; }
            if (ir.entity->entity_id.empty()) { fail("deserialize: missing required entity entity_id"); return result; }
        } else if (key == "representations") {                    if (!r.begin_array("representations")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                return result;
            }
            while (r.has_more() && !r.has_error()) {
                auto rep = std::make_unique<IrNode>();
                if (!parse_ir_node(*rep, "representations[]")) return result;
                if (!is_allowed_in_representations(rep->kind)) {
                    fail(("deserialize: node kind " + std::to_string(static_cast<std::uint32_t>(rep->kind)) + " not allowed in representations (expected representation_plan)").c_str());
                    return result;
                }
                ir.representations.push_back(std::move(rep));
                r.record_array_element("representations");
                if (!r.next_array_element("representations")) break;
            }
            if (!r.end_array("representations")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                return result;
            }
        } else if (key == "runtime_plans") {                    if (!r.begin_array("runtime_plans")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                return result;
            }
            while (r.has_more() && !r.has_error()) {
                auto plan = std::make_unique<IrNode>();
                if (!parse_ir_node(*plan, "runtime_plans[]")) return result;
                if (!is_allowed_in_runtime_plans(plan->kind)) {
                    fail(("deserialize: node kind " + std::to_string(static_cast<std::uint32_t>(plan->kind)) + " not allowed in runtime_plans (expected runtime_plan)").c_str());
                    return result;
                }
                ir.runtime_plans.push_back(std::move(plan));
                r.record_array_element("runtime_plans");
                if (!r.next_array_element("runtime_plans")) break;
            }
            if (!r.end_array("runtime_plans")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                return result;
            }
        } else if (key == "package_plans") {                    if (!r.begin_array("package_plans")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                return result;
            }
            while (r.has_more() && !r.has_error()) {
                auto plan = std::make_unique<IrNode>();
                if (!parse_ir_node(*plan, "package_plans[]")) return result;
                if (!is_allowed_in_package_plans(plan->kind)) {
                    fail(("deserialize: node kind " + std::to_string(static_cast<std::uint32_t>(plan->kind)) + " not allowed in package_plans (expected package_plan)").c_str());
                    return result;
                }
                ir.package_plans.push_back(std::move(plan));
                r.record_array_element("package_plans");
                if (!r.next_array_element("package_plans")) break;
            }
            if (!r.end_array("package_plans")) {
                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, r.error_message(), {});
                return result;
            }
        } else {
            r.skip_value();
        }
        if (!r.next_object_member("root")) break;
    }

    // Root object closure
    if (!r.end_object("root")) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     r.error_message(), {});
        return result;
    }

    // Trailing data check (root '}' already consumed by end_object above)
    r.skip_ws();
    if (r.position() < r.source().size()) {
        fail("deserialize: trailing data after root '}'"); return result;
    }

    if (r.has_error()) {
        fail(r.error_message().c_str()); return result;
    }
    if (!parsed_any) {
        fail("deserialize: no fields parsed"); return result;
    }

    // Fail-closed: validate root schema_version
    if (root_schema_version != 1) {
        fail(("deserialize: unsupported root schema_version: " + std::to_string(root_schema_version) + " (expected 1)").c_str());
        return result;
    }

    // Duplicate root field detection
    // (handled implicitly by the non-repeating nature of the root fields)

    // Fail-closed: require valid ir_version, seed_identity, entity_id, and entity
    if (!saw_ir_version) { fail("deserialize: missing required ir_version"); return result; }
    if (ir.ir_version != "gspl-ir/1.0") {
        fail(("deserialize: unsupported ir_version: " + ir.ir_version).c_str()); return result;
    }
    if (!saw_seed_identity || ir.seed_identity.empty()) {
        fail("deserialize: missing required seed_identity"); return result;
    }
    if (!saw_entity_id || ir.entity_id.empty()) {
        fail("deserialize: missing required entity_id"); return result;
    }
    if (!ir.entity) {
        fail("deserialize: missing required entity"); return result;
    }

    // Run structural validation
    auto validation = IrSerializer::validate(ir);
    if (!validation.ok()) {
        result.diagnostics = validation;
        return result;
    }

    result.value = std::move(ir);
    return result;
}

DiagnosticResult IrSerializer::validate(SpriteIr const& ir) {
    DiagnosticResult dr;
    if (ir.entity_id.empty())
        dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, "Entity ID must not be empty", {});
    if (ir.seed_identity.empty())
        dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, "Seed identity must not be empty", {});
    if (ir.ir_version != "gspl-ir/1.0")
        dr.add_error(DiagnosticCode::GSPL_IR_UNSUPPORTED_VERSION,
                     "Unsupported IR version: " + ir.ir_version, {});
    // Validate entity if present
    if (ir.entity) {
        if (ir.entity->entity_id.empty())
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                         "Entity IR must have non-empty entity_id", {});
        if (ir.entity->schema_version == 0)
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                         "Entity IR schema_version must be >= 1", {});
        if (ir.entity->kind != IrNodeKind::entity)
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                         "Entity root kind must be 'entity'", {});
        if (ir.entity->identity.empty())
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                         "Entity must have non-empty identity", {});
        // Validate entity children kinds
        for (auto const& child : ir.entity->children) {
            if (!is_allowed_as_entity_child(child->kind)) {
                dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                    "Child node kind " + std::to_string(static_cast<std::uint32_t>(child->kind)) + " not allowed as entity child", {});
            }
        }
        // Validate genes have nonempty type_id
        for (auto const& gene : ir.entity->genes) {
            if (gene.descriptor.type_id.empty())
                dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                             "Gene must have non-empty type_id", {});
        }
    }
    // Validate collection-specific node kinds
    for (auto const& rep : ir.representations) {
        if (!is_allowed_in_representations(rep->kind))
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                "Representation node kind " + std::to_string(static_cast<std::uint32_t>(rep->kind)) + " not allowed (expected representation_plan)", {});
    }
    for (auto const& plan : ir.runtime_plans) {
        if (!is_allowed_in_runtime_plans(plan->kind))
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                "Runtime plan node kind " + std::to_string(static_cast<std::uint32_t>(plan->kind)) + " not allowed (expected runtime_plan)", {});
    }
    for (auto const& plan : ir.package_plans) {
        if (!is_allowed_in_package_plans(plan->kind))
            dr.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                "Package plan node kind " + std::to_string(static_cast<std::uint32_t>(plan->kind)) + " not allowed (expected package_plan)", {});
    }
    return dr;
}

std::string IrSerializer::diff(SpriteIr const& before, SpriteIr const& after) {
    std::ostringstream ss;
    ss << "diff:\n";
    if (before.entity_id != after.entity_id)
        ss << "  entity_id: '" << before.entity_id << "' -> '" << after.entity_id << "'\n";
    if (before.seed_identity != after.seed_identity)
        ss << "  seed_identity: '" << before.seed_identity << "' -> '" << after.seed_identity << "'\n";
    if (before.ir_version != after.ir_version)
        ss << "  ir_version: '" << before.ir_version << "' -> '" << after.ir_version << "'\n";

    auto count_diff = [&](std::string_view label, std::size_t b, std::size_t a) {
        if (b != a) ss << "  " << label << "_count: " << b << " -> " << a << "\n";
    };
    count_diff("representation", before.representations.size(), after.representations.size());
    count_diff("runtime_plan", before.runtime_plans.size(), after.runtime_plans.size());
    count_diff("package_plan", before.package_plans.size(), after.package_plans.size());

    if (before.entity && after.entity) {
        if (before.entity->entity_id != after.entity->entity_id)
            ss << "  entity.entity_id: '" << before.entity->entity_id
               << "' -> '" << after.entity->entity_id << "'\n";
        count_diff("entity.gene", before.entity->genes.size(), after.entity->genes.size());
        count_diff("entity.child", before.entity->children.size(), after.entity->children.size());
    }
    return ss.str();
}

std::string IrSerializer::explain(SpriteIr const& ir, std::string node_id) {
    std::ostringstream ss;
    ss << "Sprite IR for entity: " << ir.entity_id
       << "\n  version: " << ir.ir_version
       << "\n  seed: " << ir.seed_identity;
    if (ir.entity) {
        ss << "\n  entity_id: " << ir.entity->entity_id
           << "\n  genes: " << ir.entity->genes.size()
           << "\n  children: " << ir.entity->children.size()
           << "\n  dependencies: " << ir.entity->dependency_ids.size();
    }
    ss << "\n  representations: " << ir.representations.size()
       << "\n  runtime_plans: " << ir.runtime_plans.size()
       << "\n  package_plans: " << ir.package_plans.size();
    if (!node_id.empty()) {
        ss << "\n  requested node: " << node_id;
    }
    return ss.str();
}

std::vector<std::string> IrSerializer::dependencies(SpriteIr const& ir, std::string node_id) {
    std::vector<std::string> deps;
    std::set<std::string> seen;

    auto collect_deps = [&](IrNode const& node) {
        for (auto const& d : node.dependency_ids) {
            if (seen.insert(d).second) deps.push_back(d);
        }
    };

    if (ir.entity) {
        if (node_id.empty() || node_id == ir.entity->entity_id ||
            node_id == ir.entity->identity) {
            collect_deps(*ir.entity);
            for (auto const& child : ir.entity->children) {
                collect_deps(*child);
            }
        } else {
            for (auto const& child : ir.entity->children) {
                if (child->identity == node_id) {
                    collect_deps(*child);
                    break;
                }
            }
        }
    }

    std::sort(deps.begin(), deps.end());
    return deps;
}

std::vector<std::string> IrSerializer::transitive_dependencies(SpriteIr const& ir, std::string node_id) {
    std::set<std::string> visited;
    std::set<std::string> closure;
    std::vector<std::string> stack;

    auto direct = dependencies(ir, node_id);
    for (auto const& d : direct) {
        if (closure.insert(d).second) stack.push_back(d);
    }

    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        if (!visited.insert(current).second) continue;

        auto child_deps = dependencies(ir, current);
        for (auto const& d : child_deps) {
            if (closure.insert(d).second) stack.push_back(d);
        }
    }

    std::vector<std::string> result(closure.begin(), closure.end());
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> IrSerializer::reverse_dependencies(SpriteIr const& ir, std::string node_id) {
    std::vector<std::string> rev;
    if (!ir.entity) return rev;

    auto check_node = [&](IrNode const& node) {
        for (auto const& d : node.dependency_ids) {
            if (d == node_id) return true;
        }
        return false;
    };

    if (check_node(*ir.entity) && ir.entity->identity != node_id) {
        rev.push_back(ir.entity->identity.empty() ? ir.entity->entity_id : ir.entity->identity);
    }
    for (auto const& child : ir.entity->children) {
        if (check_node(*child) && child->identity != node_id) {
            rev.push_back(child->identity);
        }
    }

    std::sort(rev.begin(), rev.end());
    return rev;
}

std::vector<std::string> IrSerializer::dependency_closure(SpriteIr const& ir, std::string root_id) {
    std::set<std::string> closure;
    std::set<std::string> visited;
    std::vector<std::string> stack;

    // Start from root or entity
    auto start_deps = dependencies(ir, root_id);
    for (auto const& d : start_deps) {
        if (closure.insert(d).second) stack.push_back(d);
    }

    while (!stack.empty()) {
        auto current = stack.back();
        stack.pop_back();
        if (!visited.insert(current).second) continue;

        auto child_deps = dependencies(ir, current);
        for (auto const& d : child_deps) {
            if (closure.insert(d).second) stack.push_back(d);
        }
    }

    // Include the root itself
    closure.insert(root_id);

    std::vector<std::string> result(closure.begin(), closure.end());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace gspl
