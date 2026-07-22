#include "gspl/gspl.hpp"
#include "gspl/types.hpp"
#include "gspl/passes.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. Primitive type construction ----
        {
            check(gspl::GsplType::make_bool().base == gspl::BaseType::Bool, "Bool type");
            check(gspl::GsplType::make_int().base == gspl::BaseType::Int, "Int type");
            check(gspl::GsplType::make_uint().base == gspl::BaseType::UInt, "UInt type");
            check(gspl::GsplType::make_fixed().base == gspl::BaseType::Fixed, "Fixed type");
            check(gspl::GsplType::make_string().base == gspl::BaseType::String, "String type");
            check(gspl::GsplType::make_color().base == gspl::BaseType::Color, "Color type");
            check(gspl::GsplType::make_duration().base == gspl::BaseType::Duration, "Duration type");
            check(gspl::GsplType::make_distance().base == gspl::BaseType::Distance, "Distance type");
            check(gspl::GsplType::make_angle().base == gspl::BaseType::Angle, "Angle type");
        }

        // ---- 2. Composite type construction ----
        {
            auto range = gspl::GsplType::make_range(gspl::GsplType::make_int());
            check(range.base == gspl::BaseType::RangeType, "Range type");
            check(range.type_args.size() == 1, "Range should have 1 type arg");
            check(range.type_args[0].base == gspl::BaseType::Int, "Range element should be Int");

            auto list = gspl::GsplType::make_list(gspl::GsplType::make_string());
            check(list.base == gspl::BaseType::ListType, "List type");
            check(list.type_args[0].base == gspl::BaseType::String, "List element should be String");

            auto opt = gspl::GsplType::make_optional(gspl::GsplType::make_fixed());
            check(opt.base == gspl::BaseType::OptionalType, "Optional type");
        }

        // ---- 3. Type equality ----
        {
            auto a = gspl::GsplType::make_int();
            auto b = gspl::GsplType::make_int();
            check(a == b, "Same primitive types should be equal");
            check(!(a == gspl::GsplType::make_uint()), "Int != UInt");
        }

        // ---- 4. Type string representation ----
        {
            check(gspl::GsplType::make_bool().to_string() == "Bool", "Bool string");
            check(gspl::GsplType::make_int().to_string() == "Int", "Int string");
            check(gspl::GsplType::make_uint().to_string() == "UInt", "UInt string");
            check(gspl::GsplType::make_fixed().to_string() == "Fixed", "Fixed string");
            check(gspl::GsplType::make_string().to_string() == "String", "String string");
            check(gspl::GsplType::make_range(gspl::GsplType::make_int()).to_string() == "Range<Int>", "Range<Int> string");
            check(gspl::GsplType::make_list(gspl::GsplType::make_string()).to_string() == "List<String>", "List<String> string");
            check(gspl::GsplType::make_optional(gspl::GsplType::make_fixed()).to_string() == "Optional<Fixed>", "Optional<Fixed> string");
        }

        // ---- 5. Dimension compatibility ----
        {
            gspl::SourceManager sm;
            gspl::TypeChecker tc(sm);
            check(tc.is_compatible_dimension(gspl::GsplType::make_int(), gspl::GsplType::make_int()), "Int compatible with Int");
            check(tc.is_compatible_dimension(gspl::GsplType::make_int(), gspl::GsplType::make_fixed()), "Int compatible with Fixed");
            check(tc.is_compatible_dimension(gspl::GsplType::make_fixed(), gspl::GsplType::make_int()), "Fixed compatible with Int");
            check(tc.is_compatible_dimension(gspl::GsplType::make_duration(), gspl::GsplType::make_tick_count()), "Duration compatible with TickCount");
            check(tc.is_compatible_dimension(gspl::GsplType::make_percentage(), gspl::GsplType::make_ratio()), "Percentage compatible with Ratio");
            check(!tc.is_compatible_dimension(gspl::GsplType::make_bool(), gspl::GsplType::make_distance()), "Bool not compatible with Distance");
        }

        // ---- 6. Assignability ----
        {
            gspl::SourceManager sm;
            gspl::TypeChecker tc(sm);
            check(tc.is_assignable(gspl::GsplType::make_int(), gspl::GsplType::make_int()), "Int assignable to Int");
            check(tc.is_assignable(gspl::GsplType::make_fixed(), gspl::GsplType::make_int()), "Int assignable to Fixed");
            check(tc.is_assignable(gspl::GsplType::make_fixed(), gspl::GsplType::make_uint()), "UInt assignable to Fixed");
            check(tc.is_assignable(gspl::GsplType::make_int(), gspl::GsplType::make_uint()), "UInt assignable to Int");
            check(tc.is_assignable(gspl::GsplType::make_optional(gspl::GsplType::make_int()), gspl::GsplType::make_int()), "Int assignable to Optional<Int>");
            check(!tc.is_assignable(gspl::GsplType::make_bool(), gspl::GsplType::make_string()), "Bool not assignable to String");
        }

        // ---- 7. Type resolution from TypeRef ----
        {
            gspl::SourceManager sm;
            gspl::TypeChecker tc(sm);
            gspl::TypeRef tr("Int");
            auto resolved = tc.resolve_type(tr);
            check(resolved.base == gspl::BaseType::Int, "resolve_type(\"Int\") should resolve to Int");

            gspl::TypeRef tr2("Bool");
            resolved = tc.resolve_type(tr2);
            check(resolved.base == gspl::BaseType::Bool, "resolve_type(\"Bool\") should resolve to Bool");

            gspl::TypeRef tr3("CustomName");
            resolved = tc.resolve_type(tr3);
            check(resolved.base == gspl::BaseType::CustomType, "Unknown type should resolve to CustomType");
            check(resolved.custom_name == "CustomName", "Custom type name should be preserved");
        }

        // ---- 8. Full type-check pipeline ----
        {
            gspl::CompilationContext ctx;
            auto buf = gspl::SourceBuffer::from_string("typetest.gspl",
                "module typetest;\n"
                "entity T {\n"
                "  rights ORIGINAL_USER_CREATION PUBLIC;\n"
                "  morphology {}\n"
                "}\n");
            ctx.sources.register_buffer(std::move(buf));
            gspl::LexPhase lex; lex.execute(ctx);
            gspl::ParsePhase parse; parse.execute(ctx);
            ctx.diagnostics = {};
            gspl::NameResolvePhase name_res; name_res.execute(ctx);
            ctx.diagnostics = {};
            gspl::TypeCheckPhase type_check; type_check.execute(ctx);
            bool has_type_err = false;
            for (auto const& d : ctx.diagnostics.diagnostics) {
                if (d.severity >= gspl::DiagnosticSeverity::error) has_type_err = true;
            }
            check(!has_type_err, "Type check pipeline should succeed");
        }

        // ---- 9. Type serialization round-trip ----
        {
            auto t = gspl::GsplType::make_range(gspl::GsplType::make_optional(gspl::GsplType::make_fixed()));
            auto str = t.to_string();
            check(str == "Range<Optional<Fixed>>", "Nested type string should be 'Range<Optional<Fixed>>'");
        }

        std::cout << "ALL TYPE SYSTEM TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}
