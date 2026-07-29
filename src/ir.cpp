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

    auto require = [&](char expected, const char* ctx) {
        if (!r.require(expected, ctx)) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                         r.error_message(), {});
        }
    };

    auto expect_sep = [&](const char* ctx) -> bool {
        // Returns true if comma consumed (more items follow)
        if (r.consume_if(',')) return true;
        // Check for trailing comma
        r.skip_ws();
        if (r.consume_if(',')) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR, ctx, {});
            return true;
        }
        return false;
    };

    // Parse a generic IrNode object (used for representations, runtime_plans, package_plans)
    auto parse_ir_node = [&](IrNode& node, const char* label) -> bool {
        if (!r.require('{', label)) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_IR_VALIDATION_ERROR,
                                         r.error_message(), {});
            return false;
        }
        while (r.has_more() && !r.has_error()) {
            auto key_opt = read_str(label);
            if (!key_opt) return false;
            auto& key = *key_opt;
            require(':', label);
            if (r.has_error()) return false;

            if (key == "kind") {
                auto kind_opt = read_i64(label);
                if (!kind_opt) return false;
                if (!is_valid_ir_node_kind(*kind_opt)) {
                    fail(("deserialize: unknown " + std::string(label) + " node kind: " + std::to_string(*kind_opt)).c_str());
                    return false;
                }
                node.kind = static_cast<IrNodeKind>(*kind_opt);
            } else if (key == "identity") {
                auto id_opt = read_str(label);
                if (!id_opt) return false;
                node.identity = std::move(*id_opt);
            } else if (key == "schema_version") {
                auto sv_opt = read_i64(label);
                if (!sv_opt) return false;
                node.schema_version = static_cast<std::uint32_t>(*sv_opt);
            } else if (key == "properties") {
                require('{', label);
                if (r.has_error()) return false;
                while (r.has_more() && !r.has_error()) {
                    auto pk_opt = read_str(label);
                    if (!pk_opt) return false;
                    require(':', label);
                    if (r.has_error()) return false;
                    auto pv_opt = read_str(label);
                    if (!pv_opt) return false;
                    node.properties[*pk_opt] = std::move(*pv_opt);
                    if (!expect_sep("deserialize: trailing comma in node properties")) break;
                }
                require('}', label);
            } else if (key == "dependency_ids") {
                require('[', label);
                if (r.has_error()) return false;
                while (r.has_more() && !r.has_error()) {
                    auto dep_opt = read_str(label);
                    if (!dep_opt) return false;
                    node.dependency_ids.push_back(std::move(*dep_opt));
                    if (!expect_sep("deserialize: trailing comma in node dependency_ids")) break;
                }
                require(']', label);
            } else {
                r.skip_value();
            }
            if (!expect_sep("deserialize: trailing comma in node")) break;
        }
        require('}', label);
        return !r.has_error();
    };

    // ── Root object ──────────────────────────────────────
    require('{', "deserialize: root object");
    if (r.has_error()) return result;

    bool parsed_any = false;

    while (r.has_more() && !r.has_error()) {
        auto key_opt = read_str("deserialize: root key");
        if (!key_opt) return result;
        auto& key = *key_opt;
        require(':', "deserialize: ':' after root key");
        if (r.has_error()) return result;

        parsed_any = true;

        if (key == "ir_version") {
            auto v_opt = read_str("ir_version");
            if (!v_opt) return result;
            ir.ir_version = std::move(*v_opt);
        } else if (key == "entity_id") {
            auto v_opt = read_str("entity_id");
            if (!v_opt) return result;
            ir.entity_id = std::move(*v_opt);
        } else if (key == "seed_identity") {
            auto v_opt = read_str("seed_identity");
            if (!v_opt) return result;
            ir.seed_identity = std::move(*v_opt);
        } else if (key == "schema_version") {
            auto v_opt = read_i64("schema_version");
            if (!v_opt) return result;
            // consumed, validated below
        } else if (key == "entity") {
            require('{', "deserialize: entity object");
            if (r.has_error()) return result;
            ir.entity = std::make_unique<EntityIr>();
            ir.entity->kind = IrNodeKind::entity;
            while (r.has_more() && !r.has_error()) {
                auto ek_opt = read_str("entity key");
                if (!ek_opt) return result;
                auto& ek = *ek_opt;
                require(':', ("deserialize: ':' after entity key '" + ek + "'").c_str());
                if (r.has_error()) return result;

                if (ek == "identity") {
                    auto v_opt = read_str("entity.identity");
                    if (!v_opt) return result;
                    ir.entity->identity = std::move(*v_opt);
                } else if (ek == "entity_id") {
                    auto v_opt = read_str("entity.entity_id");
                    if (!v_opt) return result;
                    ir.entity->entity_id = std::move(*v_opt);
                } else if (ek == "schema_version") {
                    auto v_opt = read_i64("entity.schema_version");
                    if (!v_opt) return result;
                    ir.entity->schema_version = static_cast<std::uint32_t>(*v_opt);
                } else if (ek == "dependency_ids") {
                    require('[', "entity.dependency_ids");
                    if (r.has_error()) return result;
                    while (r.has_more() && !r.has_error()) {
                        auto dep_opt = read_str("entity.dependency_ids[]");
                        if (!dep_opt) return result;
                        ir.entity->dependency_ids.push_back(std::move(*dep_opt));
                        if (!expect_sep("deserialize: trailing comma in entity.dependency_ids")) break;
                    }
                    require(']', "entity.dependency_ids");
                } else if (ek == "properties") {
                    require('{', "entity.properties");
                    if (r.has_error()) return result;
                    while (r.has_more() && !r.has_error()) {
                        auto pk_opt = read_str("entity.properties key");
                        if (!pk_opt) return result;
                        require(':', ("entity.properties['" + *pk_opt + "']").c_str());
                        if (r.has_error()) return result;
                        auto pv_opt = read_str("entity.properties value");
                        if (!pv_opt) return result;
                        ir.entity->properties[*pk_opt] = std::move(*pv_opt);
                        if (!expect_sep("deserialize: trailing comma in entity.properties")) break;
                    }
                    require('}', "entity.properties");
                } else if (ek == "children") {
                    require('[', "entity.children");
                    if (r.has_error()) return result;
                    while (r.has_more() && !r.has_error()) {
                        auto child = std::make_unique<IrNode>();
                        if (!parse_ir_node(*child, "entity.children[]")) return result;
                        ir.entity->children.push_back(std::move(child));
                        if (!expect_sep("deserialize: trailing comma in entity.children")) break;
                    }
                    require(']', "entity.children");
                } else if (ek == "genes") {
                    require('[', "entity.genes");
                    if (r.has_error()) return result;
                    GeneRegistry registry;
                    while (r.has_more() && !r.has_error()) {
                        require('{', "entity.genes[]");
                        if (r.has_error()) return result;
                        GeneInstance gi;
                        bool saw_kind = false, saw_schema = false, saw_type = false;
                        while (r.has_more() && !r.has_error()) {
                            auto gk_opt = read_str("gene key");
                            if (!gk_opt) return result;
                            auto& gk = *gk_opt;
                            require(':', ("gene.'" + gk + "'").c_str());
                            if (r.has_error()) return result;

                            if (gk == "kind") {
                                auto kv_opt = read_i64("gene.kind");
                                if (!kv_opt) return result;
                                auto kind_val = static_cast<GeneKind>(*kv_opt);
                                auto const* desc = registry.lookup(kind_val);
                                if (desc) gi.descriptor = *desc;
                                else gi.descriptor.kind = kind_val;
                                saw_kind = true;
                            } else if (gk == "schema") {
                                auto sv_opt = read_i64("gene.schema");
                                if (!sv_opt) return result;
                                gi.descriptor.schema_version = static_cast<std::uint32_t>(*sv_opt);
                                saw_schema = true;
                            } else if (gk == "type") {
                                auto t_opt = read_str("gene.type");
                                if (!t_opt) return result;
                                gi.descriptor.type_id = std::move(*t_opt);
                                saw_type = true;
                            } else if (gk == "source") {
                                auto s_opt = read_str("gene.source");
                                if (!s_opt) return result;
                                gi.source_module = std::move(*s_opt);
                            } else if (gk == "values") {
                                require('{', "gene.values");
                                if (r.has_error()) return result;
                                while (r.has_more() && !r.has_error()) {
                                    auto vk_opt = read_str("gene.value key");
                                    if (!vk_opt) return result;
                                    require(':', ("gene.values['" + *vk_opt + "']").c_str());
                                    if (r.has_error()) return result;
                                    if (r.consume_if('{')) {
                                        bool saw_tag = false, saw_value = false;
                                        std::uint32_t tag_val = 0;
                                        std::string raw_val;
                                        bool in_tagged_value = true;
                                        while (r.has_more() && !r.has_error() && in_tagged_value) {
                                            auto tk_opt = read_str("gene.value tagged key");
                                            if (!tk_opt) return result;
                                            auto& tk = *tk_opt;
                                            require(':', ("gene.value.'" + tk + "'").c_str());
                                            if (r.has_error()) return result;
                                            if (tk == "t") {
                                                auto tv_opt = read_i64("gene.value.t");
                                                if (!tv_opt) return result;
                                                tag_val = static_cast<std::uint32_t>(*tv_opt);
                                                saw_tag = true;
                                            } else if (tk == "v") {
                                                raw_val = r.read_typed_value();
                                                saw_value = true;
                                            } else {
                                                r.skip_value();
                                            }
                                            in_tagged_value = expect_sep("deserialize: trailing comma in gene tagged value");
                                        }
                                        require('}', "gene.value tagged object");
                                        if (r.has_error()) return result;
                                        if (!saw_tag) { fail("deserialize: gene value missing required 't' field"); return result; }
                                        if (!saw_value) { fail("deserialize: gene value missing required 'v' field"); return result; }
                                        if (!raw_val.empty()) {
                                            auto tag = static_cast<GeneValueTag>(tag_val);
                                            auto gv_result = json_to_gene_value_result(raw_val, tag);
                                            if (!gv_result.ok()) {
                                                result.diagnostics = gv_result.diagnostics;
                                                return result;
                                            }
                                            gi.values[*vk_opt] = *gv_result.value;
                                        }
                                    }
                                    // Current schema: reject bare string values
                                    if (!expect_sep("deserialize: trailing comma in gene values")) break;
                                }
                                require('}', "gene.values");
                            } else {
                                r.skip_value();
                            }
                            if (!expect_sep("deserialize: trailing comma in gene")) break;
                        }
                        require('}', "gene object");
                        if (r.has_error()) return result;
                        if (saw_kind && saw_schema && saw_type && !gi.descriptor.type_id.empty()) {
                            ir.entity->genes.push_back(std::move(gi));
                        }
                        if (!expect_sep("deserialize: trailing comma in genes array")) break;
                    }
                    require(']', "entity.genes");
                } else {
                    r.skip_value();
                }
                if (!expect_sep("deserialize: trailing comma in entity")) break;
            }
            require('}', "entity object");
            if (r.has_error()) return result;
        } else if (key == "representations") {
            require('[', "representations");
            if (r.has_error()) return result;
            while (r.has_more() && !r.has_error()) {
                auto rep = std::make_unique<IrNode>();
                if (!parse_ir_node(*rep, "representations[]")) return result;
                ir.representations.push_back(std::move(rep));
                if (!expect_sep("deserialize: trailing comma in representations")) break;
            }
            require(']', "representations");
        } else if (key == "runtime_plans") {
            require('[', "runtime_plans");
            if (r.has_error()) return result;
            while (r.has_more() && !r.has_error()) {
                auto plan = std::make_unique<IrNode>();
                if (!parse_ir_node(*plan, "runtime_plans[]")) return result;
                ir.runtime_plans.push_back(std::move(plan));
                if (!expect_sep("deserialize: trailing comma in runtime_plans")) break;
            }
            require(']', "runtime_plans");
        } else if (key == "package_plans") {
            require('[', "package_plans");
            if (r.has_error()) return result;
            while (r.has_more() && !r.has_error()) {
                auto plan = std::make_unique<IrNode>();
                if (!parse_ir_node(*plan, "package_plans[]")) return result;
                ir.package_plans.push_back(std::move(plan));
                if (!expect_sep("deserialize: trailing comma in package_plans")) break;
            }
            require(']', "package_plans");
        } else {
            r.skip_value();
        }
        if (!expect_sep("deserialize: trailing comma in root")) break;
    }

    // Document closure: consume closing '}' and reject trailing data
    if (!r.has_error()) {
        r.skip_ws();
        if (!r.consume_if('}')) {
            fail("deserialize: missing closing '}' of root object"); return result;
        }
        r.skip_ws();
        if (r.position() < r.source().size()) {
            fail("deserialize: trailing data after root '}'"); return result;
        }
    }

    if (r.has_error()) {
        fail(r.error_message().c_str()); return result;
    }
    if (!parsed_any) {
        fail("deserialize: no fields parsed"); return result;
    }

    // Fail-closed: require valid ir_version, seed_identity, entity_id, and entity
    if (ir.ir_version != "gspl-ir/1.0") {
        fail(("deserialize: unsupported ir_version: " + ir.ir_version).c_str()); return result;
    }
    if (ir.seed_identity.empty()) {
        fail("deserialize: missing required seed_identity"); return result;
    }
    if (ir.entity_id.empty()) {
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
