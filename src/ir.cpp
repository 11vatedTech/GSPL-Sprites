#include "gspl/ir.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <set>
#include <sstream>

namespace gspl {
namespace {

// ── minimal bounded JSON helpers ───────────────────────────

std::string json_escape(std::string_view sv) {
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
    ss << "  \"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
}

void json_append_int(std::ostringstream& ss, std::string_view key, auto value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << value;
}

void json_append_bool(std::ostringstream& ss, std::string_view key, bool value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << (value ? "true" : "false");
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
           << ",\"type\":\"" << json_escape(g.descriptor.type_id) << "\""
           << ",\"source\":\"" << json_escape(g.source_module) << "\""
           << ",\"values\":{";
        bool first_val = true;
        for (auto const& [k, v] : g.values) {
            if (!first_val) ss << ",";
            first_val = false;
            ss << "\"" << json_escape(k) << "\":\"" << json_escape(gene_value_to_string(v)) << "\"";
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
        ss << "\"" << json_escape(k) << "\":\"" << json_escape(v) << "\"";
    }
    ss << "}";

    // dependency_ids
    ss << ",\n  \"dependency_ids\": [";
    bool first_dep = true;
    for (auto const& d : entity.dependency_ids) {
        if (!first_dep) ss << ", ";
        first_dep = false;
        ss << "\"" << json_escape(d) << "\"";
    }
    ss << "]";

    // children
    ss << ",\n  \"children\": [";
    bool first_child = true;
    for (auto const& child : entity.children) {
        if (!first_child) ss << ",\n  ";
        first_child = false;
        ss << "{\"kind\":" << static_cast<std::uint32_t>(child->kind)
           << ",\"identity\":\"" << json_escape(child->identity) << "\"";
        if (!child->properties.empty()) {
            ss << ",\"properties\":{";
            bool fp = true;
            for (auto const& [k, v] : child->properties) {
                if (!fp) ss << ",";
                fp = false;
                ss << "\"" << json_escape(k) << "\":\"" << json_escape(v) << "\"";
            }
            ss << "}";
        }
        if (!child->dependency_ids.empty()) {
            ss << ",\"dependency_ids\":[";
            bool fd = true;
            for (auto const& d : child->dependency_ids) {
                if (!fd) ss << ",";
                fd = false;
                ss << "\"" << json_escape(d) << "\"";
            }
            ss << "]";
        }
        ss << "}";
    }
    ss << "]\n";
    ss << "}";
    return ss.str();
}

// ── minimal JSON reader ───────────────────────────────────

class JsonReader {
public:
    explicit JsonReader(std::string_view src) : src_(src), pos_(0) {}

    void skip_ws() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\n' ||
               src_[pos_] == '\r' || src_[pos_] == '\t')) ++pos_;
    }

    bool expect(char c) {
        skip_ws();
        if (pos_ >= src_.size() || src_[pos_] != c) return false;
        ++pos_;
        return true;
    }

    std::string read_string() {
        skip_ws();
        if (pos_ >= src_.size() || src_[pos_] != '"') return {};
        ++pos_;
        std::string out;
        while (pos_ < src_.size() && src_[pos_] != '"') {
            if (src_[pos_] == '\\' && pos_ + 1 < src_.size()) {
                ++pos_;
                switch (src_[pos_]) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (pos_ + 4 < src_.size()) {
                        auto hex = src_.substr(pos_ + 1, 4);
                        unsigned cp = 0;
                        for (auto hc : hex) {
                            cp <<= 4;
                            if (hc >= '0' && hc <= '9') cp |= (hc - '0');
                            else if (hc >= 'a' && hc <= 'f') cp |= (hc - 'a' + 10);
                            else if (hc >= 'A' && hc <= 'F') cp |= (hc - 'A' + 10);
                        }
                        if (cp < 0x80) out += static_cast<char>(cp);
                        else out += '?';
                        pos_ += 4;
                    }
                    continue; // skip outer ++pos_ 
                }
                default: out += src_[pos_]; break;
                }
            } else {
                out += src_[pos_];
            }
            ++pos_;
        }
        if (pos_ < src_.size()) ++pos_; // skip closing quote
        return out;
    }

    std::int64_t read_int(std::int64_t fallback = 0) {
        skip_ws();
        std::string num;
        if (pos_ < src_.size() && src_[pos_] == '-') { num += src_[pos_]; ++pos_; }
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            num += src_[pos_];
            ++pos_;
        }
        if (num.empty()) return fallback;
        std::int64_t v{};
        auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), v);
        if (ec != std::errc{}) return fallback;
        return v;
    }

    bool read_bool() {
        skip_ws();
        if (pos_ + 4 <= src_.size() && src_.substr(pos_, 4) == "true") {
            pos_ += 4;
            return true;
        }
        if (pos_ + 5 <= src_.size() && src_.substr(pos_, 5) == "false") {
            pos_ += 5;
            return false;
        }
        return false;
    }

    bool has_more() const { return pos_ < src_.size(); }

    void skip_value() {
        skip_ws();
        if (pos_ >= src_.size()) return;
        char c = src_[pos_];
        if (c == '"') {
            read_string();
        } else if (c == '{') {
            int depth = 1; ++pos_;
            while (pos_ < src_.size() && depth > 0) {
                if (src_[pos_] == '{') ++depth;
                else if (src_[pos_] == '}') --depth;
                else if (src_[pos_] == '"') read_string();
                else ++pos_;
            }
            if (pos_ < src_.size()) ++pos_; // skip past '}'
        } else if (c == '[') {
            int depth = 1; ++pos_;
            while (pos_ < src_.size() && depth > 0) {
                if (src_[pos_] == '[') ++depth;
                else if (src_[pos_] == ']') --depth;
                else if (src_[pos_] == '"') read_string();
                else ++pos_;
            }
            if (pos_ < src_.size()) ++pos_; // skip past ']'
        } else if (c == 't' || c == 'f') {
            read_bool();
        } else {
            // number or null — consume until delimiter
            while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
                   src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') ++pos_;
        }
    }

