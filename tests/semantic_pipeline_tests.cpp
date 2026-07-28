#include "gspl/gspl.hpp"
#include "gspl/cli.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"
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

        std::cout << "ALL SEMANTIC PIPELINE TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
