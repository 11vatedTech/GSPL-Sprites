#include "gspl/types.hpp"
#include <sstream>

namespace gspl {

GsplType GsplType::make_bool() { return GsplType{BaseType::Bool, {}, {}}; }
GsplType GsplType::make_int() { return GsplType{BaseType::Int, {}, {}}; }
GsplType GsplType::make_uint() { return GsplType{BaseType::UInt, {}, {}}; }
GsplType GsplType::make_fixed() { return GsplType{BaseType::Fixed, {}, {}}; }
GsplType GsplType::make_string() { return GsplType{BaseType::String, {}, {}}; }
GsplType GsplType::make_color() { return GsplType{BaseType::Color, {}, {}}; }
GsplType GsplType::make_duration() { return GsplType{BaseType::Duration, {}, {}}; }
GsplType GsplType::make_distance() { return GsplType{BaseType::Distance, {}, {}}; }
GsplType GsplType::make_angle() { return GsplType{BaseType::Angle, {}, {}}; }
GsplType GsplType::make_percentage() { return GsplType{BaseType::Percentage, {}, {}}; }
GsplType GsplType::make_ratio() { return GsplType{BaseType::Ratio, {}, {}}; }
GsplType GsplType::make_tick_count() { return GsplType{BaseType::TickCount, {}, {}}; }
GsplType GsplType::make_vector2() { return GsplType{BaseType::Vector2, {}, {}}; }
GsplType GsplType::make_vector3() { return GsplType{BaseType::Vector3, {}, {}}; }
GsplType GsplType::make_range(GsplType element) { return GsplType{BaseType::RangeType, {}, {element}}; }
GsplType GsplType::make_list(GsplType element) { return GsplType{BaseType::ListType, {}, {element}}; }
GsplType GsplType::make_optional(GsplType inner) { return GsplType{BaseType::OptionalType, {}, {inner}}; }
GsplType GsplType::make_reference(std::string name) { return GsplType{BaseType::ReferenceType, std::move(name), {}}; }
GsplType GsplType::make_custom(std::string name) { return GsplType{BaseType::CustomType, std::move(name), {}}; }

std::string GsplType::to_string() const {
    switch (base) {
    case BaseType::Bool: return "Bool";
    case BaseType::Int: return "Int";
    case BaseType::UInt: return "UInt";
    case BaseType::Fixed: return "Fixed";
    case BaseType::String: return "String";
    case BaseType::Color: return "Color";
    case BaseType::Duration: return "Duration";
    case BaseType::Distance: return "Distance";
    case BaseType::Angle: return "Angle";
    case BaseType::Percentage: return "Percentage";
    case BaseType::Ratio: return "Ratio";
    case BaseType::Vector2: return "Vector2";
    case BaseType::Vector3: return "Vector3";
    case BaseType::RangeType: return "Range<" + (type_args.empty() ? "?" : type_args[0].to_string()) + ">";
    case BaseType::ListType: return "List<" + (type_args.empty() ? "?" : type_args[0].to_string()) + ">";
    case BaseType::OptionalType: return "Optional<" + (type_args.empty() ? "?" : type_args[0].to_string()) + ">";
    case BaseType::CustomType: return custom_name;
    case BaseType::Identifier: return custom_name.empty() ? "Identifier" : custom_name;
    default: return "Error";
    }
}

TypeChecker::TypeChecker(SourceManager const& sources) : sources_(sources) {}

bool TypeChecker::is_compatible_dimension(GsplType const& a, GsplType const& b) const {
    if (a == b) return true;
    if (a.base == BaseType::Int && b.base == BaseType::Fixed) return true;
    if (a.base == BaseType::UInt && b.base == BaseType::Fixed) return true;
    if (a.base == BaseType::Fixed && b.base == BaseType::Int) return true;
    if (a.base == BaseType::Fixed && b.base == BaseType::UInt) return true;
    if (a.base == BaseType::Distance && b.base == BaseType::Int) return true;
    if (a.base == BaseType::Int && b.base == BaseType::Distance) return true;
    if (a.base == BaseType::Duration && b.base == BaseType::TickCount) return true;
    if (a.base == BaseType::Percentage && b.base == BaseType::Ratio) return true;
    return false;
}

bool TypeChecker::is_assignable(GsplType const& target, GsplType const& source) const {
    if (target == source) return true;
    if (target.base == BaseType::Int && source.base == BaseType::UInt) return true;
    if (target.base == BaseType::Fixed && (source.base == BaseType::Int || source.base == BaseType::UInt)) return true;
    if (target.base == BaseType::OptionalType && source.base == target.type_args[0].base) return true;
    return is_compatible_dimension(target, source);
}

GsplType TypeChecker::resolve_type(TypeRef const& type_ref) const {
    if (type_ref.name == "Bool") return GsplType::make_bool();
    if (type_ref.name == "Int") return GsplType::make_int();
    if (type_ref.name == "UInt") return GsplType::make_uint();
    if (type_ref.name == "Fixed") return GsplType::make_fixed();
    if (type_ref.name == "String") return GsplType::make_string();
    if (type_ref.name == "Color") return GsplType::make_color();
    if (type_ref.name == "Duration") return GsplType::make_duration();
    if (type_ref.name == "Vector2") return GsplType::make_vector2();
    if (type_ref.name == "Vector3") return GsplType::make_vector3();
    if (type_ref.name == "Range") {
        return type_ref.type_args.empty() ? GsplType::make_range(GsplType::make_int())
                                          : GsplType::make_range(resolve_type(
                                                static_cast<TypeRef const&>(*type_ref.type_args[0])));
    }
    return GsplType::make_custom(type_ref.name);
}

GsplType TypeChecker::check_binary_op(TokenKind op, GsplType const& left, GsplType const& right, SourceSpan span) {
    if (op == TokenKind::plus || op == TokenKind::minus ||
        op == TokenKind::star || op == TokenKind::slash) {
        if (!is_compatible_dimension(left, right)) {
            diags_.add_error(DiagnosticCode::GSPL_TYPE_INCOMPATIBLE_UNITS,
                             "Incompatible types for arithmetic: " + left.to_string() + " and " + right.to_string(),
                             span);
            return GsplType::make_int();
        }
        if (left.base == BaseType::Fixed || right.base == BaseType::Fixed) return GsplType::make_fixed();
        return left;
    }
    if (op == TokenKind::equal_equal || op == TokenKind::bang_equal) return GsplType::make_bool();
    if (op == TokenKind::less || op == TokenKind::less_equal || op == TokenKind::greater || op == TokenKind::greater_equal) {
        return GsplType::make_bool();
    }
    return GsplType::make_int();
}

GsplType TypeChecker::check_expression(AstNode const& expr) {
    if (auto const* lit = dynamic_cast<LiteralNode const*>(&expr)) {
        switch (lit->literal_kind) {
        case TokenKind::integer_literal: return GsplType::make_int();
        case TokenKind::unsigned_literal: return GsplType::make_uint();
        case TokenKind::fixed_literal: return GsplType::make_fixed();
        case TokenKind::string_literal: return GsplType::make_string();
        case TokenKind::boolean_literal: return GsplType::make_bool();
        default: return GsplType::make_int();
        }
    }
    return GsplType::make_int();
}

DiagnosticResult TypeChecker::check_types(ModuleDecl const&) {
    return diags_;
}

} // namespace gspl
