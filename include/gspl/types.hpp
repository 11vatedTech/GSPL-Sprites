#pragma once
#include "gspl/source.hpp"
#include "gspl/token.hpp"
#include "gspl/ast.hpp"
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <string>
#include <variant>

namespace gspl {

enum class BaseType : std::uint8_t {
    Bool, Int, UInt, Fixed, String, Identifier, EntityId, ModuleId,
    FormId, TransformationId, PartId, AbilityId, ResourceId, MaterialId,
    Color, Vector2, Vector3, Duration, TickCount, Distance, PixelDistance,
    WorldDistance, Angle, Ratio, Percentage, RangeType, ListType, MapType,
    OptionalType, ReferenceType, CustomType, ErrorType
};

struct GsplType {
    BaseType base{BaseType::ErrorType};
    std::string custom_name;
    std::vector<GsplType> type_args;
    bool operator==(const GsplType&) const = default;
    std::string to_string() const;
    static GsplType make_bool();
    static GsplType make_int();
    static GsplType make_uint();
    static GsplType make_fixed();
    static GsplType make_string();
    static GsplType make_color();
    static GsplType make_duration();
    static GsplType make_distance();
    static GsplType make_angle();
    static GsplType make_percentage();
    static GsplType make_ratio();
    static GsplType make_tick_count();
    static GsplType make_vector2();
    static GsplType make_vector3();
    static GsplType make_range(GsplType element);
    static GsplType make_list(GsplType element);
    static GsplType make_optional(GsplType inner);
    static GsplType make_reference(std::string name);
    static GsplType make_custom(std::string name);
};

class TypeChecker {
public:
    explicit TypeChecker(SourceManager const& sources);
    DiagnosticResult check_types(ModuleDecl const& module);
    GsplType resolve_type(TypeRef const& type_ref) const;
    bool is_assignable(GsplType const& target, GsplType const& source) const;
    bool is_compatible_dimension(GsplType const& a, GsplType const& b) const;
private:
    SourceManager const& sources_;
    DiagnosticResult diags_;
    GsplType check_expression(AstNode const& expr);
    GsplType check_binary_op(TokenKind op, GsplType const& left, GsplType const& right, SourceSpan span);
};

} // namespace gspl
