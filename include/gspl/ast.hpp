#pragma once
#include "gspl/source.hpp"
#include "gspl/token.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gspl {

enum class AstKind : std::uint8_t {
    module, import, entity, gene, trait, form, transformation, morphology,
    part, joint, socket, material, palette, resource, animation, behavior,
    ability, collision, rights, provenance, expression, type_ref, attribute,
    block, if_expr, let_binding, const_decl, identifier_ref, literal,
    binary_op, unary_op, call, member_access, list_literal, map_literal
};

struct AstNode {
    AstKind kind{};
    SourceSpan span;
    virtual ~AstNode() = default;
};

struct IdentifierRef : AstNode {
    std::string name;
    explicit IdentifierRef(std::string n) : name(std::move(n)) { kind = AstKind::identifier_ref; }
};

struct TypeRef : AstNode {
    std::string name;
    std::vector<std::unique_ptr<AstNode>> type_args;
    explicit TypeRef(std::string n) : name(std::move(n)) { kind = AstKind::type_ref; }
};

struct LiteralNode : AstNode {
    TokenKind literal_kind{};
    std::string value;
    LiteralNode(TokenKind tk, std::string v) : literal_kind(tk), value(std::move(v)) { kind = AstKind::literal; }
};

struct AttributeNode : AstNode {
    std::string key;
    std::unique_ptr<AstNode> value;
    AttributeNode(std::string k, std::unique_ptr<AstNode> v) : key(std::move(k)), value(std::move(v)) { kind = AstKind::attribute; }
};

struct ImportDecl : AstNode {
    std::string module_path;
    std::optional<std::string> alias;
    explicit ImportDecl(std::string path) : module_path(std::move(path)) { kind = AstKind::import; }
};

struct EntityDecl;
struct GeneDecl;
struct FormDecl;
struct TransformationDecl;
struct PartDecl;
struct MorphologyDecl;
struct ResourceDecl;
struct AbilityDecl;
struct AnimationDecl;
struct BehaviorDecl;
struct CollisionDecl;
struct RightsDecl;

struct ModuleDecl : AstNode {
    std::string name;
    std::vector<std::unique_ptr<ImportDecl>> imports;
    std::vector<std::unique_ptr<AstNode>> declarations;
    ModuleDecl() { kind = AstKind::module; }
};

struct EntityDecl : AstNode {
    std::string name;
    std::vector<std::unique_ptr<AstNode>> body;
    explicit EntityDecl(std::string n) : name(std::move(n)) { kind = AstKind::entity; }
};

struct GeneDecl : AstNode {
    std::string name;
    std::vector<std::string> dependencies;
    std::vector<std::unique_ptr<AstNode>> body;
    GeneDecl(std::string n, std::vector<std::string> deps)
        : name(std::move(n)), dependencies(std::move(deps)) { kind = AstKind::gene; }
};

struct FormDecl : AstNode {
    std::string name;
    std::optional<std::string> extends;
    std::vector<std::unique_ptr<AstNode>> body;
    FormDecl(std::string n, std::optional<std::string> ext)
        : name(std::move(n)), extends(std::move(ext)) { kind = AstKind::form; }
};

struct TransformationDecl : AstNode {
    std::string name;
    std::string from_form;
    std::string to_form;
    std::unique_ptr<AstNode> duration;
    std::unique_ptr<AstNode> resource_cost;
    TransformationDecl(std::string n, std::string from, std::string to)
        : name(std::move(n)), from_form(std::move(from)), to_form(std::move(to)) { kind = AstKind::transformation; }
};

struct PartDecl : AstNode {
    std::string name;
    std::vector<std::unique_ptr<AstNode>> attributes;
    explicit PartDecl(std::string n) : name(std::move(n)) { kind = AstKind::part; }
};

struct MorphologyDecl : AstNode {
    std::vector<std::unique_ptr<AstNode>> parts;
    MorphologyDecl() { kind = AstKind::morphology; }
};

struct ResourceDecl : AstNode {
    std::string name;
    std::string resource_type;
    std::vector<std::unique_ptr<AstNode>> body;
    ResourceDecl(std::string n, std::string t) : name(std::move(n)), resource_type(std::move(t)) { kind = AstKind::resource; }
};

struct AbilityDecl : AstNode {
    std::string name;
    std::vector<std::unique_ptr<AstNode>> body;
    explicit AbilityDecl(std::string n) : name(std::move(n)) { kind = AstKind::ability; }
};

struct RightsDecl : AstNode {
    std::string classification;
    std::string code;
    explicit RightsDecl(std::string c) : classification(std::move(c)) { kind = AstKind::rights; }
};

struct GenericBlock : AstNode {
    std::string block_type;
    std::string name;
    std::vector<std::string> secondary_names;
    std::vector<std::unique_ptr<AstNode>> attributes;
    GenericBlock(std::string t) : block_type(std::move(t)) { kind = AstKind::block; }
};

struct BlockNode : AstNode {
    std::vector<std::unique_ptr<AstNode>> statements;
    BlockNode() { kind = AstKind::block; }
};

} // namespace gspl
