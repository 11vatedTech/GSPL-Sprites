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

// ── Node kind validation ───────────────────────────────────

namespace {
bool is_valid_ir_node_kind(std::int64_t raw) noexcept {
    return raw >= 0 &&
           raw <= static_cast<std::int64_t>(IrNodeKind::target_requirement);
}
} // namespace

SpriteIrDeserializeResult IrSerializer::deserialize(std::string_view json) {
    SpriteIrDeserializeResult result;
    SpriteIr ir;

    if (json.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     "deserialize: empty input", {});
        return result;
    }

    BoundedJsonReader r(json);
    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     r.error_message(), {});
        return result;
    }
    r.require('{', "deserialize: root object");
    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     r.error_message(), {});
        return result;
    }

    bool parsed_any = false;

    // Parse top-level key-value pairs
    while (r.has_more()) {
        auto key = r.read_string();
        if (key.empty() || r.has_error()) break;
        if (!r.expect(':')) break;

        parsed_any = true;

        if (key == "ir_version") {
            ir.ir_version = r.read_string();
        } else if (key == "entity_id") {
            ir.entity_id = r.read_string();
        } else if (key == "seed_identity") {
            ir.seed_identity = r.read_string();
        } else if (key == "schema_version") {
            r.read_int64();
        } else if (key == "entity") {
            if (r.require('{', "deserialize: entity object")) {
                ir.entity = std::make_unique<EntityIr>();
                ir.entity->kind = IrNodeKind::entity;
                while (r.has_more()) {
                    auto ek = r.read_string();
                    if (ek.empty() || r.has_error()) break;
                    if (!r.require(':', ("deserialize: ':' after entity key '" + ek + "'").c_str())) break;
                    if (ek == "identity") {
                        ir.entity->identity = r.read_string();
                    } else if (ek == "entity_id") {
                        ir.entity->entity_id = r.read_string();
                    } else if (ek == "schema_version") {
                        ir.entity->schema_version = static_cast<std::uint32_t>(r.read_int64());
                    } else if (ek == "dependency_ids") {
                        if (r.expect('[')) {
                            for (;;) {
                                auto dep = r.read_string();
                                if (dep.empty()) break;
                                ir.entity->dependency_ids.push_back(dep);
                                if (!r.expect(',')) break;
                            }
                            r.expect(']');
                        }
                    } else if (ek == "properties") {
                        if (r.expect('{')) {
                            while (r.has_more()) {
                                auto pk = r.read_string();
                                if (pk.empty()) break;
                                if (!r.expect(':')) break;
                                ir.entity->properties[pk] = r.read_string();
                                if (!r.expect(',')) break;
                            }
                            r.expect('}');
                        }
                    } else if (ek == "children") {
                        if (r.expect('[')) {
                            while (r.has_more()) {
                                if (!r.expect('{')) break;
                                auto child = std::make_unique<IrNode>();
                                while (r.has_more()) {
                                    auto ck = r.read_string();
                                    if (ck.empty()) break;
                                    if (!r.expect(':')) break;
                                    if (ck == "kind") {
                                        auto kind_raw = r.read_int64();
                                        if (!is_valid_ir_node_kind(kind_raw)) {
                                            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                                "deserialize: unknown child node kind: " + std::to_string(kind_raw), {});
                                            return result;
                                        }
                                        child->kind = static_cast<IrNodeKind>(kind_raw);
                                    } else if (ck == "identity") {
                                        child->identity = r.read_string();
                                    } else if (ck == "schema_version") {
                                        child->schema_version = static_cast<std::uint32_t>(r.read_int64());
                                    } else if (ck == "properties") {
                                        if (r.expect('{')) {
                                            while (r.has_more()) {
                                                auto pk = r.read_string();
                                                if (pk.empty()) break;
                                                if (!r.expect(':')) break;
                                                child->properties[pk] = r.read_string();
                                                if (!r.expect(',')) break;
                                            }
                                            r.expect('}');
                                        }
                                    } else if (ck == "dependency_ids") {
                                        if (r.expect('[')) {
                                            while (r.has_more()) {
                                                auto dep = r.read_string();
                                                if (dep.empty()) break;
                                                child->dependency_ids.push_back(dep);
                                                if (!r.expect(',')) break;
                                            }
                                            r.expect(']');
                                        }
                                    } else {
                                        r.skip_value();
                                    }
                                    if (!r.expect(',')) break;
                                }
                                r.expect('}');
                                ir.entity->children.push_back(std::move(child));
                                if (!r.expect(',')) break;
                            }
                            r.expect(']');
                        }
                    } else if (ek == "genes") {
                        if (r.expect('[')) {
                            GeneRegistry registry;
                            while (r.has_more()) {
                                if (!r.expect('{')) break;
                                GeneInstance gi;
                                while (r.has_more()) {
                                    auto gk = r.read_string();
                                    if (gk.empty()) break;
                                    if (!r.expect(':')) break;
                                    if (gk == "kind") {
                                        auto kind_val = static_cast<GeneKind>(r.read_int64());
                                        auto const* desc = registry.lookup(kind_val);
                                        if (desc) gi.descriptor = *desc;
                                        else gi.descriptor.kind = kind_val;
                                    } else if (gk == "schema") {
                                        gi.descriptor.schema_version = static_cast<std::uint32_t>(r.read_int64());
                                    } else if (gk == "type") {
                                        gi.descriptor.type_id = r.read_string();
                                    } else if (gk == "source") {
                                        gi.source_module = r.read_string();
                                    } else if (gk == "values") {
                                        if (r.expect('{')) {
                                            while (r.has_more()) {
                                                auto vk = r.read_string();
                                                if (vk.empty()) break;
                                                if (!r.expect(':')) break;
                                                if (r.expect('{')) {
                                                    std::uint32_t tag_val = 0;
                                                    std::string raw_val;
                                                    while (r.has_more()) {
                                                        auto tk = r.read_string();
                                                        if (tk.empty()) break;
                                                        if (!r.expect(':')) break;
                                                        if (tk == "t") {
                                                            tag_val = static_cast<std::uint32_t>(r.read_int64());
                                                        } else if (tk == "v") {
                                                            raw_val = r.read_typed_value();
                                                        } else {
                                                            r.skip_value();
                                                        }
                                                        if (!r.expect(',')) break;
                                                    }
                                                    r.expect('}');
                                                    if (!raw_val.empty()) {
                                                        auto tag = static_cast<GeneValueTag>(tag_val);
                                                        auto gv_result = json_to_gene_value_result(raw_val, tag);
                                                        if (!gv_result.ok()) {
                                                            result.diagnostics = gv_result.diagnostics;
                                                            return result;
                                                        }
                                                        gi.values[vk] = *gv_result.value;
                                                    }
                                                }
                                                // Current schema: reject bare string values (no legacy fallback)
                                                if (!r.expect(',')) break;
                                            }
                                            r.expect('}');
                                        }
                                    } else {
                                        r.skip_value();
                                    }
                                    if (!r.expect(',')) break;
                                }
                                r.expect('}');
                                if (!gi.descriptor.type_id.empty()) {
                                    ir.entity->genes.push_back(std::move(gi));
                                }
                                if (!r.expect(',')) break;
                            }
                            r.expect(']');
                        }
                    } else {
                        r.skip_value();
                    }
                    if (!r.expect(',')) break;
                }
                r.expect('}');
            }
        } else if (key == "representations") {
            if (r.expect('[')) {
                while (r.has_more()) {
                    if (!r.expect('{')) break;
                    auto rep = std::make_unique<IrNode>();
                    while (r.has_more()) {
                        auto rk = r.read_string();
                        if (rk.empty()) break;
                        if (!r.expect(':')) break;
                        if (rk == "kind") {
                            auto kind_raw = r.read_int64();
                            if (!is_valid_ir_node_kind(kind_raw)) {
                                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                    "deserialize: unknown representation node kind: " + std::to_string(kind_raw), {});
                                return result;
                            }
                            rep->kind = static_cast<IrNodeKind>(kind_raw);
                        } else if (rk == "identity") {
                            rep->identity = r.read_string();
                        } else if (rk == "schema_version") {
                            rep->schema_version = static_cast<std::uint32_t>(r.read_int64());
                        } else if (rk == "properties") {
                            if (r.expect('{')) {
                                while (r.has_more()) {
                                    auto pk = r.read_string();
                                    if (pk.empty()) break;
                                    if (!r.expect(':')) break;
                                    rep->properties[pk] = r.read_string();
                                    if (!r.expect(',')) break;
                                }
                                r.expect('}');
                            }
                        } else if (rk == "dependency_ids") {
                            if (r.expect('[')) {
                                while (r.has_more()) {
                                    auto dep = r.read_string();
                                    if (dep.empty()) break;
                                    rep->dependency_ids.push_back(dep);
                                    if (!r.expect(',')) break;
                                }
                                r.expect(']');
                            }
                        } else {
                            r.skip_value();
                        }
                        if (!r.expect(',')) break;
                    }
                    r.expect('}');
                    ir.representations.push_back(std::move(rep));
                    if (!r.expect(',')) break;
                }
                r.expect(']');
            }
        } else if (key == "runtime_plans") {
            if (r.expect('[')) {
                while (r.has_more()) {
                    if (!r.expect('{')) break;
                    auto plan = std::make_unique<IrNode>();
                    while (r.has_more()) {
                        auto pk = r.read_string();
                        if (pk.empty()) break;
                        if (!r.expect(':')) break;
                        if (pk == "kind") {
                            auto kind_raw = r.read_int64();
                            if (!is_valid_ir_node_kind(kind_raw)) {
                                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                    "deserialize: unknown runtime_plan node kind: " + std::to_string(kind_raw), {});
                                return result;
                            }
                            plan->kind = static_cast<IrNodeKind>(kind_raw);
                        } else if (pk == "identity") plan->identity = r.read_string();
                        else if (pk == "schema_version") plan->schema_version = static_cast<std::uint32_t>(r.read_int64());
                        else if (pk == "properties" && r.expect('{')) {
                            while (r.has_more()) {
                                auto ppk = r.read_string();
                                if (ppk.empty()) break;
                                if (!r.expect(':')) break;
                                plan->properties[ppk] = r.read_string();
                                if (!r.expect(',')) break;
                            }
                            r.expect('}');
                        } else if (pk == "dependency_ids" && r.expect('[')) {
                            while (r.has_more()) {
                                auto dep = r.read_string();
                                if (dep.empty()) break;
                                plan->dependency_ids.push_back(dep);
                                if (!r.expect(',')) break;
                            }
                            r.expect(']');
                        } else r.skip_value();
                        if (!r.expect(',')) break;
                    }
                    r.expect('}');
                    ir.runtime_plans.push_back(std::move(plan));
                    if (!r.expect(',')) break;
                }
                r.expect(']');
            }
        } else if (key == "package_plans") {
            if (r.expect('[')) {
                while (r.has_more()) {
                    if (!r.expect('{')) break;
                    auto plan = std::make_unique<IrNode>();
                    while (r.has_more()) {
                        auto pk = r.read_string();
                        if (pk.empty()) break;
                        if (!r.expect(':')) break;
                        if (pk == "kind") {
                            auto kind_raw = r.read_int64();
                            if (!is_valid_ir_node_kind(kind_raw)) {
                                result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                    "deserialize: unknown package_plan node kind: " + std::to_string(kind_raw), {});
                                return result;
                            }
                            plan->kind = static_cast<IrNodeKind>(kind_raw);
                        } else if (pk == "identity") plan->identity = r.read_string();
                        else if (pk == "schema_version") plan->schema_version = static_cast<std::uint32_t>(r.read_int64());
                        else if (pk == "properties" && r.expect('{')) {
                            while (r.has_more()) {
                                auto ppk = r.read_string();
                                if (ppk.empty()) break;
                                if (!r.expect(':')) break;
                                plan->properties[ppk] = r.read_string();
                                if (!r.expect(',')) break;
                            }
                            r.expect('}');
                        } else if (pk == "dependency_ids" && r.expect('[')) {
                            while (r.has_more()) {
                                auto dep = r.read_string();
                                if (dep.empty()) break;
                                plan->dependency_ids.push_back(dep);
                                if (!r.expect(',')) break;
                            }
                            r.expect(']');
                        } else r.skip_value();
                        if (!r.expect(',')) break;
                    }
                    r.expect('}');
                    ir.package_plans.push_back(std::move(plan));
                    if (!r.expect(',')) break;
                }
                r.expect(']');
            }
        } else {
            r.skip_value();
        }
        if (!r.expect(',')) break;
    }

    // Document closure: consume closing '}' and reject trailing data
    if (!r.has_error()) {
        r.skip_ws();
        if (!r.consume_if('}')) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                         "deserialize: missing closing '}' of root object", {});
            return result;
        }
        r.skip_ws();
        if (r.position() < r.source().size()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                         "deserialize: trailing data after root '}'", {});
            return result;
        }
    }

    if (r.has_error()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     r.error_message(), {});
        return result;
    }
    if (!parsed_any) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     "deserialize: no fields parsed", {});
        return result;
    }

    // Fail-closed: require valid ir_version, seed_identity, entity_id, and entity
    if (ir.ir_version != "gspl-ir/1.0") {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_UNSUPPORTED_VERSION,
                                     "deserialize: unsupported ir_version: " + ir.ir_version, {});
        return result;
    }
    if (ir.seed_identity.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     "deserialize: missing required seed_identity", {});
        return result;
    }
    if (ir.entity_id.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     "deserialize: missing required entity_id", {});
        return result;
    }
    if (!ir.entity) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                     "deserialize: missing required entity", {});
        return result;
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
