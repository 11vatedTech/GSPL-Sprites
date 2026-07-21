#pragma once
#include "gspl/genes.hpp"
#include "gspl/types.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace gspl {

enum class IrNodeKind : std::uint32_t {
    entity, gene, structure, form, transformation, material, animation,
    behavior, ability, projectile, effect, collision, resource,
    representation_plan, runtime_plan, package_plan, target_requirement
};

struct IrNode {
    IrNodeKind kind{};
    std::string identity;
    std::uint32_t schema_version{1};
    std::unordered_map<std::string, std::string> properties;
    std::vector<std::string> dependency_ids;
    virtual ~IrNode() = default;
};

struct EntityIr : IrNode {
    std::string entity_id;
    std::vector<GeneInstance> genes;
    std::vector<std::unique_ptr<IrNode>> children;
    EntityIr() { kind = IrNodeKind::entity; }
};

struct SpriteIr {
    std::string ir_version{"gspl-ir/1.0"};
    std::string entity_id;
    std::string seed_identity;
    std::unique_ptr<EntityIr> entity;
    std::vector<std::unique_ptr<IrNode>> representations;
    std::vector<std::unique_ptr<IrNode>> runtime_plans;
    std::vector<std::unique_ptr<IrNode>> package_plans;
};

class IrSerializer {
public:
    static std::string serialize(SpriteIr const& ir);
    static SpriteIr deserialize(std::string_view json);
    static DiagnosticResult validate(SpriteIr const& ir);
    static std::string diff(SpriteIr const& before, SpriteIr const& after);
    static std::string explain(SpriteIr const& ir, std::string node_id);
    static std::vector<std::string> dependencies(SpriteIr const& ir, std::string node_id);
};

} // namespace gspl
