#pragma once
#include "gspl/source.hpp"
#include "gspl/token.hpp"
#include "gspl/ast.hpp"
#include "gspl/diagnostics.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gspl {

struct ModulePath {
    std::vector<std::string> segments;
    std::string to_string() const;
    static ModulePath from_string(std::string_view path);
    static ModulePath from_file_path(std::filesystem::path const& fs_path,
                                     std::vector<std::filesystem::path> const& roots);
    auto operator<=>(const ModulePath&) const = default;
};

struct ModuleIdentity {
    ModulePath path;
    SourceId source_id{};
    std::string content_hash;
};

class ModuleResolver {
public:
    explicit ModuleResolver(SourceManager& sources);
    DiagnosticResult register_module(std::string path, SourceId source_id);
    DiagnosticResult resolve_imports();
    std::vector<ModuleIdentity> const& modules() const { return modules_; }
    bool has_cycle() const { return cycle_detected_; }
    std::vector<ModulePath> cycle_path() const { return cycle_path_; }
private:
    SourceManager& sources_;
    DiagnosticResult diags_;
    std::vector<ModuleIdentity> modules_;
    std::unordered_map<std::string, std::size_t> path_to_index_;
    bool cycle_detected_{};
    std::vector<ModulePath> cycle_path_;
    bool dfs_has_cycle(std::size_t idx, std::vector<bool>& visited,
                       std::vector<bool>& in_stack, std::vector<std::size_t>& stack);
};

struct NameScope {
    std::optional<std::size_t> parent;
    std::unordered_map<std::string, AstKind> symbols;
    std::unordered_map<std::string, SourceSpan> symbol_spans;
    NameScope() = default;
    NameScope(std::optional<std::size_t> p) : parent(p) {}
};

class NameResolver {
public:
    explicit NameResolver(SourceManager const& sources);
    DiagnosticResult resolve(ModuleDecl const& module);
private:
    SourceManager const& sources_;
    DiagnosticResult diags_;
    std::vector<NameScope> scopes_;
    std::size_t push_scope(std::optional<std::size_t> parent = std::nullopt);
    void pop_scope();
    NameScope& current_scope();
    void declare(std::string name, AstKind kind, SourceSpan span);
    void lookup(std::string name, SourceSpan span);
    void resolve_entity(EntityDecl const& entity);
    void resolve_gene(GeneDecl const& gene);
    void resolve_form(FormDecl const& form);
};

} // namespace gspl
