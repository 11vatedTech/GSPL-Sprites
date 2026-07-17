#include "gspl_sprites/domain.hpp"

#include <algorithm>
#include <deque>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
std::string json_escape(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) { case '\\': out += "\\\\"; break; case '"': out += "\\\""; break; case '\n': out += "\\n"; break; case '\r': out += "\\r"; break; case '\t': out += "\\t"; break; default: if (c < 0x20) throw std::runtime_error("control character in canonical value"); out += static_cast<char>(c); }
  }
  return out;
}

std::string artifact_validation_text(ArtifactValidation value) {
  switch (value) { case ArtifactValidation::pending: return "PENDING"; case ArtifactValidation::valid: return "VALID"; case ArtifactValidation::rejected: return "REJECTED"; }
  throw std::logic_error("unreachable artifact validation");
}
}

GeneRegistry::GeneRegistry() {
  const auto add = [this](std::string id, GeneKind kind, std::string purpose,
                          std::vector<GeneKind> deps = {}, bool runtime = false,
                          bool target = false) {
    descriptors_.push_back({std::move(id), 1, kind, std::move(purpose), std::move(deps), {}, runtime, target});
  };
  add("gspl.sprite.gene.identity", GeneKind::identity, "stable entity identity");
  add("gspl.sprite.gene.classification", GeneKind::classification, "entity taxonomy", {GeneKind::identity});
  add("gspl.sprite.gene.appearance", GeneKind::appearance, "canonical visual characteristics", {GeneKind::identity}, false, true);
  add("gspl.sprite.gene.ability", GeneKind::ability, "executable semantic capability", {GeneKind::identity}, true, true);
  add("gspl.sprite.gene.rights", GeneKind::rights, "rights and permitted-use authority", {GeneKind::identity}, false, true);
}

const GeneDescriptor* GeneRegistry::find(std::string_view type_id, std::uint32_t version) const noexcept {
  const auto found = std::ranges::find_if(descriptors_, [&](const auto& d) { return d.type_id == type_id && d.schema_version == version; });
  return found == descriptors_.end() ? nullptr : &*found;
}

ValidationResult GeneRegistry::validate(std::span<const GeneInstance> genes) const {
  ValidationResult result;
  std::set<GeneKind> kinds;
  std::set<std::string> unique_types;
  for (const auto& gene : genes) {
    const auto* descriptor = find(gene.type_id, gene.schema_version);
    if (descriptor == nullptr) { result.diagnostics.push_back({"SPRITE_GENE_UNKNOWN", "unregistered gene type/version: " + gene.type_id}); continue; }
    if (descriptor->kind != gene.kind) result.diagnostics.push_back({"SPRITE_GENE_KIND_MISMATCH", "gene kind does not match descriptor: " + gene.type_id});
    if (!unique_types.insert(gene.type_id + ":" + std::to_string(gene.schema_version)).second && gene.kind != GeneKind::ability)
      result.diagnostics.push_back({"SPRITE_GENE_DUPLICATE", "non-repeatable gene occurs more than once: " + gene.type_id});
    kinds.insert(gene.kind);
    const bool payload_matches =
      (gene.kind == GeneKind::identity && std::holds_alternative<IdentityGene>(gene.payload)) ||
      (gene.kind == GeneKind::classification && std::holds_alternative<ClassificationGene>(gene.payload)) ||
      (gene.kind == GeneKind::appearance && std::holds_alternative<AppearanceGene>(gene.payload)) ||
      (gene.kind == GeneKind::ability && std::holds_alternative<AbilityGene>(gene.payload)) ||
      (gene.kind == GeneKind::rights && std::holds_alternative<RightsGene>(gene.payload));
    if (!payload_matches) result.diagnostics.push_back({"SPRITE_GENE_PAYLOAD_MISMATCH", "typed payload does not match gene kind: " + gene.type_id});
  }
  for (const auto& gene : genes) if (const auto* descriptor = find(gene.type_id, gene.schema_version))
    for (const auto dependency : descriptor->dependencies) if (!kinds.contains(dependency)) result.diagnostics.push_back({"SPRITE_GENE_DEPENDENCY_MISSING", "gene dependency missing for " + gene.type_id});
  return result;
}

RightsDecision evaluate_rights(RightsClass classification, AssetUsage usage) {
  if (classification == RightsClass::prohibited) return {false, "SPRITE_RIGHTS_PROHIBITED", "prohibited material may not be processed"};
  if (classification == RightsClass::unknown) return {false, "SPRITE_RIGHTS_UNKNOWN", "rights must be resolved before use"};
  if (classification == RightsClass::restricted) return {usage == AssetUsage::private_study, "SPRITE_RIGHTS_RESTRICTED", "restricted material is limited to private study"};
  if (classification == RightsClass::research_only) return {usage == AssetUsage::private_study || usage == AssetUsage::research, "SPRITE_RIGHTS_RESEARCH_ONLY", "research-only material cannot be commercially exported or used for training"};
  if (usage == AssetUsage::model_training && classification == RightsClass::user_owned)
    return {false, "SPRITE_RIGHTS_TRAINING_NOT_GRANTED", "ownership alone does not establish model-training permission"};
  return {true, "SPRITE_RIGHTS_ALLOWED", "classification permits the requested use"};
}

