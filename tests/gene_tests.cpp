#include "gspl/gspl.hpp"
#include "gspl/genes.hpp"
#include "gspl/passes.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. GeneRegistry lookup ----
        {
            gspl::GeneRegistry reg;
            auto* id = reg.lookup(gspl::GeneKind::identity);
            check(id != nullptr, "Identity gene should be registered");
            check(id->type_id == "gspl.gene.identity", "Identity gene type_id should match");
            check(id->runtime_relevant, "Identity should be runtime_relevant");
            check(id->target_relevant, "Identity should be target_relevant");
        }

        // ---- 2. All built-in genes registered ----
        {
            gspl::GeneRegistry reg;
            auto kinds = reg.available_kinds();
            check(kinds.size() >= 34, "Should have at least 34 gene kinds");
            check(reg.lookup(gspl::GeneKind::classification) != nullptr, "Classification registered");
            check(reg.lookup(gspl::GeneKind::morphology) != nullptr, "Morphology registered");
            check(reg.lookup(gspl::GeneKind::appearance) != nullptr, "Appearance registered");
            check(reg.lookup(gspl::GeneKind::animation) != nullptr, "Animation registered");
            check(reg.lookup(gspl::GeneKind::behavior) != nullptr, "Behavior registered");
            check(reg.lookup(gspl::GeneKind::combat) != nullptr, "Combat registered");
            check(reg.lookup(gspl::GeneKind::provenance) != nullptr, "Provenance registered");
        }

        // ---- 3. Gene dependency validation ----
        {
            gspl::GeneRegistry reg;
            std::vector<gspl::GeneInstance> genes;
            gspl::GeneInstance g;
            g.descriptor = *reg.lookup(gspl::GeneKind::identity);
            genes.push_back(g);
            g.descriptor = *reg.lookup(gspl::GeneKind::classification);
            genes.push_back(g);

            auto result = reg.validate_composition(genes);
            check(result.ok(), "Identity+Classification should have no conflicts");
        }

        // ---- 4. Missing dependency detection ----
        {
            gspl::GeneRegistry reg;
            std::vector<gspl::GeneInstance> genes;
            gspl::GeneInstance g;
            // lineage depends on identity, but identity is missing
            g.descriptor = *reg.lookup(gspl::GeneKind::lineage);
            genes.push_back(g);
            auto result = reg.validate_composition(genes);
            bool has_missing = false;
            for (auto const& d : result.diagnostics) {
                if (d.code == gspl::DiagnosticCode::GSPL_GENE_MISSING_DEPENDENCY) has_missing = true;
            }
            check(has_missing, "Missing dependency should produce GSPL_GENE_MISSING_DEPENDENCY");
        }

        // ---- 5. Dependency chain: morphology -> form ----
        {
            gspl::GeneRegistry reg;
            std::vector<gspl::GeneInstance> genes;
            gspl::GeneInstance g;
            g.descriptor = *reg.lookup(gspl::GeneKind::form);
            genes.push_back(g);
            g.descriptor = *reg.lookup(gspl::GeneKind::morphology);
            genes.push_back(g);
            auto result = reg.validate_composition(genes);
            check(result.ok(), "Form+Morphology should validate (morphology depends on form)");
        }

        // ---- 6. Custom gene registration ----
        {
            gspl::GeneRegistry reg;
            gspl::GeneDescriptor custom;
            custom.kind = static_cast<gspl::GeneKind>(999);
            custom.type_id = "test.custom.gene";
            bool inserted = reg.register_gene(custom);
            check(inserted, "Custom gene should be registered");
            auto* lookup = reg.lookup(static_cast<gspl::GeneKind>(999));
            check(lookup != nullptr, "Custom gene should be findable");
            check(lookup->type_id == "test.custom.gene", "Custom gene type_id should match");
        }

        // ---- 7. Gene composition ----
        {
            gspl::GeneRegistry reg;
            std::vector<gspl::GeneInstance> base;
            std::vector<gspl::GeneInstance> overrides;

            gspl::GeneInstance g;
            g.descriptor = *reg.lookup(gspl::GeneKind::identity);
            g.values["stable_id"] = "base.entity";
            base.push_back(g);

            g.values["stable_id"] = "override.entity";
            overrides.push_back(g);

            auto composed = reg.compose(base, overrides);
            check(composed.size() == 1, "Composed should have 1 gene");
            // override should replace base when same kind
            check(composed[0].descriptor.kind == gspl::GeneKind::identity, "Composed kind should be identity");
        }

        // ---- 8. Full gene pipeline through compilation ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("genetest.gspl",
                "module genetest;\n"
                "entity E {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  gene identity {\n"
                "    stable_id: \"test.gene.entity\"\n"
                "  }\n"
                "  gene classification {\n"
                "    taxonomy: \"test.sample\"\n"
                "  }\n"
                "  morphology {}\n"
                "}\n");
            ctx.sources.register_buffer(std::move(buf));
            gspl::LexPhase lex; lex.execute(ctx);
            gspl::ParsePhase parse; parse.execute(ctx);
            ctx.diagnostics = {};
            gspl::NameResolvePhase name_res; name_res.execute(ctx);
            ctx.diagnostics = {};
            gspl::TypeCheckPhase type_check; type_check.execute(ctx);
            ctx.diagnostics = {};
            gspl::GeneCompositionPhase gene_comp; gene_comp.execute(ctx);
            bool has_gene_err = false;
            for (auto const& d : ctx.diagnostics.diagnostics) {
                if (d.severity >= gspl::DiagnosticSeverity::error) has_gene_err = true;
            }
            check(!has_gene_err, "Gene pipeline should succeed");
        }

        // ---- 9. Gene dependency conflict detection via compilation ----
        {
            gspl::GeneRegistry reg;
            std::vector<gspl::GeneInstance> genes;

            // Add form and morphology (valid)
            gspl::GeneInstance g;
            g.descriptor = *reg.lookup(gspl::GeneKind::form);
            genes.push_back(g);
            g.descriptor = *reg.lookup(gspl::GeneKind::morphology);
            genes.push_back(g);

            // Add identity
            g.descriptor = *reg.lookup(gspl::GeneKind::identity);
            genes.push_back(g);

            auto result = reg.validate_composition(genes);
            check(result.ok(), "Valid composition should pass");
        }

        std::cout << "ALL GENE TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
