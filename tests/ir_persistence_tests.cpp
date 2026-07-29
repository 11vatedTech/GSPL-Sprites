#include "gspl/ir.hpp"
#include "gspl/json.hpp"
#include <iostream>
#include <string>

namespace {
int failures = 0;
void c(bool v, const char* m) { if (!v) { std::cerr << "FAILED: " << m << "\n"; ++failures; } }

std::string min_json() {
    return R"({"ir_version":"gspl-ir/1.0","entity_id":"test.entity","seed_identity":"sha256:deadbeef","schema_version":1,"entity":{"kind":0,"identity":"test-identity","entity_id":"test.entity","schema_version":1,"dependency_ids":[],"properties":{},"children":[],"genes":[]},"representations":[],"runtime_plans":[],"package_plans":[]})";
}

void ft(const char* l, const std::string& j) {
    auto r = gspl::IrSerializer::deserialize(j);
    c(!r.ok(), l);
    c(!r.value.has_value(), (std::string(l) + " - no value").c_str());
}
}

int main() {
    // Positive: round-trip + byte-stable
    {
        auto r = gspl::IrSerializer::deserialize(min_json());
        c(r.ok(), "valid IR deserialized");
        if (r.ok()) {
            c(r.value->entity_id == "test.entity", "entity_id");
            c(r.value->entity != nullptr, "entity present");
            if (r.value->entity) {
                c(r.value->entity->kind == gspl::IrNodeKind::entity, "entity kind");
                c(r.value->entity->identity == "test-identity", "entity identity");
            }
            std::string s1 = gspl::IrSerializer::serialize(*r.value);
            auto r2 = gspl::IrSerializer::deserialize(s1);
            c(r2.ok(), "re-serialized ok");
            if (r2.ok()) {
                std::string s2 = gspl::IrSerializer::serialize(*r2.value);
                c(s1 == s2, "byte-stable serialization");
            }
        }
    }

    // Negative: empty / truncated / whitespace
    ft("empty input", "");
    ft("whitespace only", "  \n\t ");
    ft("truncated root", R"({"ir_version": "gspl-ir/1.0")");

    // Negative: missing required root fields
    ft("missing ir_version", R"({"entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("missing entity_id", R"({"ir_version":"gspl-ir/1.0","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("missing seed_identity", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("missing entity", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1})");

    // Negative: unsupported versions
    ft("unsupported ir_version", R"({"ir_version":"gspl-ir/2.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("unsupported schema_version", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":2,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");

    // Negative: entity kind
    ft("wrong entity kind", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":1,"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("missing entity kind", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"identity":"i","entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("missing entity identity", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"entity_id":"x","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");
    ft("missing entity entity_id", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","schema_version":1,"genes":[],"properties":{},"dependency_ids":[],"children":[]}})");

    // Negative: trailing root data
    ft("trailing root data", min_json() + "garbage");

    // Negative: trailing comma
    ft("trailing comma", R"({"ir_version":"gspl-ir/1.0",})");

    // Negative: missing colon
    ft("missing colon", R"({"ir_version" "gspl-ir/1.0"})");

    // Negative: invalid GeneKind
    ft("GeneKind=999", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":999,"schema":1,"type":"x","source":"x","values":{}}]}})");

    // Negative: missing gene fields
    ft("gene missing schema", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"type":"x","source":"x","values":{}}]}})");
    ft("gene missing type", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"source":"x","values":{}}]}})");
    ft("gene missing source", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"x","values":{}}]}})");
    ft("gene missing values", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"x","source":"x"}]}})");
    ft("gene empty type_id", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"","source":"x","values":{}}]}})");
    ft("duplicate gene kind", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"kind":0,"schema":1,"type":"x","source":"x","values":{}}]}})");

    // Negative: invalid GeneValueTag
    ft("GeneValueTag=99", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"gspl.gene.identity","source":"x","values":{"x":{"t":99,"v":"\"hello\""}}}]}})");

    // Negative: non-object gene value
    ft("bare-string gene value", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"gspl.gene.identity","source":"x","values":{"x":"bare-string"}}]}})");

    // Negative: typed wrapper missing t/v / unknown field
    ft("missing t in wrapper", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"gspl.gene.identity","source":"x","values":{"x":{"v":"\"hello\""}}}]}})");
    ft("missing v in wrapper", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"gspl.gene.identity","source":"x","values":{"x":{"t":0}}}]}})");
    ft("unknown wrapper field", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"gspl.gene.identity","source":"x","values":{"x":{"t":0,"v":"\"hello\"","extra":1}}}]}})");

    // Negative: duplicate gene value name
    ft("duplicate value name", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[],"genes":[{"kind":0,"schema":1,"type":"gspl.gene.identity","source":"x","values":{"x":{"t":0,"v":"\"a\""},"x":{"t":0,"v":"\"b\""}}}]}})");

    // Negative: wrong-collection node kinds
    ft("representation in runtime_plans", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[]},"runtime_plans":[{"kind":13,"identity":"rep","schema_version":1}]})");
    ft("entity in representations", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[]},"representations":[{"kind":0,"identity":"ent","schema_version":1}]})");
    ft("disallowed child kind", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[{"kind":0,"identity":"child","schema_version":1}],"genes":[]}})");

    // Negative: generic node missing required fields
    ft("node missing kind", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[]},"representations":[{"identity":"rep","schema_version":1}]})");
    ft("node missing identity", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[]},"representations":[{"kind":13,"schema_version":1}]})");
    ft("node zero schema", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[]},"representations":[{"kind":13,"identity":"rep","schema_version":0}]})");

    // Negative: invalid IrNodeKind
    ft("IrNodeKind=999", R"({"ir_version":"gspl-ir/1.0","entity_id":"x","seed_identity":"x","schema_version":1,"entity":{"kind":0,"identity":"i","entity_id":"x","schema_version":1,"properties":{},"dependency_ids":[],"children":[]},"representations":[{"kind":999,"identity":"rep","schema_version":1}]})");

    // Positive: configurable limits
    {
        gspl::BoundedJsonConfig cfg;
        cfg.max_input_bytes = 50;
        auto r = gspl::IrSerializer::deserialize(min_json(), cfg);
        c(!r.ok(), "max_input_bytes limit enforced");
    }
    {
        gspl::BoundedJsonConfig cfg;
        cfg.max_input_bytes = 64 * 1024;
        auto r = gspl::IrSerializer::deserialize(min_json(), cfg);
        c(r.ok(), "within limit succeeds");
    }

    if (failures == 0) {
        std::cout << "ALL IR PERSISTENCE TESTS PASSED\n";
        return 0;
    }
    std::cerr << failures << " IR PERSISTENCE TEST(S) FAILED\n";
    return 1;
}