std::string AssetGraph::add(std::string artifact_type, std::string_view bytes,
                            std::vector<std::string> dependencies,
                            std::string compiler_pass, std::string provenance_id,
                            std::string target, ArtifactValidation validation) {
  if (artifact_type.empty() || compiler_pass.empty() || provenance_id.empty() || target.empty()) throw std::invalid_argument("artifact metadata fields must not be empty");
  std::ranges::sort(dependencies);
  if (std::ranges::adjacent_find(dependencies) != dependencies.end()) throw std::invalid_argument("duplicate artifact dependency");
  for (const auto& dependency : dependencies) if (!nodes_.contains(dependency)) throw std::invalid_argument("artifact dependency is absent: " + dependency);
  const auto content_hash = sha256(bytes);
  std::ostringstream identity; identity << artifact_type << '\n' << 1 << '\n' << content_hash << '\n' << compiler_pass << '\n' << provenance_id << '\n' << target;
  for (const auto& dependency : dependencies) identity << '\n' << dependency;
  const auto id = sha256(identity.str());
  ArtifactNode node{id, std::move(artifact_type), 1, std::move(dependencies), std::move(compiler_pass), std::move(provenance_id), std::move(target), content_hash, validation};
  const auto [found, inserted] = nodes_.emplace(id, node);
  if (!inserted && (found->second.content_hash != node.content_hash || found->second.dependencies != node.dependencies)) throw std::logic_error("immutable artifact identity collision");
  return id;
}

const ArtifactNode* AssetGraph::find(std::string_view id) const noexcept {
  const auto found = nodes_.find(id); return found == nodes_.end() ? nullptr : &found->second;
}

std::vector<std::string> AssetGraph::affected_by(std::span<const std::string> changed) const {
  std::set<std::string> affected(changed.begin(), changed.end());
  for (const auto& id : affected) if (!nodes_.contains(id)) throw std::invalid_argument("changed artifact is absent: " + id);
  bool progressed = true;
  while (progressed) {
    progressed = false;
    for (const auto& [id, node] : nodes_) if (!affected.contains(id) && std::ranges::any_of(node.dependencies, [&](const auto& dep){ return affected.contains(dep); })) { affected.insert(id); progressed = true; }
  }
  return {affected.begin(), affected.end()};
}

std::string AssetGraph::canonical_manifest() const {
  std::ostringstream out; out << "{\"artifacts\":["; std::size_t index = 0;
  for (const auto& [id, node] : nodes_) {
    if (index++) out << ',';
    out << "{\"contentHash\":\"" << node.content_hash << "\",\"dependencies\":[";
    for (std::size_t i = 0; i < node.dependencies.size(); ++i) { if (i) out << ','; out << "\"" << node.dependencies[i] << "\""; }
    out << "],\"id\":\"" << id << "\",\"pass\":\"" << json_escape(node.compiler_pass) << "\",\"provenance\":\"" << json_escape(node.provenance_id) << "\",\"schemaVersion\":1,\"target\":\"" << json_escape(node.target) << "\",\"type\":\"" << json_escape(node.artifact_type) << "\",\"validation\":\"" << artifact_validation_text(node.validation) << "\"}";
  }
  out << "]}"; return out.str();
}

std::vector<GeneInstance> genes_from_seed(const SpriteSeed& seed) {
  std::vector<GeneInstance> genes;
  genes.push_back({"gspl.sprite.gene.identity", 1, GeneKind::identity, IdentityGene{seed.stable_id, seed.name}});
  genes.push_back({"gspl.sprite.gene.classification", 1, GeneKind::classification, ClassificationGene{seed.classification}});
  genes.push_back({"gspl.sprite.gene.appearance", 1, GeneKind::appearance, AppearanceGene{seed.primary_color, seed.accent_color}});
  for (const auto& ability : seed.abilities) genes.push_back({"gspl.sprite.gene.ability", 1, GeneKind::ability, AbilityGene{ability}});
  genes.push_back({"gspl.sprite.gene.rights", 1, GeneKind::rights, RightsGene{seed.rights}});
  return genes;
}

std::string canonical_provenance(const ProvenanceRecord& record) {
  if (record.id.empty() || record.actor_identity.empty() || record.operation.empty() || record.output.empty()) throw std::invalid_argument("provenance record is incomplete");
  auto inputs = record.inputs; std::ranges::sort(inputs);
  std::ostringstream out; out << "{\"actor\":" << static_cast<int>(record.actor) << ",\"actorIdentity\":\"" << json_escape(record.actor_identity) << "\",\"id\":\"" << json_escape(record.id) << "\",\"inputs\":[";
  for (std::size_t i = 0; i < inputs.size(); ++i) { if (i) out << ','; out << "\"" << json_escape(inputs[i]) << "\""; }
  out << "],\"operation\":\"" << json_escape(record.operation) << "\",\"output\":\"" << json_escape(record.output) << "\"}"; return out.str();
}
} // namespace gspl::sprites