private:
    std::string_view src_;
    std::size_t pos_;
};

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

    // representations
    ss << ",\n  \"representation_count\": " << ir.representations.size();
    ss << ",\n  \"runtime_plan_count\": " << ir.runtime_plans.size();
    ss << ",\n  \"package_plan_count\": " << ir.package_plans.size();
    ss << "\n}";
    return ss.str();
}

SpriteIr IrSerializer::deserialize(std::string_view json) {
    SpriteIr ir;
    if (json.empty()) {
        ir.entity_id = "deserialized";
        ir.seed_identity = "seed";
        ir.entity = std::make_unique<EntityIr>();
        ir.entity->entity_id = "deserialized_entity";
        return ir;
    }

    JsonReader r(json);
    if (!r.expect('{')) return ir;

    // Parse top-level key-value pairs
    while (r.has_more()) {
        r.skip_ws();
        auto key = r.read_string();
        if (key.empty()) break;
        if (!r.expect(':')) break;

        if (key == "ir_version") {
            ir.ir_version = r.read_string();
        } else if (key == "entity_id") {
            ir.entity_id = r.read_string();
        } else if (key == "seed_identity") {
            ir.seed_identity = r.read_string();
        } else if (key == "schema_version") {
            r.read_int();
        } else if (key == "entity") {
            r.skip_ws();
            if (r.expect('{')) {
                ir.entity = std::make_unique<EntityIr>();
                ir.entity->kind = IrNodeKind::entity;
                while (r.has_more()) {
                    r.skip_ws();
                    auto ek = r.read_string();
                    if (ek.empty()) break;
                    if (!r.expect(':')) break;
                    if (ek == "identity") {
                        ir.entity->identity = r.read_string();
                    } else if (ek == "entity_id") {
                        ir.entity->entity_id = r.read_string();
                    } else if (ek == "schema_version") {
                        ir.entity->schema_version = static_cast<std::uint32_t>(r.read_int());
                    } else if (ek == "dependency_ids") {
                        r.skip_ws();
                        if (r.expect('[')) {
                            for (;;) {
                                r.skip_ws();
                                auto dep = r.read_string();
                                if (dep.empty()) break;
                                ir.entity->dependency_ids.push_back(dep);
                                r.skip_ws();
                                if (!r.expect(',')) break;
                            }
                        }
                    } else {
                        r.skip_value();
                    }
                    r.skip_ws();
                    if (!r.expect(',')) {
                        r.skip_ws();
                        break; // '}' or end-of-object
                    }
                }
            }
        } else {
            r.skip_value();
        }
        r.skip_ws();
        if (!r.expect(',')) {
            r.skip_ws();
            break; // '}' or end-of-object
        }
    }

    if (ir.entity_id.empty()) {
        ir.entity_id = "deserialized";
        ir.seed_identity = "seed";
    }
    if (!ir.entity) {
        ir.entity = std::make_unique<EntityIr>();
        ir.entity->entity_id = ir.entity_id;
    }
    return ir;
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
            // search for specific node
            for (auto const& child : ir.entity->children) {
                if (child->identity == node_id) {
                    collect_deps(*child);
                    break;
                }
            }
        }
    }

    // Sort for determinism
    std::sort(deps.begin(), deps.end());
    return deps;
}

} // namespace gspl
