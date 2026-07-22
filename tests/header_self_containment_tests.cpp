#include "gspl/sdk.hpp"
#include "gspl/utf8.hpp"
#include "gspl/source.hpp"
#include "gspl/lexer.hpp"
#include "gspl/ast.hpp"
#include "gspl/parser.hpp"
#include "gspl/modules.hpp"
#include "gspl/types.hpp"
#include "gspl/expressions.hpp"
#include "gspl/genes.hpp"
#include "gspl/ir.hpp"
#include "gspl/passes.hpp"
#include "gspl/semantics.hpp"
#include "gspl/lowering.hpp"
#include "gspl/cache.hpp"
#include "gspl/provider.hpp"
#include "gspl/legacy.hpp"
#include "gspl/diagnostics.hpp"
#include "gspl/cli.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace { void check(bool v, const char* m) { if (!v) throw std::runtime_error(m); } }

int main() {
    try {
        // 1. GsplContext lifecycle
        {
            gspl::GsplContext ctx;
            check(ctx.version() == "1.0.0", "version should be 1.0.0");
        }

        // 2. SourceBuffer round-trip through SDK
        {
            gspl::GsplContext ctx;
            auto buf = gspl::SourceBuffer::from_string("test.gspl", "module test;\nentity E { rights PUBLIC; morphology {} }\n");
            auto diags = ctx.compile_source(std::move(buf));
            (void)diags;
        }

        // 3. ExpressionConfig defaults
        {
            gspl::ExpressionConfig cfg;
            check(cfg.deterministic_seed == false, "default seed should be false");
            check(cfg.deterministic_entropy == 42, "default entropy should be 42");
        }

        // 4. ProviderRegistry basic
        {
            auto& reg = gspl::ProviderRegistry::instance();
            reg.clear();
            reg.register_provider("test", std::make_unique<gspl::FakeTestProvider>("test"));
            auto provs = reg.available_providers();
            check(provs.size() == 1, "should have 1 provider");
            reg.clear();
        }

        // 5. Utf8 validation
        {
            auto result = gspl::validate_utf8("hello");
            check(result.valid, "ASCII should be valid UTF-8");
        }

        // 6. CacheConfig defaults
        {
            gspl::CacheConfig cfg;
            check(cfg.enabled == true, "cache should be enabled by default");
            check(cfg.max_bytes == 256ULL * 1024 * 1024, "default cache size");
        }

        // 7. Legacy detection
        {
            check(gspl::is_legacy_sprite_format("sprite Test\n"), "should detect legacy sprite");
            check(!gspl::is_legacy_sprite_format("module test;"), "should not detect GSPL module");
        }

        // 8. MigrateOptions defaults
        {
            gspl::MigrateOptions mopts;
            check(mopts.dry_run == false, "dry-run should be false by default");
            check(mopts.preserve_comments == true, "preserve_comments should be true");
            check(mopts.verify_output == true, "verify_output should be true");
        }

        std::cout << "ALL HEADER SELF-CONTAINMENT TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
