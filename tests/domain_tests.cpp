#include "gspl_sprites/domain.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
  try {
    SpriteSeed seed{"gspl.sprite-seed/0.1", "original.test", "Test", "fictional", RightsClass::original_user_creation, 7, "#112233", "#AABBCC", {{"arc", "electric.projectile", 20, 4, 2}}};
    GeneRegistry registry; const auto genes = genes_from_seed(seed);
    check(registry.descriptor_count() == 5, "unexpected gene descriptor inventory");
    check(registry.validate(genes).ok(), "seed genes rejected");
    auto malformed = genes; malformed[0].payload = AbilityGene{seed.abilities[0]}; check(!registry.validate(malformed).ok(), "payload mismatch accepted");
    auto missing_identity = genes; missing_identity.erase(missing_identity.begin()); check(!registry.validate(missing_identity).ok(), "missing dependency accepted");

    check(evaluate_rights(RightsClass::original_user_creation, AssetUsage::commercial_export).allowed, "original work export denied");
    check(!evaluate_rights(RightsClass::unknown, AssetUsage::private_study).allowed, "unknown rights allowed");
    check(!evaluate_rights(RightsClass::research_only, AssetUsage::commercial_export).allowed, "research-only export allowed");
    check(!evaluate_rights(RightsClass::user_owned, AssetUsage::model_training).allowed, "training permission inferred from ownership");

    AssetGraph graph;
    const auto seed_id = graph.add("canonical-seed", "seed", {}, "canonicalize/1", "prov-seed", "portable", ArtifactValidation::valid);
    const auto ir_id = graph.add("sprite-ir", "ir", {seed_id}, "lower-ir/1", "prov-ir", "portable", ArtifactValidation::valid);
    const auto svg_id = graph.add("image/svg+xml", "svg", {ir_id}, "render-svg/1", "prov-svg", "svg", ArtifactValidation::valid);
    check(graph.size() == 3 && graph.find(svg_id) != nullptr, "asset graph insert failed");
    check(graph.add("image/svg+xml", "svg", {ir_id}, "render-svg/1", "prov-svg", "svg", ArtifactValidation::valid) == svg_id && graph.size() == 3, "idempotent graph add failed");
    const auto affected = graph.affected_by(std::span<const std::string>(&seed_id, 1)); check(affected.size() == 3, "transitive invalidation incomplete");
    bool missing_rejected = false; try { (void)graph.add("x", "x", {"missing"}, "pass", "prov", "target", ArtifactValidation::valid); } catch (const std::invalid_argument&) { missing_rejected = true; } check(missing_rejected, "missing dependency accepted");
    check(graph.canonical_manifest() == graph.canonical_manifest(), "asset manifest is nondeterministic");

    const ProvenanceRecord provenance{"prov-svg", ProvenanceActor::compiler, "gspl-sprites/0.1.0", "render-svg/1", {ir_id}, svg_id};
    check(canonical_provenance(provenance).find("render-svg/1") != std::string::npos, "provenance serialization lost operation");
    std::cout << "all gspl sprites domain tests passed\n"; return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

