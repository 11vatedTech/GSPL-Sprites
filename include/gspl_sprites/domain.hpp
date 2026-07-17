#pragma once

#include "gspl_sprites/core.hpp"

#include <map>
#include <set>
#include <span>
#include <variant>

namespace gspl::sprites {

enum class GeneKind {
  identity, classification, lineage, morphology, anatomy, proportion,
  appearance, palette, material, surface, clothing, equipment, style, pose,
  motion, locomotion, animation, expression, behavior, personality, emotion,
  perception, ability, combat, projectile, effect, transformation, interaction,
  physics, collision, audio, voice_event, camera, projection, optimization,
  target, provenance, rights
};

struct IdentityGene { std::string stable_id; std::string name; };
struct ClassificationGene { std::string taxonomy; };
struct AppearanceGene { std::string primary_color; std::string accent_color; };
struct AbilityGene { AbilitySeed ability; };
struct RightsGene { RightsClass classification; };
using GenePayload = std::variant<IdentityGene, ClassificationGene, AppearanceGene, AbilityGene, RightsGene>;

struct GeneInstance {
  std::string type_id;
  std::uint32_t schema_version{1};
  GeneKind kind{};
  GenePayload payload;
};

struct GeneDescriptor {
  std::string type_id;
  std::uint32_t schema_version{1};
  GeneKind kind{};
  std::string purpose;
  std::vector<GeneKind> dependencies;
  std::vector<GeneKind> conflicts;
  bool runtime_relevant{};
  bool target_relevant{};
};

class GeneRegistry final {
public:
  GeneRegistry();
  [[nodiscard]] const GeneDescriptor* find(std::string_view type_id, std::uint32_t version) const noexcept;
  [[nodiscard]] ValidationResult validate(std::span<const GeneInstance> genes) const;
  [[nodiscard]] std::size_t descriptor_count() const noexcept { return descriptors_.size(); }
private:
  std::vector<GeneDescriptor> descriptors_;
};

enum class AssetUsage { private_study, research, commercial_export, model_training };
struct RightsDecision { bool allowed{}; std::string code; std::string explanation; };
[[nodiscard]] RightsDecision evaluate_rights(RightsClass classification, AssetUsage usage);

enum class ProvenanceActor { user, compiler, algorithm, model };
struct ProvenanceRecord {
  std::string id;
  ProvenanceActor actor{};
  std::string actor_identity;
  std::string operation;
  std::vector<std::string> inputs;
  std::string output;
};

enum class ArtifactValidation { pending, valid, rejected };
struct ArtifactNode {
  std::string id;
  std::string artifact_type;
  std::uint32_t schema_version{1};
  std::vector<std::string> dependencies;
  std::string compiler_pass;
  std::string provenance_id;
  std::string target;
  std::string content_hash;
  ArtifactValidation validation{ArtifactValidation::pending};
};

class AssetGraph final {
public:
  [[nodiscard]] std::string add(std::string artifact_type, std::string_view bytes,
                                std::vector<std::string> dependencies,
                                std::string compiler_pass, std::string provenance_id,
                                std::string target, ArtifactValidation validation);
  [[nodiscard]] const ArtifactNode* find(std::string_view id) const noexcept;
  [[nodiscard]] std::vector<std::string> affected_by(std::span<const std::string> changed) const;
  [[nodiscard]] std::string canonical_manifest() const;
  [[nodiscard]] std::size_t size() const noexcept { return nodes_.size(); }
private:
  std::map<std::string, ArtifactNode, std::less<>> nodes_;
};

[[nodiscard]] std::vector<GeneInstance> genes_from_seed(const SpriteSeed& seed);
[[nodiscard]] std::string canonical_provenance(const ProvenanceRecord& record);

} // namespace gspl::sprites

