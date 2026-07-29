#include "gspl/gspl.hpp"
#include "gspl/cli.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"
#include "gspl/json.hpp"
#include "gspl_sprites/core.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
    try {
        // ---- 1. Full pipeline: modern GSPL source → all passes ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("voltfox.gspl",
                "module voltfox;\n"
                "entity Voltfox {\n"
                "  rights ORIGINAL_USER_CREATION PROHIBITED;\n"
                "  gene identity {\n"
                "    stable_id: \"original.voltfox\"\n"
                "    name: \"Voltfox\"\n"
                "  }\n"
                "  gene classification {\n"
                "    taxonomy: \"biological.fictional.electric-fox\"\n"
                "  }\n"
                "  gene appearance {\n"
                "    primary_color: \"#242038\"\n"
                "    accent_color: \"#56F1FF\"\n"
                "  }\n"
                "  form base {\n"
                "    max_health: 100\n"
                "    ability_envelope: 1.0\n"
                "  }\n"
                "  form storm {\n"
                "    max_health: 150\n"
                "    ability_envelope: 1.5\n"
                "  }\n"
                "  transformation ascend base storm {}\n"
                "  transformation descend storm base {}\n"
                "  ability directional_lightning {}\n"
                "  ability storm_lightning {}\n"
                "  resource energy resource {\n"
                "    min: 0\n"
                "    max: 100\n"
                "  }\n"
                "  morphology {}\n"
                "}\n");

            ctx.sources.register_buffer(std::move(buf));

            gspl::LexPhase lex;
            auto lex_diags = lex.execute(ctx);
            check(ctx.tokens.size() > 5, "Lex phase should produce tokens");

            gspl::ParsePhase parse;
            auto parse_diags = parse.execute(ctx);
            check(ctx.ast != nullptr, "Parse phase should produce AST");
            check(ctx.ast->name == "voltfox", "Module name should be 'voltfox'");
            check(parse_diags.diagnostics.empty(),
                   ("Parse should have no diagnostics, got: " +
                    (parse_diags.diagnostics.empty() ? "none" : parse_diags.diagnostics[0].message)).c_str());

            ctx.diagnostics = {};
            gspl::ModuleResolvePhase mod_res;
            mod_res.execute(ctx);
            ctx.diagnostics = {};

            gspl::NameResolvePhase name_res;
            name_res.execute(ctx);
            ctx.diagnostics = {};

            gspl::TypeCheckPhase type_check;
            type_check.execute(ctx);
            ctx.diagnostics = {};

            gspl::GeneCompositionPhase gene_comp;
            gene_comp.execute(ctx);

            gspl::IrGenPhase ir_gen;
            ir_gen.execute(ctx);
            check(ctx.ir.entity_id == "voltfox", "IR entity_id should be 'voltfox'");

            gspl::IrValidatePhase ir_val;
            ir_val.execute(ctx);

            gspl::IrOptimizePhase ir_opt;
            ir_opt.execute(ctx);
        }

        // ---- 2. Canonicalization from AST ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "entity TestEntity {\n"
                "  rights ORIGINAL_USER_CREATION PROHIBITED;\n"
                "  gene identity {\n"
                "    stable_id: \"test.entity\"\n"
                "  }\n"
                "  gene classification {\n"
                "    taxonomy: \"test.sample\"\n"
                "  }\n"
                "  gene appearance {\n"
                "    primary_color: \"#FF0000\"\n"
                "  }\n"
                "  form default {}\n"
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

            gspl::CanonicalizePhase canon;
            auto canon_diags = canon.execute(ctx);
            check(canon_diags.diagnostics.empty(),
                   ("Canonicalize should succeed, got: " +
                    (canon_diags.diagnostics.empty() ? "none" : canon_diags.diagnostics[0].message)).c_str());

            auto const& ce = ctx.canonical;
            check(!ce.stable_id.empty(), "Canonical stable_id should be non-empty");
            check(ce.stable_id == "test.entity", "Canonical stable_id should come from identity gene");
            check(!ce.classification.empty(), "Canonical classification should be non-empty");
            check(ce.classification == "test.sample", "Canonical classification should come from classification gene");
            check(!ce.rights.empty(), "Canonical rights should be non-empty");
            check(ce.rights == "ORIGINAL_USER_CREATION/PROHIBITED", "Canonical rights should be 'ORIGINAL_USER_CREATION/PROHIBITED'");
            check(ce.primary_color == "#FF0000", "Canonical primary_color should come from appearance gene");
        }

        // ---- 3. CanonicalEntity identity determinism ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("test.gspl",
                "module test;\n"
                "entity TestEntity {\n"
                "  rights ORIGINAL_USER_CREATION PROHIBITED;\n"
                "  gene identity {\n"
                "    stable_id: \"determinism.test\"\n"
                "  }\n"
                "  gene classification {\n"
                "    taxonomy: \"test.determinism\"\n"
                "  }\n"
                "  form default {}\n"
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
            gspl::CanonicalizePhase canon; canon.execute(ctx);

            auto const& ce = ctx.canonical;
            check(ce.stable_id == "determinism.test", "Canonical stable_id should come from identity gene");

            auto hash1 = gspl::CanonicalEntityIdentity(ctx.canonical).hash();
            auto hash2 = gspl::CanonicalEntityIdentity(ctx.canonical).hash();
            check(!hash1.empty(), "Identity hash should be non-empty");
            check(hash1 == hash2, "CanonicalEntity identity should be deterministic");
            check(hash1.size() == 64, "CanonicalEntity identity should be SHA-256 hex digest");
            auto canonical_payload = gspl::CanonicalEntityIdentity(ctx.canonical).serialized();
            check(hash1 == gspl::sprites::sha256(canonical_payload),
                  "Identity hash should equal SHA-256 of canonical payload");

            gspl::CanonicalEntity ce2;
            ce2.stable_id = "different.test";
            auto hash3 = gspl::CanonicalEntityIdentity(ce2).hash();
            check(hash1 != hash3, "Different stable_id should produce different hash");
        }

        // ---- 4. CanonicalEntityValidator ----
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "test.entity";
            ce.name = "Test Entity";
            ce.rights = "ORIGINAL_USER_CREATION";

            gspl::CanonicalEntityValidator validator;
            auto result = validator.validate(ce);
            check(result.diagnostics.empty(),
                   ("Valid CanonicalEntity should pass validation, got: " +
                    (result.diagnostics.empty() ? "none" : result.diagnostics[0].message)).c_str());
        }

        // ---- 5. CanonicalEntitySerializer round-trip ----
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "serialize.test";
            ce.name = "Serialize Test";
            ce.classification = "test.serialize";
            ce.rights = "ORIGINAL_USER_CREATION";
            ce.provenance_hash = "abcd1234";

            auto json = gspl::CanonicalEntitySerializer::to_json(ce);
            check(!json.empty(), "JSON serialization should produce output");
            check(json.find("serialize.test") != std::string::npos, "JSON should contain stable_id");
            check(json.find("ORIGINAL_USER_CREATION") != std::string::npos, "JSON should contain rights");

            auto yaml = gspl::CanonicalEntitySerializer::to_yaml(ce);
            check(!yaml.empty(), "YAML serialization should produce output");
        }

        // ---- 6. CanonicalEntityDiff ----
        {
            gspl::CanonicalEntity ce_a;
            ce_a.stable_id = "diff.a";
            ce_a.name = "Diff A";
            ce_a.classification = "test.diff";
            ce_a.rights = "ORIGINAL_USER_CREATION";

            gspl::CanonicalEntity ce_b = ce_a;
            ce_b.classification = "test.diff.modified";
            gspl::CanonicalForm cf;
            cf.id = "storm";
            ce_b.forms.push_back(cf);

            auto diffs = gspl::CanonicalEntityDiff::diff(ce_a, ce_b);
            check(!diffs.empty(), "Different entities should produce diffs");

            bool found_classification = false;
            bool found_forms = false;
            for (auto const& d : diffs) {
                if (d.field.find("classification") != std::string::npos) found_classification = true;
                if (d.field.find("forms") != std::string::npos) found_forms = true;
            }
            check(found_classification, "Diff should detect classification change");
            check(found_forms, "Diff should detect form count change");

            auto identical_diffs = gspl::CanonicalEntityDiff::diff(ce_a, ce_a);
            check(identical_diffs.empty(), "Identical entities should produce no diffs");
        }

        // ---- 7. SpriteIrLowering direct path ----
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "lowering.test";
            ce.name = "Lowering Test";
            ce.classification = "test.lowering";
            ce.rights = "ORIGINAL_USER_CREATION";
            ce.primary_color = "#242038";
            ce.accent_color = "#56F1FF";
            ce.provenance_hash = "hash123";
            ce.entropy_root = 42;
            gspl::CanonicalForm cf_default;
            cf_default.id = "default";
            ce.forms.push_back(cf_default);
            gspl::CanonicalPart cp_root;
            cp_root.name = "root";
            ce.morphology["root"] = cp_root;

            auto result = gspl::SpriteIrLowering::lower(ce);
            check(result.diagnostics.empty(),
                   ("Lowering should succeed, got: " +
                    (result.diagnostics.empty() ? "none" : result.diagnostics[0].message)).c_str());

            auto const& sir = result.sprite_ir;
            check(!sir.entity_id.empty(), "SpriteIr entity_id should be non-empty");
            check(sir.name == "Lowering Test", "SpriteIr name should match");
            check(sir.classification == "test.lowering", "SpriteIr classification should match");
            check(!sir.provenance_hash.empty(), "SpriteIr should have provenance_hash");
            check(sir.form_definitions.size() == 1, "SpriteIr should have 1 form definition");
            check(sir.morphology.size() == 1, "SpriteIr should have 1 morphology part");
        }

        // ---- 8. SpriteSeedLowering compatibility path ----
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "seed.lowering.test";
            ce.name = "Seed Lowering Test";
            ce.classification = "test.seed-lowering";
            ce.rights = "ORIGINAL_USER_CREATION";
            ce.primary_color = "#242038";
            ce.accent_color = "#56F1FF";
            ce.entropy_root = 99;
            gspl::CanonicalForm cf;
            cf.id = "default";
            ce.forms.push_back(cf);
            gspl::CanonicalTransformation ct;
            ct.id = "test-transform";
            ct.from_form = "default";
            ct.to_form = "default";
            ce.transformations.push_back(ct);
            gspl::CanonicalPart cp;
            cp.name = "root";
            ce.morphology["root"] = cp;

            auto seed = gspl::SpriteSeedLowering::lower(ce);
            check(seed.stable_id == "seed.lowering.test", "SpriteSeed stable_id should match");
            check(seed.classification == "test.seed-lowering", "SpriteSeed classification should match");
            check(seed.primary_color == "#242038", "SpriteSeed primary_color should match");
            check(seed.forms.size() == 1, "SpriteSeed should have 1 form");
        }

        // ---- 9. Production compile from lowered seed ----
        // Note: production validate() requires >=11 morphology parts and
        // cross-referenced forms+transformations when forms/morphology are
        // present. This test uses a minimal seed that avoids those paths.
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "production.test";
            ce.name = "Production Test";
            ce.classification = "test.production";
            ce.rights = "ORIGINAL_USER_CREATION";
            ce.primary_color = "#242038";
            ce.accent_color = "#56F1FF";
            ce.provenance_hash = "hash789";
            ce.entropy_root = 100;

            auto seed = gspl::SpriteSeedLowering::lower(ce);

            // Add required ability (1..64 required)
            gspl::sprites::AbilitySeed ability;
            ability.id = "test-ability";
            ability.effect = "generic.projectile";
            ability.cost = 5;
            ability.cooldown_ticks = 10;
            ability.active_ticks = 4;
            seed.abilities.push_back(ability);

            auto validation = gspl::sprites::validate(seed);
            if (!validation.ok()) {
                std::string errs;
                for (auto const& d : validation.diagnostics) errs += d.code + ": " + d.message + "; ";
                throw std::runtime_error("Production validation failed: " + errs);
            }

            auto prod_ir = gspl::sprites::compile(seed);
            check(!prod_ir.entity_id.empty(), "Production IR should have entity_id");
            check(prod_ir.entity_id == "production.test", "Production IR entity_id should match");
        }

        // ---- 10. PassManager full pipeline ----
        {
            gspl::SourceManager sm;
            sm.register_buffer(gspl::SourceBuffer::from_string("full.gspl",
                "module full;\n"
                "entity FullEntity {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  gene identity {\n"
                "    stable_id: \"full.test\"\n"
                "  }\n"
                "  gene classification {\n"
                "    taxonomy: \"test.full\"\n"
                "  }\n"
                "  ability default_attack {}\n"
                "}\n"));

            gspl::CompilationContext ctx;
            ctx.sources = std::move(sm);

            gspl::PassManager pm;
            pm.register_pass(std::make_unique<gspl::LexPhase>());
            pm.register_pass(std::make_unique<gspl::ParsePhase>());
            pm.register_pass(std::make_unique<gspl::ModuleResolvePhase>());
            pm.register_pass(std::make_unique<gspl::NameResolvePhase>());
            pm.register_pass(std::make_unique<gspl::TypeCheckPhase>());
            pm.register_pass(std::make_unique<gspl::GeneCompositionPhase>());
            pm.register_pass(std::make_unique<gspl::IrGenPhase>());
            pm.register_pass(std::make_unique<gspl::IrValidatePhase>());
            pm.register_pass(std::make_unique<gspl::IrOptimizePhase>());
            pm.register_pass(std::make_unique<gspl::CanonicalizePhase>());
            pm.register_pass(std::make_unique<gspl::CanonicalValidatePhase>());
            pm.register_pass(std::make_unique<gspl::SpriteIrLowerPhase>());
            pm.register_pass(std::make_unique<gspl::SeedLowerPhase>());

            std::vector<gspl::PassKind> targets = {
                gspl::PassKind::lex, gspl::PassKind::parse,
                gspl::PassKind::module_resolve, gspl::PassKind::name_resolve,
                gspl::PassKind::type_check, gspl::PassKind::gene_composition,
                gspl::PassKind::ir_gen, gspl::PassKind::ir_validate,
                gspl::PassKind::ir_optimize, gspl::PassKind::canonicalize,
                gspl::PassKind::canonical_validate,
                gspl::PassKind::sprite_ir_lower, gspl::PassKind::seed_lower
            };

            auto result = pm.run_passes(ctx, targets);
            for (auto const& d : result.diagnostics) {
                if (d.severity >= gspl::DiagnosticSeverity::error) {
                    throw std::runtime_error("PassManager pipeline error: " + d.message);
                }
            }
            check(ctx.composed_genes.size() >= 2, "GeneCompositionPhase should collect AST gene declarations");
            check(!ctx.canonical.stable_id.empty(),
                  "Canonical entity should be populated after pipeline");
            check(ctx.canonical.stable_id == "full.test",
                  ("Canonical stable_id should be 'full.test', got: '" + ctx.canonical.stable_id + "'").c_str());
        }

        // ---- 11. Reused PassManager must run every pass for each compilation ----
        {
            auto make_context = [](std::string mn, std::string en, std::string sid) {
                gspl::CompilationContext ctx;
                ctx.sources.register_buffer(gspl::SourceBuffer::from_string(mn + ".gspl",
                    "module " + mn + ";\n"
                    "entity " + en + " {\n"
                    "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                    "  gene identity { stable_id: \"" + sid + "\" }\n"
                    "  ability default_attack {}\n"
                    "}\n"));
                return ctx;
            };
            gspl::PassManager pm;
            pm.register_pass(std::make_unique<gspl::LexPhase>());
            pm.register_pass(std::make_unique<gspl::ParsePhase>());
            pm.register_pass(std::make_unique<gspl::ModuleResolvePhase>());
            pm.register_pass(std::make_unique<gspl::NameResolvePhase>());
            pm.register_pass(std::make_unique<gspl::TypeCheckPhase>());
            pm.register_pass(std::make_unique<gspl::GeneCompositionPhase>());
            pm.register_pass(std::make_unique<gspl::CanonicalizePhase>());
            std::vector<gspl::PassKind> targets = {gspl::PassKind::canonicalize};

            auto first = make_context("first", "FirstEntity", "first.semantic");
            auto first_result = pm.run_passes(first, targets);
            for (auto const& d : first_result.diagnostics)
                if (d.severity >= gspl::DiagnosticSeverity::error)
                    throw std::runtime_error("First compile failed: " + d.message);
            check(first.canonical.stable_id == "first.semantic", "First compile should use its identity gene");

            auto second = make_context("second", "SecondEntity", "second.semantic");
            auto second_result = pm.run_passes(second, targets);
            for (auto const& d : second_result.diagnostics)
                if (d.severity >= gspl::DiagnosticSeverity::error)
                    throw std::runtime_error("Second compile failed: " + d.message);
            check(second.canonical.stable_id == "second.semantic", "Second compile should not reuse first genes");
            check(second.tokens.size() > 5, "Second compile should run lexing");
            check(second.composed_genes.size() == 1, "Second compile should compose its own gene");
        }

        // ---- 12. Duplicate non-repeatable genes fail closed ----
        {
            gspl::CompilationContext ctx;
            ctx.sources.register_buffer(gspl::SourceBuffer::from_string("dup.gspl",
                "module dup;\n"
                "entity DupEntity {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  gene identity { stable_id: \"first\" }\n"
                "  gene identity { stable_id: \"second\" }\n"
                "}\n"));
            gspl::LexPhase lex; lex.execute(ctx);
            gspl::ParsePhase parse; parse.execute(ctx);
            ctx.diagnostics = {};
            gspl::GeneCompositionPhase gene_comp; gene_comp.execute(ctx);
            bool duplicate = false;
            for (auto const& d : ctx.diagnostics.diagnostics)
                if (d.code == gspl::DiagnosticCode::GSPL_GENE_DUPLICATE) duplicate = true;
            check(duplicate, "Duplicate identity genes should emit GSPL_GENE_DUPLICATE");
        }

        // ---- 13. Unknown and malformed gene payloads fail closed ----
        {
            gspl::CompilationContext unknown_ctx;
            unknown_ctx.sources.register_buffer(gspl::SourceBuffer::from_string("unk.gspl",
                "module unk;\n"
                "entity Unk {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  gene not_a_gene { value: \"x\" }\n"
                "}\n"));
            gspl::LexPhase lex; lex.execute(unknown_ctx);
            gspl::ParsePhase parse; parse.execute(unknown_ctx);
            unknown_ctx.diagnostics = {};
            gspl::GeneCompositionPhase gene_comp; gene_comp.execute(unknown_ctx);
            bool unknown = false;
            for (auto const& d : unknown_ctx.diagnostics.diagnostics)
                if (d.code == gspl::DiagnosticCode::GSPL_GENE_UNKNOWN) unknown = true;
            check(unknown, "Unknown gene kinds should emit GSPL_GENE_UNKNOWN");

            gspl::CompilationContext malformed_ctx;
            malformed_ctx.sources.register_buffer(gspl::SourceBuffer::from_string("bad.gspl",
                "module bad;\n"
                "entity Bad {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  gene appearance { primary_color: \"not-a-color\" }\n"
                "}\n"));
            gspl::LexPhase lex2; lex2.execute(malformed_ctx);
            gspl::ParsePhase parse2; parse2.execute(malformed_ctx);
            malformed_ctx.diagnostics = {};
            gspl::GeneCompositionPhase gene_comp2; gene_comp2.execute(malformed_ctx);
            bool invalid = false;
            for (auto const& d : malformed_ctx.diagnostics.diagnostics)
                if (d.code == gspl::DiagnosticCode::GSPL_GENE_INVALID_VALUE) invalid = true;
            check(invalid, "Malformed gene payloads should emit GSPL_GENE_INVALID_VALUE");
        }

        // ---- 14. CompilationContext reset clears semantic products ----
        {
            gspl::CompilationContext ctx;
            ctx.sources.register_buffer(gspl::SourceBuffer::from_string("reset.gspl",
                "module reset;\nentity ResetEntity { gene identity { stable_id: \"reset.before\" } }\n"));
            gspl::LexPhase lex; lex.execute(ctx);
            gspl::ParsePhase parse; parse.execute(ctx);
            gspl::GeneCompositionPhase gene_comp; gene_comp.execute(ctx);
            gspl::CanonicalizePhase canon; canon.execute(ctx);
            check(!ctx.tokens.empty(), "Should have tokens before reset");
            check(!ctx.composed_genes.empty(), "Should have genes before reset");
            check(!ctx.canonical.stable_id.empty(), "Should have canonical entity before reset");
            ctx.reset();
            check(ctx.tokens.empty(), "reset() should clear tokens");
            check(ctx.ast == nullptr, "reset() should clear AST");
            check(ctx.composed_genes.empty(), "reset() should clear composed genes");
            check(ctx.ir.entity_id.empty(), "reset() should clear Sprite IR");
            check(ctx.canonical.stable_id.empty(), "reset() should clear canonical entity");
            check(ctx.diagnostics.diagnostics.empty(), "reset() should clear diagnostics");
        }

        // ---- 15. Lowering diagnostics for unsupported semantics ----
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "diagnostics.test";
            ce.name = "Diagnostics Test";
            ce.classification = "test.diagnostics";
            ce.rights = "INVALID_CLASSIFICATION";
            gspl::CanonicalForm cf;
            cf.id = "default";
            ce.forms.push_back(cf);
            gspl::CanonicalPart cp;
            cp.name = "root";
            ce.morphology["root"] = cp;

            auto result = gspl::SpriteIrLowering::lower(ce);
            bool has_rights_diag = false;
            for (auto const& ld : result.diagnostics) {
                if (ld.code == gspl::LoweringDiagnostic::Code::RIGHTS_INVALID) {
                    has_rights_diag = true;
                }
            }
            check(has_rights_diag, "Lowering should produce RIGHTS_INVALID diagnostic for invalid rights");
        }

        // ---- 12. GenericBlock lowering (bones, sockets, clips, states, transitions, collisions) ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("generic.gspl",
                "module generic;\n"
                "entity GenericEntity {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  bone torso { parent: \"root\"; x: 0; y: 0; z: 0; length_mm: 50; }\n"
                "  socket muzzle { bone: \"torso\"; x: 10; y: 5; z: 0; }\n"
                "  clip idle {\n"
                "    track root { tick: 0; }\n"
                "    track root { tick: 10; }\n"
                "  }\n"
                "  state idle_state { clip: \"idle\"; }\n"
                "  transition idle_ready { to: \"ready\"; ability: \"test\"; threshold: 50; }\n"
                "  collision hitbox { type: \"CIRCLE\"; socket: \"torso\"; radius_mm: 20; }\n"
                "  window test_ability { shape: \"hitbox\"; start_tick: 2; duration_ticks: 5; }\n"
                "  runtime { aggression: 80; curiosity: 20; }\n"
                "  morphology {}\n"
                "}\n");

            ctx.sources.register_buffer(std::move(buf));
            gspl::LexPhase lex; lex.execute(ctx);
            gspl::ParsePhase parse; parse.execute(ctx);
            check(ctx.ast != nullptr, "Generic block test should produce AST");

            ctx.diagnostics = {};
            gspl::NameResolvePhase name_res; name_res.execute(ctx);
            ctx.diagnostics = {};
            gspl::TypeCheckPhase type_check; type_check.execute(ctx);
            ctx.diagnostics = {};
            gspl::GeneCompositionPhase gene_comp; gene_comp.execute(ctx);
            gspl::CanonicalizePhase canon; canon.execute(ctx);

            auto const& ce = ctx.canonical;
            check(ce.bones.size() == 1, "Should have 1 bone");
            check(ce.bones[0].id == "torso", "Bone should be 'torso'");
            check(ce.bones[0].parent == "root", "Bone parent should be 'root'");

            check(ce.sockets.size() == 1, "Should have 1 socket");
            check(ce.sockets[0].id == "muzzle", "Socket should be 'muzzle'");
            check(ce.sockets[0].bone == "torso", "Socket bone should be 'torso'");

            check(ce.clips.size() == 1, "Should have 1 clip");
            check(ce.clips[0].name == "idle", "Clip should be 'idle'");

            check(ce.states.size() == 1, "Should have 1 state");
            check(ce.states[0].name == "idle_state", "State should be 'idle_state'");
            check(!ce.states[0].clip_name.empty(), "State should have clip_name");

            check(ce.transitions.size() == 1, "Should have 1 transition");
            check(ce.transitions[0].from_state == "idle_ready", "Transition from_state should be 'idle_ready'");
            check(ce.transitions[0].to_state == "ready", "Transition to_state should be 'ready'");

            check(ce.collision_shapes.size() == 1, "Should have 1 collision shape");
            check(ce.collision_shapes[0].id == "hitbox", "Collision shape should be 'hitbox'");

            check(ce.collision_windows.size() == 1, "Should have 1 collision window");
            check(ce.collision_windows[0].ability_id == "test_ability", "Collision window ability should be 'test_ability'");

            check(ce.runtime.has_value(), "Should have runtime attributes");
            check(ce.runtime->aggression == 80, "Runtime aggression should be 80");
        }

        // ---- 16. CanonicalEntitySerializer from_json round-trip ----
        {
            gspl::CanonicalEntity ce;
            ce.stable_id = "roundtrip.test";
            ce.name = "Roundtrip Test";
            ce.classification = "test.roundtrip";
            ce.rights = "ORIGINAL_USER_CREATION";
            ce.rights_allow_export = true;
            ce.primary_color = "#242038";
            ce.accent_color = "#56F1FF";
            ce.provenance_hash = "abcd1234";
            ce.entropy_root = 42;

            auto json = gspl::CanonicalEntitySerializer::to_json(ce);
            auto result = gspl::CanonicalEntitySerializer::from_json(json);
            check(result.ok(), "from_json should succeed for valid JSON");
            check(result.value.has_value(), "from_json should produce a value for valid JSON");
            auto const& restored = *result.value;
            check(restored.stable_id == "roundtrip.test", "from_json should preserve stable_id");
            check(restored.name == "Roundtrip Test", "from_json should preserve name");
            check(restored.classification == "test.roundtrip", "from_json should preserve classification");
            check(restored.rights == "ORIGINAL_USER_CREATION", "from_json should preserve rights");
            check(restored.rights_allow_export == true, "from_json should preserve rights_allow_export");
            check(restored.primary_color == "#242038", "from_json should preserve primary_color");
            check(restored.accent_color == "#56F1FF", "from_json should preserve accent_color");
            check(restored.provenance_hash == "abcd1234", "from_json should preserve provenance_hash");
            check(restored.entropy_root == 42, "from_json should preserve entropy_root");

            auto hash_before = gspl::CanonicalEntityIdentity(ce).hash();
            auto hash_after = gspl::CanonicalEntityIdentity(restored).hash();
            check(hash_before == hash_after, "CanonicalEntity identity should survive JSON round-trip");
        }

        // ---- 17. from_json malformed input ----
        {
            auto result = gspl::CanonicalEntitySerializer::from_json("");
            check(!result.ok(), "from_json should reject empty input");
            check(!result.value.has_value(), "from_json should reject empty input");

            auto result2 = gspl::CanonicalEntitySerializer::from_json("not json");
            check(!result2.ok(), "from_json should reject non-JSON input");
            check(!result2.value.has_value(), "from_json should reject non-JSON input");
        }

        // ---- 18. from_json missing required stable_id ----
        {
            auto result = gspl::CanonicalEntitySerializer::from_json(
                "{\"name\": \"Missing ID\"}");
            check(!result.ok(), "from_json should reject entity without stable_id");
        }

        // ---- 19. IrSerializer round-trip ----
        {
            gspl::SpriteIr ir;
            ir.entity_id = "serialize-test";
            ir.seed_identity = "seed-42";
            ir.entity = std::make_unique<gspl::EntityIr>();
            ir.entity->entity_id = "serialize-test";
            ir.entity->identity = "test-identity";
            ir.entity->dependency_ids = {"dep-a", "dep-b"};

            auto json = gspl::IrSerializer::serialize(ir);
            check(!json.empty(), "serialize should produce output");
            check(json.find("gspl-ir/1.0") != std::string::npos, "serialize should embed ir_version");
            check(json.find("serialize-test") != std::string::npos, "serialize should embed entity_id");
            check(json.find("entity") != std::string::npos, "serialize should embed entity tree");
            check(json.find("dep-a") != std::string::npos, "serialize should embed dependency_ids");

            auto deser_result = gspl::IrSerializer::deserialize(json);
            check(deser_result.ok(), "deserialize should succeed for valid JSON");
            check(deser_result.value.has_value(), "deserialize should produce a value");
            auto const& restored = *deser_result.value;
            check(restored.entity_id == "serialize-test", "deserialize should restore entity_id");
            check(restored.seed_identity == "seed-42", "deserialize should restore seed_identity");
            check(restored.entity != nullptr, "deserialize should restore entity");
            check(restored.entity->entity_id == "serialize-test", "deserialize should restore entity.entity_id");
            check(restored.entity->identity == "test-identity", "deserialize should restore entity.identity");
            check(restored.entity->dependency_ids.size() == 2, "deserialize should restore dependency_ids");

            auto deps = gspl::IrSerializer::dependencies(ir, "");
            check(deps.size() == 2, "dependencies() should return entity dependency_ids");
            check(deps[0] == "dep-a", "dependencies should be sorted");
            check(deps[1] == "dep-b", "dependencies should be sorted");
        }

        // ---- 20. IrOptimizePhase canonical ordering ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("order.gspl",
                "module order;\n"
                "entity OrderTest {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  gene identity { stable_id: \"order.test\" }\n"
                "  gene classification { taxonomy: \"order.test\" }\n"
                "  ability z_ability {}\n"
                "  ability a_ability {}\n"
                "  bone z_bone { parent: \"root\"; }\n"
                "  bone a_bone { parent: \"root\"; }\n"
                "  form default {}\n"
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
            gspl::CanonicalizePhase canon; canon.execute(ctx);

            gspl::IrOptimizePhase opt;
            opt.execute(ctx);

            check(ctx.canonical.abilities.size() == 2, "Should have 2 abilities");
            check(ctx.canonical.abilities[0].id == "a_ability", "Abilities should be sorted (a before z)");
            check(ctx.canonical.abilities[1].id == "z_ability", "Abilities should be sorted (a before z)");

            check(ctx.canonical.bones.size() == 2, "Should have 2 bones");
            check(ctx.canonical.bones[0].id == "a_bone", "Bones should be sorted (a before z)");
            check(ctx.canonical.bones[1].id == "z_bone", "Bones should be sorted (a before z)");

            auto hash1 = gspl::CanonicalEntityIdentity(ctx.canonical).hash();
            gspl::IrOptimizePhase opt2;
            opt2.execute(ctx);
            auto hash2 = gspl::CanonicalEntityIdentity(ctx.canonical).hash();
            check(hash1 == hash2, "Repeated optimization should be idempotent");
        }

        // ---- 21. DEF-0012: Maximally populated CanonicalEntity round-trip ----
        {
            gspl::CanonicalEntity ce;
            ce.schema_version = "gspl.canonical-entity/1.0";
            ce.stable_id = "maximal.roundtrip.test";
            ce.name = "Maximal Roundtrip Entity";
            ce.classification = "biological.fictional.dragon";
            ce.rights = "ORIGINAL_USER_CREATION";
            ce.rights_allow_export = false;
            ce.entropy_root = 12345678901234567890ULL;
            ce.primary_color = "#FF4500";
            ce.accent_color = "#FFD700";
            ce.storm_primary_color = "#00BFFF";
            ce.storm_accent_color = "#1E90FF";
            ce.emissive_color = "#FF0000";
            ce.aura_color = "#FF69B4";
            ce.provenance_hash = "sha256:abcdef1234567890";
            ce.provenance_source = "gspl-source://maximal.fixture/1.0";
            ce.initial_state = "idle";

            // Forms
            gspl::CanonicalForm f1;
            f1.id = "base";
            f1.transformation_ids = {"ascend", "rage"};
            f1.resource_capacity = 200;
            f1.collision_scale = 1.0;
            f1.ability_envelope = 1.0;
            f1.max_health = 150;
            ce.forms.push_back(f1);

            gspl::CanonicalForm f2;
            f2.id = "storm";
            f2.transformation_ids = {"descend"};
            f2.resource_capacity = 300;
            f2.collision_scale = 1.5;
            f2.ability_envelope = 2.0;
            f2.max_health = 250;
            ce.forms.push_back(f2);

            // Transformations
            gspl::CanonicalTransformation t1;
            t1.id = "ascend";
            t1.from_form = "base";
            t1.to_form = "storm";
            t1.trigger_condition = "resource >= 100";
            t1.duration_ticks = 300;
            t1.resource_cost = 50;
            ce.transformations.push_back(t1);

            gspl::CanonicalTransformation t2;
            t2.id = "descend";
            t2.from_form = "storm";
            t2.to_form = "base";
            t2.trigger_condition = "resource <= 20";
            t2.duration_ticks = 200;
            t2.resource_cost = 25;
            ce.transformations.push_back(t2);

            gspl::CanonicalTransformation t3;
            t3.id = "rage";
            t3.from_form = "base";
            t3.to_form = "base";
            t3.trigger_condition = "health <= 30";
            t3.duration_ticks = 500;
            t3.resource_cost = 80;
            ce.transformations.push_back(t3);

            // Morphology
            gspl::CanonicalPart head;
            head.name = "head";
            head.parent = "root";
            head.x = 0; head.y = 80; head.z = 0;
            head.size_x = 30; head.size_y = 25; head.size_z = 20;
            head.color = "#FF4500";
            head.rotation_degrees = 0;
            head.emissive = false;
            head.electrical_marking = false;
            ce.morphology["head"] = head;

            gspl::CanonicalPart torso;
            torso.name = "torso";
            torso.parent = "head";
            torso.x = 0; torso.y = 40; torso.z = 0;
            torso.size_x = 40; torso.size_y = 50; torso.size_z = 25;
            torso.color = "#FF4500";
            torso.rotation_degrees = 0;
            torso.emissive = false;
            ce.morphology["torso"] = torso;

            // Form-specific morphology overrides
            gspl::CanonicalPart storm_head;
            storm_head.name = "head";
            storm_head.parent = "root";
            storm_head.x = 0; storm_head.y = 85; storm_head.z = 5;
            storm_head.size_x = 35; storm_head.size_y = 28; storm_head.size_z = 22;
            storm_head.color = "#00BFFF";
            storm_head.rotation_degrees = 0;
            storm_head.emissive = true;
            storm_head.electrical_marking = true;
            ce.form_morphology_overrides["storm"]["head"] = storm_head;

            // Abilities
            gspl::CanonicalAbility a1;
            a1.id = "fire_breath";
            a1.effect = "elemental.fire";
            a1.cost = 30;
            a1.cooldown_ticks = 60;
            a1.active_ticks = 20;
            a1.origin_socket = "mouth";
            a1.speed_mm_per_tick = 15.0;
            a1.collision_radius_mm = 8.0;
            a1.status_id = "burning";
            a1.status_duration_ticks = 40;
            ce.abilities.push_back(a1);

            gspl::CanonicalAbility sa1;
            sa1.id = "storm_fire_breath";
            sa1.effect = "elemental.fire.storm";
            sa1.cost = 50;
            sa1.cooldown_ticks = 40;
            sa1.active_ticks = 30;
            sa1.origin_socket = "mouth";
            sa1.speed_mm_per_tick = 25.0;
            sa1.collision_radius_mm = 14.0;
            ce.storm_abilities.push_back(sa1);

            // Bones
            gspl::CanonicalSkeletalBone root_bone;
            root_bone.id = "root";
            root_bone.x = 0; root_bone.y = 0; root_bone.z = 0;
            ce.bones.push_back(root_bone);

            gspl::CanonicalSkeletalBone b1;
            b1.id = "spine";
            b1.parent = "root";
            b1.x = 0; b1.y = 45; b1.z = 0;
            b1.scale_x = 1.0; b1.scale_y = 1.0;
            b1.length_mm = 80;
            b1.min_rotation = -30;
            b1.max_rotation = 30;
            ce.bones.push_back(b1);

            // Sockets
            gspl::CanonicalSocket s1;
            s1.id = "mouth";
            s1.bone = "spine";
            s1.x = 0; s1.y = 85; s1.z = 12;
            s1.scale_x = 1.0; s1.scale_y = 1.0;
            ce.sockets.push_back(s1);

            // Animation clips
            gspl::CanonicalAnimationClip clip;
            clip.name = "idle";
            clip.loop = true;
            gspl::CanonicalAnimationClip::Track track;
            track.bone = "spine";
            track.keys = {{0, "pose:idle_0"}, {30, "pose:idle_30"}, {60, "pose:idle_0"}};
            clip.tracks.push_back(track);
            clip.clip_events = {{0, "event:loop_start"}, {60, "event:loop_end"}};
            ce.clips.push_back(clip);

            gspl::CanonicalAnimationClip atk_clip;
            atk_clip.name = "attack";
            atk_clip.loop = false;
            gspl::CanonicalAnimationClip::Track atk_track;
            atk_track.bone = "spine";
            atk_track.keys = {{0, "pose:atk_0"}, {15, "pose:atk_15"}};
            atk_clip.tracks.push_back(atk_track);
            atk_clip.clip_events = {{10, "event:hit"}};
            ce.clips.push_back(atk_clip);

            // States
            gspl::CanonicalAnimationState state;
            state.name = "idle";
            state.clip_name = "idle";
            ce.states.push_back(state);

            gspl::CanonicalAnimationState attack_state;
            attack_state.name = "attacking";
            attack_state.clip_name = "attack";
            ce.states.push_back(attack_state);

            // Transitions
            gspl::CanonicalTransition trans;
            trans.from_state = "idle";
            trans.to_state = "attacking";
            trans.ability_id = "fire_breath";
            trans.comparison = "GREATER_EQUAL";
            trans.threshold = 80;
            trans.resource_cost = 5;
            trans.cooldown_ticks = 10;
            ce.transitions.push_back(trans);

            // Collision shapes
            gspl::CanonicalCollisionShape cshape;
            cshape.id = "body_hitbox";
            cshape.shape_type = "CIRCLE";
            cshape.socket = "spine";
            cshape.radius_mm = 25.0;
            cshape.offset_x = 0;
            cshape.offset_y = 30;
            cshape.scale_x = 1.0;
            cshape.scale_y = 1.0;
            ce.collision_shapes.push_back(cshape);

            // Collision windows
            gspl::CanonicalCollisionWindow cwin;
            cwin.ability_id = "fire_breath";
            cwin.shape_id = "body_hitbox";
            cwin.start_tick = 5;
            cwin.duration_ticks = 15;
            cwin.active = true;
            ce.collision_windows.push_back(cwin);

            // Resources
            gspl::CanonicalResource res;
            res.id = "mana";
            res.resource_type = "magical";
            res.min = 0;
            res.max = 200;
            res.initial = 100;
            ce.resources.push_back(res);

            // Runtime
            gspl::CanonicalRuntime rt;
            rt.aggression = 85;
            rt.curiosity = 30;
            rt.energy = 60;
            rt.loyalty = 90;
            gspl::CanonicalAnimationIntent intent;
            intent.behavior_state = "idle";
            intent.clip_name = "idle";
            rt.animation_intents.push_back(intent);
            ce.runtime = rt;

            // Genes — full typed round-trip
            gspl::GeneInstance gene1;
            gene1.descriptor.kind = gspl::GeneKind::identity;
            gene1.descriptor.schema_version = 1;
            gene1.descriptor.type_id = "gene.identity/1.0";
            gene1.source_module = "maximal_test";
            gene1.values["stable_id"] = std::string{"maximal.roundtrip.test"};
            gene1.values["name"] = std::string{"Maximal Roundtrip Entity"};
            ce.genes.push_back(gene1);

            gspl::GeneInstance gene2;
            gene2.descriptor.kind = gspl::GeneKind::appearance;
            gene2.descriptor.schema_version = 1;
            gene2.descriptor.type_id = "gene.appearance/1.0";
            gene2.source_module = "maximal_test";
            gene2.values["is_boss"] = true;
            gene2.values["level"] = static_cast<std::int64_t>(99);
            gene2.values["experience"] = static_cast<std::uint64_t>(999999);
            gene2.values["scale_factor"] = 1.5;
            gene2.values["tags"] = std::vector<std::string>{"dragon", "fire", "boss"};
            ce.genes.push_back(gene2);

            // Round-trip
            auto json = gspl::CanonicalEntitySerializer::to_json(ce);
            auto result = gspl::CanonicalEntitySerializer::from_json(json);
            check(result.ok(), "DEF-0012: maximally populated round-trip should succeed");
            check(result.value.has_value(), "DEF-0012: maximally populated round-trip should produce value");
            auto const& r = *result.value;

            // Root fields
            check(r.schema_version == ce.schema_version, "DEF-0012: schema_version round-trip");
            check(r.stable_id == ce.stable_id, "DEF-0012: stable_id round-trip");
            check(r.name == ce.name, "DEF-0012: name round-trip");
            check(r.classification == ce.classification, "DEF-0012: classification round-trip");
            check(r.rights == ce.rights, "DEF-0012: rights round-trip");
            check(r.rights_allow_export == ce.rights_allow_export, "DEF-0012: rights_allow_export round-trip");
            check(r.entropy_root == ce.entropy_root, "DEF-0012: entropy_root round-trip");
            check(r.primary_color == ce.primary_color, "DEF-0012: primary_color round-trip");
            check(r.accent_color == ce.accent_color, "DEF-0012: accent_color round-trip");
            check(r.storm_primary_color == ce.storm_primary_color, "DEF-0012: storm_primary_color round-trip");
            check(r.storm_accent_color == ce.storm_accent_color, "DEF-0012: storm_accent_color round-trip");
            check(r.emissive_color == ce.emissive_color, "DEF-0012: emissive_color round-trip");
            check(r.aura_color == ce.aura_color, "DEF-0012: aura_color round-trip");
            check(r.provenance_hash == ce.provenance_hash, "DEF-0012: provenance_hash round-trip");
            check(r.provenance_source == ce.provenance_source, "DEF-0012: provenance_source round-trip");
            check(r.initial_state == ce.initial_state,
                  ("DEF-0012: initial_state round-trip, expected '" + ce.initial_state + "' got '" + r.initial_state + "'").c_str());

            // Forms
            check(r.forms.size() == ce.forms.size(), "DEF-0012: forms count round-trip");
            for (std::size_t i = 0; i < ce.forms.size(); ++i) {
                check(r.forms[i].id == ce.forms[i].id, ("DEF-0012: form " + std::to_string(i) + " id").c_str());
                check(r.forms[i].resource_capacity == ce.forms[i].resource_capacity, "DEF-0012: form resource_capacity");
                check(r.forms[i].collision_scale == ce.forms[i].collision_scale, "DEF-0012: form collision_scale");
                check(r.forms[i].ability_envelope == ce.forms[i].ability_envelope, "DEF-0012: form ability_envelope");
                check(r.forms[i].max_health == ce.forms[i].max_health, "DEF-0012: form max_health");
                check(r.forms[i].transformation_ids.size() == ce.forms[i].transformation_ids.size(), "DEF-0012: form transformation_ids");
            }

            // Transformations
            check(r.transformations.size() == ce.transformations.size(), "DEF-0012: transformations count");
            for (std::size_t i = 0; i < ce.transformations.size(); ++i) {
                check(r.transformations[i].id == ce.transformations[i].id, "DEF-0012: transformation id");
                check(r.transformations[i].from_form == ce.transformations[i].from_form, "DEF-0012: transformation from_form");
                check(r.transformations[i].to_form == ce.transformations[i].to_form, "DEF-0012: transformation to_form");
                check(r.transformations[i].duration_ticks == ce.transformations[i].duration_ticks, "DEF-0012: transformation duration");
            }

            // Morphology
            check(r.morphology.size() == ce.morphology.size(), "DEF-0012: morphology count");
            for (auto const& [name, part] : ce.morphology) {
                check(r.morphology.count(name) == 1, ("DEF-0012: morphology key " + name).c_str());
                auto const& rp = r.morphology.at(name);
                check(rp.name == part.name, ("DEF-0012: morphology " + name + " name").c_str());
                check(rp.parent == part.parent, "DEF-0012: morphology parent");
                check(rp.x == part.x && rp.y == part.y, "DEF-0012: morphology position");
            }

            // Form morphology overrides round-trip
            check(r.form_morphology_overrides.size() == ce.form_morphology_overrides.size(),
                  "DEF-0012: form_morphology_overrides count");
            for (auto const& [form_name, parts] : ce.form_morphology_overrides) {
                check(r.form_morphology_overrides.count(form_name) == 1,
                      ("DEF-0012: form_morphology_overrides form '" + form_name + "' present").c_str());
                auto const& r_parts = r.form_morphology_overrides.at(form_name);
                check(r_parts.size() == parts.size(),
                      ("DEF-0012: form_morphology_overrides '" + form_name + "' parts count").c_str());
                for (auto const& [part_name, part] : parts) {
                    check(r_parts.count(part_name) == 1,
                          ("DEF-0012: form_morphology_overrides '" + form_name + "' part '" + part_name + "' present").c_str());
                    auto const& rp = r_parts.at(part_name);
                    check(rp.name == part.name, "DEF-0012: fmo part name");
                    check(rp.parent == part.parent, "DEF-0012: fmo part parent");
                    check(rp.emissive == part.emissive, "DEF-0012: fmo part emissive");
                    check(rp.electrical_marking == part.electrical_marking, "DEF-0012: fmo part marking");
                }
            }

            // Abilities
            check(r.abilities.size() == ce.abilities.size(), "DEF-0012: abilities count");
            check(r.storm_abilities.size() == ce.storm_abilities.size(), "DEF-0012: storm_abilities count");
            if (!r.abilities.empty()) {
                check(r.abilities[0].id == ce.abilities[0].id, "DEF-0012: ability id");
                check(r.abilities[0].cost == ce.abilities[0].cost, "DEF-0012: ability cost");
                check(r.abilities[0].status_duration_ticks == ce.abilities[0].status_duration_ticks, "DEF-0012: ability status_duration");
            }

            // Bones
            check(r.bones.size() == ce.bones.size(), "DEF-0012: bones count");
            if (!r.bones.empty()) {
                check(r.bones[0].id == ce.bones[0].id, "DEF-0012: bone id");
                check(r.bones[0].length_mm == ce.bones[0].length_mm, "DEF-0012: bone length");
            }

            // Sockets
            check(r.sockets.size() == ce.sockets.size(), "DEF-0012: sockets count");

            // Clips
            check(r.clips.size() == ce.clips.size(), "DEF-0012: clips count");
            if (!r.clips.empty()) {
                check(r.clips[0].name == ce.clips[0].name, "DEF-0012: clip name");
                check(r.clips[0].tracks.size() == ce.clips[0].tracks.size(), "DEF-0012: clip tracks count");
                check(r.clips[0].clip_events.size() == ce.clips[0].clip_events.size(), "DEF-0012: clip events count");
            }

            // States
            check(r.states.size() == ce.states.size(), "DEF-0012: states count");

            // Transitions
            check(r.transitions.size() == ce.transitions.size(), "DEF-0012: transitions count");

            // Collision
            check(r.collision_shapes.size() == ce.collision_shapes.size(), "DEF-0012: collision_shapes count");
            check(r.collision_windows.size() == ce.collision_windows.size(), "DEF-0012: collision_windows count");

            // Resources
            check(r.resources.size() == ce.resources.size(), "DEF-0012: resources count");
            if (!r.resources.empty()) {
                check(r.resources[0].id == ce.resources[0].id, "DEF-0012: resource id");
                check(r.resources[0].max == ce.resources[0].max, "DEF-0012: resource max");
            }

            // Runtime
            check(r.runtime.has_value() == ce.runtime.has_value(), "DEF-0012: runtime presence");
            if (r.runtime && ce.runtime) {
                check(r.runtime->aggression == ce.runtime->aggression, "DEF-0012: runtime aggression");
                check(r.runtime->curiosity == ce.runtime->curiosity, "DEF-0012: runtime curiosity");
                check(r.runtime->energy == ce.runtime->energy, "DEF-0012: runtime energy");
                check(r.runtime->loyalty == ce.runtime->loyalty, "DEF-0012: runtime loyalty");
                check(r.runtime->animation_intents.size() == ce.runtime->animation_intents.size(), "DEF-0012: runtime animation_intents");
            }

            // Identity hash survival
            auto hash_before = gspl::CanonicalEntityIdentity(ce).hash();
            auto hash_after = gspl::CanonicalEntityIdentity(r).hash();
            check(hash_before == hash_after, "DEF-0012: identity hash survives maximally populated round-trip");

            // Gene round-trip
            check(r.genes.size() == ce.genes.size(),
                  ("DEF-0012: genes count round-trip, expected " + std::to_string(ce.genes.size()) +
                   " got " + std::to_string(r.genes.size())).c_str());
            for (std::size_t gi = 0; gi < ce.genes.size() && gi < r.genes.size(); ++gi) {
                check(r.genes[gi].descriptor.kind == ce.genes[gi].descriptor.kind,
                      ("DEF-0012: gene " + std::to_string(gi) + " kind").c_str());
                check(r.genes[gi].descriptor.schema_version == ce.genes[gi].descriptor.schema_version,
                      ("DEF-0012: gene " + std::to_string(gi) + " schema").c_str());
                check(r.genes[gi].descriptor.type_id == ce.genes[gi].descriptor.type_id,
                      ("DEF-0012: gene " + std::to_string(gi) + " type_id").c_str());
                check(r.genes[gi].source_module == ce.genes[gi].source_module,
                      ("DEF-0012: gene " + std::to_string(gi) + " source_module").c_str());
                check(r.genes[gi].values.size() == ce.genes[gi].values.size(),
                      ("DEF-0012: gene " + std::to_string(gi) + " values count").c_str());
                // Verify each value round-trips with correct type
                for (auto const& [vk, vv] : ce.genes[gi].values) {
                    check(r.genes[gi].values.count(vk) == 1,
                          ("DEF-0012: gene " + std::to_string(gi) + " value key '" + vk + "' present").c_str());
                    auto const& rv = r.genes[gi].values.at(vk);
                    check(rv.index() == vv.index(),
                          ("DEF-0012: gene " + std::to_string(gi) + " value '" + vk + "' type preserved").c_str());
                }
            }
            // Verify specific GeneValue types survived
            if (r.genes.size() >= 2) {
                auto const& g2 = r.genes[1];
                check(g2.values.count("is_boss") == 1, "DEF-0012: gene bool value key present");
                check(g2.values.count("level") == 1, "DEF-0012: gene int64 value key present");
                check(g2.values.count("experience") == 1, "DEF-0012: gene uint64 value key present");
                check(g2.values.count("scale_factor") == 1, "DEF-0012: gene double value key present");
                check(g2.values.count("tags") == 1, "DEF-0012: gene string_list value key present");
                // Check type preservation via variant index
                check(std::get_if<bool>(&g2.values.at("is_boss")) != nullptr, "DEF-0012: is_boss is bool");
                check(std::get_if<std::int64_t>(&g2.values.at("level")) != nullptr, "DEF-0012: level is int64");
                check(std::get_if<std::uint64_t>(&g2.values.at("experience")) != nullptr, "DEF-0012: experience is uint64");
                check(std::get_if<double>(&g2.values.at("scale_factor")) != nullptr, "DEF-0012: scale_factor is double");
                check(std::get_if<std::vector<std::string>>(&g2.values.at("tags")) != nullptr, "DEF-0012: tags is string_list");
            }

            // SpriteIr round-trip with genes
            gspl::SpriteIr ir;
            ir.entity_id = "maximal-ir-test";
            ir.seed_identity = "seed-maximal";
            ir.entity = std::make_unique<gspl::EntityIr>();
            ir.entity->entity_id = "maximal-ir-test";
            ir.entity->identity = "ir-identity";
            ir.entity->schema_version = 1;
            ir.entity->genes = ce.genes;
            ir.entity->dependency_ids = {"gene.identity/1.0", "gene.appearance/1.0"};

            auto ir_json = gspl::IrSerializer::serialize(ir);
            auto ir_deser = gspl::IrSerializer::deserialize(ir_json);
            check(ir_deser.ok(), "DEF-0012: SpriteIr maximally populated round-trip should succeed");
            check(ir_deser.value.has_value(), "DEF-0012: SpriteIr maximally populated should produce value");
            auto const& restored_ir = *ir_deser.value;
            check(restored_ir.entity_id == ir.entity_id, "DEF-0012: SpriteIr entity_id round-trip");
            check(restored_ir.entity != nullptr, "DEF-0012: SpriteIr entity present");
            check(restored_ir.entity->genes.size() == ir.entity->genes.size(),
                  ("DEF-0012: SpriteIr genes count, expected " + std::to_string(ir.entity->genes.size()) +
                   " got " + std::to_string(restored_ir.entity->genes.size())).c_str());

            // Verify GeneValue type survival in IR round-trip
            if (!restored_ir.entity->genes.empty()) {
                auto const& g = restored_ir.entity->genes.back();
                check(g.values.count("is_boss") == 1, "DEF-0012: gene bool value key present");
                check(g.values.count("level") == 1, "DEF-0012: gene int64 value key present");
                check(g.values.count("experience") == 1, "DEF-0012: gene uint64 value key present");
                check(g.values.count("scale_factor") == 1, "DEF-0012: gene double value key present");
                check(g.values.count("tags") == 1, "DEF-0012: gene string_list value key present");
            }
        }

        // ---- 22. DEF-0014: Resource-limit boundary tests ----
        {
            // max_input_bytes: limit-1, limit, limit+1
            {
                gspl::BoundedJsonConfig cfg;
                cfg.max_input_bytes = 10;

                // limit-1: 9 bytes should parse
                std::string valid = "{\"a\":1}"; // 7 bytes
                gspl::BoundedJsonReader r1(valid, cfg);
                check(!r1.has_error(), "DEF-0014: 7 bytes under 10-byte limit should succeed");

                // limit+1: 11+ bytes should fail
                std::string over = "{\"abcdef\":1}"; // 12 bytes
                gspl::BoundedJsonReader r2(over, cfg);
                check(r2.has_error(), "DEF-0014: 12 bytes over 10-byte limit should fail");
            }

            // max_nesting_depth: limit-1, limit, limit+1
            {
                gspl::BoundedJsonConfig cfg;
                cfg.max_nesting_depth = 3;

                // limit-1: depth 2 should work
                std::string d2 = "{\"a\":{\"b\":1}}";
                gspl::BoundedJsonReader r1(d2, cfg);
                r1.skip_value();
                check(!r1.has_error(), "DEF-0014: depth 2 under limit 3 should succeed");

                // limit+1: depth 4 should fail
                std::string d4 = "{\"a\":{\"b\":{\"c\":{\"d\":1}}}}";
                gspl::BoundedJsonReader r2(d4, cfg);
                r2.skip_value();
                check(r2.has_error(), "DEF-0014: depth 4 over limit 3 should fail");
            }

            // max_string_length: limit-1, limit, limit+1
            {
                gspl::BoundedJsonConfig cfg;
                cfg.max_string_length = 10;

                // limit-1: string of 9 chars should work
                std::string s9 = "\"123456789\"";
                gspl::BoundedJsonReader r1(s9, cfg);
                auto val = r1.read_string();
                check(val == "123456789", "DEF-0014: 9-char string under 10 limit should succeed");

                // limit+1: string of 11 chars should fail
                std::string s11 = "\"12345678901\"";
                gspl::BoundedJsonReader r2(s11, cfg);
                r2.read_string();
                check(r2.has_error(), "DEF-0014: 11-char string over 10 limit should fail");
            }

            // max_object_members: limit-1, limit, limit+1
            {
                gspl::BoundedJsonConfig cfg;
                cfg.max_object_members = 3;

                // limit-1: 2 members should work
                std::string m2 = "{\"a\":1,\"b\":2}";
                gspl::BoundedJsonReader r1(m2, cfg);
                r1.skip_value();
                check(!r1.has_error(), "DEF-0014: 2 members under limit 3 should succeed");

                // limit+1: 4 members should fail
                std::string m4 = "{\"a\":1,\"b\":2,\"c\":3,\"d\":4}";
                gspl::BoundedJsonReader r2(m4, cfg);
                r2.skip_value();
                check(r2.has_error(), "DEF-0014: 4 members over limit 3 should fail");
            }

            // max_array_length: limit-1, limit, limit+1
            {
                gspl::BoundedJsonConfig cfg;
                cfg.max_array_length = 3;

                // limit-1: 2 elements should work
                std::string a2 = "[1,2]";
                gspl::BoundedJsonReader r1(a2, cfg);
                r1.skip_value();
                check(!r1.has_error(), "DEF-0014: 2 elements under limit 3 should succeed");

                // limit+1: 4 elements should fail
                std::string a4 = "[1,2,3,4]";
                gspl::BoundedJsonReader r2(a4, cfg);
                r2.skip_value();
                check(r2.has_error(), "DEF-0014: 4 elements over limit 3 should fail");
            }

            // edge: at-limit (3) should work
            {
                gspl::BoundedJsonConfig cfg;
                cfg.max_array_length = 3;
                std::string a3 = "[1,2,3]";
                gspl::BoundedJsonReader r(a3, cfg);
                r.skip_value();
                check(!r.has_error(), "DEF-0014: 3 elements at limit 3 should succeed");
            }
        }

        // ---- 23. DEF-0015: Malformed-input tests for production deserializers ----
        {
            // CanonicalEntity: truncated JSON
            auto r1 = gspl::CanonicalEntitySerializer::from_json(
                "{\"stable_id\": \"test\", \"name\": \"Test\"");
            check(!r1.ok(), "DEF-0015: from_json should reject truncated JSON");

            // CanonicalEntity: wrong field type (string where number expected)
            auto r2 = gspl::CanonicalEntitySerializer::from_json(
                "{\"stable_id\": \"test\", \"entropy_root\": \"not-a-number\"}");
            check(!r2.ok(),
                  "DEF-0015: from_json should reject wrong field type");
            check(!r2.value.has_value(),
                  "DEF-0015: from_json should return no value for wrong field type");

            // CanonicalEntity: malformed gene value (bool tag with string value)
            gspl::CanonicalEntity ce;
            ce.stable_id = "malformed-gene-test";
            ce.name = "Test";
            ce.rights = "ORIGINAL_USER_CREATION";
            auto json = gspl::CanonicalEntitySerializer::to_json(ce);
            // Inject a malformed gene: bool tag with a string value
            auto bad_json = json;
            auto pos = bad_json.find("\"gene_count\"");
            if (pos != std::string::npos) {
                bad_json.insert(pos,
                    "\"genes\": [{\"kind\":1,\"schema\":1,\"type\":\"test\","
                    "\"source\":\"test\",\"values\":{\"bad\":{\"t\":1,\"v\":\"not-bool\"}}}],");
                auto r3 = gspl::CanonicalEntitySerializer::from_json(bad_json);
                // DEF-0015: Malformed gene values must fail closed.
                // Both semantics.cpp and ir.cpp now use BoundedJsonReader with fail-closed parsing.
                check(!r3.ok(), "DEF-0015: from_json should reject malformed gene injection");
                check(!r3.value.has_value(), "DEF-0015: from_json should not return a value for malformed genes");
            }

            // IrSerializer: empty input
            auto ir1 = gspl::IrSerializer::deserialize("");
            check(!ir1.ok(), "DEF-0015: IrSerializer should reject empty input");

            // IrSerializer: non-JSON input
            auto ir2 = gspl::IrSerializer::deserialize("garbage");
            check(!ir2.ok(), "DEF-0015: IrSerializer should reject non-JSON input");

            // IrSerializer: missing entity
            auto ir3 = gspl::IrSerializer::deserialize(
                "{\"ir_version\":\"gspl-ir/1.0\",\"entity_id\":\"test\"}");
            check(!ir3.ok(), "DEF-0015: IrSerializer should reject input missing entity");

            // IrSerializer: truncated entity (must fail with document closure)
            auto ir4 = gspl::IrSerializer::deserialize(
                "{\"ir_version\":\"gspl-ir/1.0\",\"entity_id\":\"truncated\",\"seed_identity\":\"seed\",\"entity\":{\"kind\":0");
            check(!ir4.ok(),
                  "DEF-0015: IrSerializer must reject truncated entity (doc closure)");
            check(!ir4.value.has_value(),
                  "DEF-0015: truncated entity must return no value");

            // IrSerializer: missing entity_id
            auto ir5 = gspl::IrSerializer::deserialize(
                "{\"ir_version\":\"gspl-ir/1.0\",\"seed_identity\":\"seed\"}");
            check(!ir5.ok(), "DEF-0015: IrSerializer should reject input missing entity_id");
        }

        std::cout << "ALL SEMANTIC PIPELINE TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
