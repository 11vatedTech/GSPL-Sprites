#include "gspl/modules.hpp"
#include "gspl/lexer.hpp"
#include <ranges>
#include <sstream>

namespace gspl {

std::string ModulePath::to_string() const {
    std::ostringstream ss;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) ss << ".";
        ss << segments[i];
    }
    return ss.str();
}

ModulePath ModulePath::from_string(std::string_view path) {
    ModulePath mp;
    std::size_t start = 0, end;
    while ((end = path.find('.', start)) != std::string_view::npos) {
        mp.segments.push_back(std::string(path.substr(start, end - start)));
        start = end + 1;
    }
    mp.segments.push_back(std::string(path.substr(start)));
    return mp;
}

ModulePath ModulePath::from_file_path(std::filesystem::path const& fs_path,
                                       std::vector<std::filesystem::path> const& roots) {
    auto norm = std::filesystem::weakly_canonical(fs_path);
    for (auto const& root : roots) {
        auto rn = std::filesystem::weakly_canonical(root);
        auto rel = std::filesystem::relative(norm, rn);
        if (!rel.empty() && rel.native()[0] != '.') {
            ModulePath mp;
            auto stem = rel.stem();
            for (auto const& seg : rel.parent_path()) {
                mp.segments.push_back(seg.string());
            }
            if (!stem.empty()) mp.segments.push_back(stem.string());
            return mp;
        }
    }
    ModulePath mp;
    auto stem = norm.stem();
    for (auto const& seg : norm.parent_path()) {
        mp.segments.push_back(seg.string());
    }
    if (!stem.empty()) mp.segments.push_back(stem.string());
    return mp;
}

ModuleResolver::ModuleResolver(SourceManager& sources) : sources_(sources) {}

DiagnosticResult ModuleResolver::register_module(std::string path, SourceId source_id) {
    if (path_to_index_.contains(path)) {
        Diagnostic d;
        d.code = DiagnosticCode::GSPL_MODULE_DUPLICATE;
        d.severity = DiagnosticSeverity::error;
        d.message = "Duplicate module: " + path;
        diags_.add(std::move(d));
        return diags_;
    }
    path_to_index_[path] = modules_.size();
    modules_.push_back({ModulePath::from_string(path), source_id, ""});
    return diags_;
}

DiagnosticResult ModuleResolver::resolve_imports() {
    std::vector<bool> visited(modules_.size(), false);
    std::vector<bool> in_stack(modules_.size(), false);
    std::vector<std::size_t> stack;
    for (std::size_t i = 0; i < modules_.size(); ++i) {
        if (!visited[i] && dfs_has_cycle(i, visited, in_stack, stack)) {
            cycle_detected_ = true;
            for (auto idx : stack) cycle_path_.push_back(modules_[idx].path);
            diags_.add_error(DiagnosticCode::GSPL_MODULE_IMPORT_CYCLE,
                             "Import cycle detected", {});
            break;
        }
    }
    return diags_;
}

bool ModuleResolver::dfs_has_cycle(std::size_t idx, std::vector<bool>& visited,
                                    std::vector<bool>& in_stack, std::vector<std::size_t>& stack) {
    visited[idx] = true;
    in_stack[idx] = true;
    stack.push_back(idx);
    auto const* buf = sources_.lookup(modules_[idx].source_id);
    if (buf) {
        Lexer lex(*buf);
        auto tokens = lex.tokenize();
        if (lex.has_error()) { stack.pop_back(); in_stack[idx] = false; return false; }
        for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i].kind == TokenKind::keyword_import) {
                ++i;
                std::string imp_path;
                while (i < tokens.size() &&
                       (tokens[i].kind == TokenKind::identifier || tokens[i].kind == TokenKind::dot)) {
                    imp_path += tokens[i].text;
                    ++i;
                }
                if (auto it = path_to_index_.find(imp_path); it != path_to_index_.end()) {
                    auto next = it->second;
                    if (!visited[next]) {
                        if (dfs_has_cycle(next, visited, in_stack, stack)) return true;
                    } else if (in_stack[next]) return true;
                }
            }
        }
    }
    stack.pop_back();
    in_stack[idx] = false;
    return false;
}

NameResolver::NameResolver(SourceManager const& sources) : sources_(sources) {
    push_scope(); // global scope
}

std::size_t NameResolver::push_scope(std::optional<std::size_t> parent) {
    scopes_.push_back(NameScope(parent));
    return scopes_.size() - 1;
}

void NameResolver::pop_scope() { scopes_.pop_back(); }

NameScope& NameResolver::current_scope() { return scopes_.back(); }

void NameResolver::declare(std::string name, AstKind kind, SourceSpan span) {
    auto& scope = current_scope();
    if (scope.symbols.contains(name)) {
        diags_.add_error(DiagnosticCode::GSPL_NAME_DUPLICATE,
                         "Duplicate name: " + name, span);
        return;
    }
    scope.symbols[name] = kind;
    scope.symbol_spans[name] = span;
}

void NameResolver::lookup(std::string name, SourceSpan span) {
    for (auto i = scopes_.size(); i-- > 0;) {
        if (scopes_[i].symbols.contains(name)) return;
    }
    diags_.add_error(DiagnosticCode::GSPL_NAME_UNKNOWN, "Unknown name: " + name, span);
}

void NameResolver::resolve_entity(EntityDecl const& entity) {
    declare(entity.name, AstKind::entity, entity.span);
    push_scope(scopes_.size() - 1);
    for (auto const& child : entity.body) {
        if (auto const* sub = dynamic_cast<FormDecl const*>(child.get())) resolve_form(*sub);
    }
    pop_scope();
}

void NameResolver::resolve_gene(GeneDecl const& gene) {
    declare(gene.name, AstKind::gene, gene.span);
    push_scope(scopes_.size() - 1);
    for (auto const& dep : gene.dependencies) {
        lookup(dep, gene.span);
    }
    pop_scope();
}

void NameResolver::resolve_form(FormDecl const& form) {
    declare(form.name, AstKind::form, form.span);
    if (form.extends) lookup(*form.extends, form.span);
}

DiagnosticResult NameResolver::resolve(ModuleDecl const& module) {
    push_scope();
    for (auto const& decl : module.declarations) {
        if (auto const* e = dynamic_cast<EntityDecl const*>(decl.get())) resolve_entity(*e);
        else if (auto const* g = dynamic_cast<GeneDecl const*>(decl.get())) resolve_gene(*g);
        else if (auto const* f = dynamic_cast<FormDecl const*>(decl.get())) resolve_form(*f);
    }
    pop_scope();
    return diags_;
}

} // namespace gspl
