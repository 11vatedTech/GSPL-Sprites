#pragma once
#include "gspl/source.hpp"
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gspl {

enum class GeneKind : std::uint32_t {
    identity, classification, lineage, form, transformation, morphology,
    anatomy, structure, proportion, appearance, palette, material, surface,
    equipment, motion, locomotion, animation, expression, behavior,
    perception, memory, emotion, ability, combat, projectile, effect,
    interaction, physics, collision, audio_event, projection,
    optimization, target, rights, provenance
};

struct GeneDescriptor {
    GeneKind kind{};
    std::uint32_t schema_version{1};
    std::string type_id;
    std::vector<GeneKind> dependencies;
    std::vector<GeneKind> conflicts;
    bool runtime_relevant{false};
    bool target_relevant{false};
};

struct GeneInstance {
    GeneDescriptor descriptor;
    std::unordered_map<std::string, std::string> values;
    std::string source_module;
    bool is_override{false};
};

class GeneRegistry {
public:
    GeneRegistry();
    bool register_gene(GeneDescriptor desc);
    GeneDescriptor const* lookup(GeneKind kind) const;
    DiagnosticResult validate_composition(std::vector<GeneInstance> const& genes) const;
    std::vector<GeneInstance> compose(std::vector<GeneInstance> base,
                                       std::vector<GeneInstance> overrides) const;
    std::vector<GeneKind> available_kinds() const;
private:
    std::unordered_map<GeneKind, GeneDescriptor> builtins_;
    DiagnosticResult check_dependencies(std::vector<GeneInstance> const& genes) const;
    DiagnosticResult check_conflicts(std::vector<GeneInstance> const& genes) const;
};

} // namespace gspl
