#include "gspl/genes.hpp"
#include <algorithm>
#include <ranges>
#include <set>

namespace gspl {

GeneRegistry::GeneRegistry() {
    auto reg = [this](GeneKind k, std::string tid, std::vector<GeneKind> deps,
                       std::vector<GeneKind> conf, bool rt, bool trg) {
        builtins_[k] = {k, 1, std::move(tid), std::move(deps), std::move(conf), rt, trg};
    };
    reg(GeneKind::identity, "gspl.gene.identity", {}, {}, true, true);
    reg(GeneKind::classification, "gspl.gene.classification", {}, {}, true, true);
    reg(GeneKind::lineage, "gspl.gene.lineage", {GeneKind::identity}, {}, true, true);
    reg(GeneKind::form, "gspl.gene.form", {}, {}, true, true);
    reg(GeneKind::transformation, "gspl.gene.transformation", {GeneKind::form}, {}, true, true);
    reg(GeneKind::morphology, "gspl.gene.morphology", {GeneKind::form}, {}, true, true);
    reg(GeneKind::anatomy, "gspl.gene.anatomy", {GeneKind::morphology}, {}, true, true);
    reg(GeneKind::structure, "gspl.gene.structure", {GeneKind::morphology}, {}, true, true);
    reg(GeneKind::proportion, "gspl.gene.proportion", {GeneKind::structure}, {}, true, true);
    reg(GeneKind::appearance, "gspl.gene.appearance", {}, {}, true, true);
    reg(GeneKind::palette, "gspl.gene.palette", {GeneKind::appearance}, {}, true, true);
    reg(GeneKind::material, "gspl.gene.material", {}, {}, true, true);
    reg(GeneKind::surface, "gspl.gene.surface", {GeneKind::material}, {}, true, true);
    reg(GeneKind::equipment, "gspl.gene.equipment", {GeneKind::surface}, {}, false, true);
    reg(GeneKind::motion, "gspl.gene.motion", {GeneKind::form}, {}, true, true);
    reg(GeneKind::locomotion, "gspl.gene.locomotion", {GeneKind::motion}, {}, true, true);
    reg(GeneKind::animation, "gspl.gene.animation", {GeneKind::motion}, {}, true, true);
    reg(GeneKind::behavior, "gspl.gene.behavior", {GeneKind::animation}, {}, true, true);
    reg(GeneKind::perception, "gspl.gene.perception", {}, {}, true, false);
    reg(GeneKind::memory, "gspl.gene.memory", {}, {}, true, false);
    reg(GeneKind::emotion, "gspl.gene.emotion", {GeneKind::perception}, {}, true, false);
    reg(GeneKind::ability, "gspl.gene.ability", {}, {}, false, true);
    reg(GeneKind::combat, "gspl.gene.combat", {GeneKind::ability}, {}, false, true);
    reg(GeneKind::projectile, "gspl.gene.projectile", {GeneKind::combat}, {}, false, true);
    reg(GeneKind::effect, "gspl.gene.effect", {}, {}, true, true);
    reg(GeneKind::interaction, "gspl.gene.interaction", {}, {}, false, true);
    reg(GeneKind::physics, "gspl.gene.physics", {}, {}, true, true);
    reg(GeneKind::collision, "gspl.gene.collision", {GeneKind::physics}, {}, true, true);
    reg(GeneKind::audio_event, "gspl.gene.audio", {}, {}, true, true);
    reg(GeneKind::projection, "gspl.gene.projection", {}, {}, true, true);
    reg(GeneKind::optimization, "gspl.gene.optimization", {}, {}, false, true);
    reg(GeneKind::target, "gspl.gene.target", {}, {}, true, true);
    reg(GeneKind::rights, "gspl.gene.rights", {}, {}, false, false);
    reg(GeneKind::provenance, "gspl.gene.provenance", {GeneKind::rights}, {}, false, false);
}

bool GeneRegistry::register_gene(GeneDescriptor desc) {
    auto [it, inserted] = builtins_.try_emplace(desc.kind, std::move(desc));
    return inserted;
}

GeneDescriptor const* GeneRegistry::lookup(GeneKind kind) const {
    auto it = builtins_.find(kind);
    return it != builtins_.end() ? &it->second : nullptr;
}

DiagnosticResult GeneRegistry::check_dependencies(std::vector<GeneInstance> const& genes) const {
    DiagnosticResult dr;
    std::set<GeneKind> present;
    for (auto const& g : genes) present.insert(g.descriptor.kind);
    for (auto const& g : genes) {
        for (auto dep : g.descriptor.dependencies) {
            if (!present.contains(dep)) {
                dr.add_error(DiagnosticCode::GSPL_GENE_MISSING_DEPENDENCY,
                             "Gene " + std::to_string(static_cast<int>(g.descriptor.kind)) +
                             " requires gene " + std::to_string(static_cast<int>(dep)), {});
            }
        }
    }
    return dr;
}

DiagnosticResult GeneRegistry::check_conflicts(std::vector<GeneInstance> const& genes) const {
    DiagnosticResult dr;
    for (std::size_t i = 0; i < genes.size(); ++i) {
        for (auto conflict : genes[i].descriptor.conflicts) {
            for (std::size_t j = i + 1; j < genes.size(); ++j) {
                if (genes[j].descriptor.kind == conflict) {
                    dr.add_error(DiagnosticCode::GSPL_GENE_CONFLICT,
                                 "Gene " + std::to_string(static_cast<int>(genes[i].descriptor.kind)) +
                                 " conflicts with gene " + std::to_string(static_cast<int>(conflict)), {});
                }
            }
        }
    }
    return dr;
}

DiagnosticResult GeneRegistry::validate_composition(std::vector<GeneInstance> const& genes) const {
    auto result = check_dependencies(genes);
    auto conflicts = check_conflicts(genes);
    for (auto& d : conflicts.diagnostics) result.add(std::move(d));
    return result;
}

std::vector<GeneInstance> GeneRegistry::compose(std::vector<GeneInstance> base,
                                                  std::vector<GeneInstance> overrides) const {
    for (auto& ov : overrides) {
        auto it = std::ranges::find_if(base, [&](auto const& b) {
            return b.descriptor.kind == ov.descriptor.kind;
        });
        if (it != base.end()) {
            ov.is_override = true;
            *it = std::move(ov);
        } else {
            base.push_back(std::move(ov));
        }
    }
    return base;
}

std::vector<GeneKind> GeneRegistry::available_kinds() const {
    std::vector<GeneKind> kinds;
    for (auto const& [k, _] : builtins_) kinds.push_back(k);
    return kinds;
}

} // namespace gspl
