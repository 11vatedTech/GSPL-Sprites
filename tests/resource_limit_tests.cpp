#include "gspl/gspl.hpp"
#include "gspl_sprites/core.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        using namespace gspl::sprites;

        // ---- 1. Default limits pass for valid source ----
        {
            std::string source = "module m; entity E { rights PUBLIC; morphology {} }\n";
            auto result = enforce_resource_limits_source(source);
            check(result.ok(), "Default limits should pass for valid source");
        }

        // ---- 2. Source size limit: limit - 1 (pass) ----
        {
            ResourceLimits limits;
            limits.max_source_bytes = 200;
            std::string source(199, ' ');
            auto result = enforce_resource_limits_source(source, limits);
            check(result.ok(), "Source just under limit should pass");
        }

        // ---- 3. Source size limit: exact limit (pass) ----
        {
            ResourceLimits limits;
            limits.max_source_bytes = 200;
            std::string source(200, ' ');
            auto result = enforce_resource_limits_source(source, limits);
            check(result.ok(), "Source at exact limit should pass");
        }

        // ---- 4. Source size limit: limit + 1 (fail) ----
        {
            ResourceLimits limits;
            limits.max_source_bytes = 200;
            std::string source(201, 'x');
            auto result = enforce_resource_limits_source(source, limits);
            bool has_limit = false;
            for (auto const& d : result.diagnostics) {
                if (d.code.find("RESOURCE_SOURCE_SIZE") != std::string::npos) has_limit = true;
            }
            check(has_limit, "Source 1 byte over limit should fail with RESOURCE_SOURCE_SIZE");
        }

        // ---- 5. Token count limit: exact ----
        {
            ResourceLimits limits;
            limits.max_token_count = 10;
            std::string source = "module m;\nentity E { rights PUBLIC; morphology {} }\n";
            // This has about 10 tokens
            auto result = enforce_resource_limits_source(source, limits);
            check(result.ok(), "Token count at limit should pass");
        }

        // ---- 6. Zero-length source ----
        {
            std::string source;
            auto result = enforce_resource_limits_source(source);
            check(result.ok(), "Empty source should pass limits");
        }

        // ---- 7. Seed resource limits: forms at limit ----
        {
            SpriteSeed seed;
            seed.stable_id = "test";
            seed.name = "Test";
            ResourceLimits limits;
            limits.max_forms = 1;
            FormSeed f;
            f.id = "default";
            seed.forms.push_back(f);
            auto result = enforce_resource_limits(seed, limits);
            check(result.ok(), "Form count at limit should pass");
        }

        // ---- 8. Seed resource limits: forms over limit ----
        {
            SpriteSeed seed;
            seed.stable_id = "test";
            seed.name = "Test";
            ResourceLimits limits;
            limits.max_forms = 1;
            FormSeed f1, f2;
            f1.id = "a";
            f2.id = "b";
            seed.forms.push_back(f1);
            seed.forms.push_back(f2);
            auto result = enforce_resource_limits(seed, limits);
            bool has_limit = false;
            for (auto const& d : result.diagnostics) {
                if (d.code.find("RESOURCE_FORMS") != std::string::npos) has_limit = true;
            }
            check(has_limit, "2 forms with max_forms=1 should fail");
        }

        // ---- 9. String length limit ----
        {
            SpriteSeed seed;
            seed.stable_id = "test";
            seed.name = std::string(5000, 'x');
            ResourceLimits limits;
            limits.max_string_length = 100;
            auto result = enforce_resource_limits(seed, limits);
            bool has_limit = false;
            for (auto const& d : result.diagnostics) {
                if (d.code.find("RESOURCE_STRING_LENGTH") != std::string::npos) has_limit = true;
            }
            check(has_limit, "Overly long name should fail with RESOURCE_STRING_LENGTH");
        }

        // ---- 10. Large malicious seeds ----
        {
            SpriteSeed seed;
            seed.stable_id = "malicious";
            seed.name = "Malicious";
            ResourceLimits limits;
            limits.max_seed_bytes = 1;
            auto result = enforce_resource_limits(seed, limits);
            bool has_size = false;
            for (auto const& d : result.diagnostics) {
                if (d.code.find("RESOURCE_SEED_SIZE") != std::string::npos) has_size = true;
            }
            // Canonical seed is > 1 byte
            check(has_size, "Canonical seed over max_seed_bytes should fail");
        }

        // ---- 11. Max gene count ----
        {
            SpriteSeed seed;
            seed.stable_id = "gene-test";
            seed.name = "Gene Test";
            ResourceLimits limits;
            limits.max_gene_count = 0;
            // 1 form = counts as 1 gene
            seed.forms.push_back({"default"});
            auto result = enforce_resource_limits(seed, limits);
            bool has_limit = false;
            for (auto const& d : result.diagnostics) {
                if (d.code.find("RESOURCE_GENE_COUNT") != std::string::npos) has_limit = true;
            }
            check(has_limit, "1 form with max_gene_count=0 should fail");
        }

        // ---- 12. Compiler config limits ----
        {
            gspl::LexerConfig lc;
            check(lc.max_token_length == 1024, "Default max_token_length should be 1024");
            check(lc.max_string_length == 4096, "Default max_string_length should be 4096");
            check(lc.max_comment_length == 65536, "Default max_comment_length should be 65536");

            gspl::ParserConfig pc;
            check(pc.max_nesting_depth == 32, "Default max_nesting_depth should be 32");
            check(pc.max_declaration_count == 65536, "Default max_declaration_count should be 65536");

            gspl::ExpressionConfig ec;
            check(ec.max_depth == 64, "Default expression max_depth should be 64");
            check(ec.max_nodes == 65536, "Default expression max_nodes should be 65536");
            check(ec.max_evaluation_steps == 1048576, "Default max_evaluation_steps should be 1048576");
        }

        // ---- 13. ResourceLimits struct sizes match spec ----
        {
            gspl::sprites::ResourceLimits rl;
            check(rl.max_seed_bytes == 1'048'576, "max_seed_bytes default");
            check(rl.max_forms == 16, "max_forms default");
            check(rl.max_transformations == 32, "max_transformations default");
            check(rl.max_bones == 64, "max_bones default");
            check(rl.max_sockets == 32, "max_sockets default");
            check(rl.max_animation_clips == 32, "max_animation_clips default");
            check(rl.max_source_bytes == 1'048'576, "max_source_bytes default");
            check(rl.max_token_count == 65536, "max_token_count default");
            check(rl.max_nesting_depth == 32, "max_nesting_depth default");
            check(rl.max_expression_depth == 64, "max_expression_depth default");
            check(rl.max_sprite_ir_nodes == 65536, "max_sprite_ir_nodes default");
        }

        std::cout << "ALL RESOURCE LIMIT TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
