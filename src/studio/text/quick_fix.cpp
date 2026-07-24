#include "gspl/studio/quick_fix.hpp"
#include <algorithm>

namespace gspl::studio {

QuickFixProvider::QuickFixProvider() {
    register_builtins();
}

void QuickFixProvider::register_fixer(int code, QuickFixGenerator gen) {
    fixers_.push_back({code, std::move(gen)});
}

std::vector<QuickFixAction> QuickFixProvider::get_fixes(QuickFixDiagnostic const& diag) const {
    std::vector<QuickFixAction> results;
    for (auto const& entry : fixers_) {
        if (entry.code == diag.code) {
            auto fixes = entry.generator(diag);
            results.insert(results.end(), fixes.begin(), fixes.end());
        }
    }
    return results;
}

void QuickFixProvider::register_builtins() {
    register_fixer(2002, [](QuickFixDiagnostic const& d) -> std::vector<QuickFixAction> {
        return {builtin_fixes::fix_missing_semicolon(d)};
    });
    register_fixer(2003, [](QuickFixDiagnostic const& d) -> std::vector<QuickFixAction> {
        return {builtin_fixes::fix_unbalanced_brace(d)};
    });
    register_fixer(1002, [](QuickFixDiagnostic const& d) -> std::vector<QuickFixAction> {
        return {builtin_fixes::fix_unterminated_string(d)};
    });
    register_fixer(5001, [](QuickFixDiagnostic const& d) -> std::vector<QuickFixAction> {
        return {builtin_fixes::fix_type_mismatch_cast(d)};
    });
    register_fixer(4002, [](QuickFixDiagnostic const& d) -> std::vector<QuickFixAction> {
        return {builtin_fixes::fix_unknown_name_import(d)};
    });
    register_fixer(4001, [](QuickFixDiagnostic const& d) -> std::vector<QuickFixAction> {
        return {builtin_fixes::fix_duplicate_name_rename(d)};
    });
}

void QuickFixProvider::clear() {
    fixers_.clear();
}

void QuickFixProvider::register_replace_fixer(int code, std::string label, std::string_view replacement) {
    register_fixer(code, [label = std::move(label), repl = std::string(replacement)](QuickFixDiagnostic const& d) {
        QuickFixAction action;
        action.label = label;
        action.is_preferred = true;
        TextEdit edit;
        edit.start_line = d.line;
        edit.start_col = d.column;
        edit.end_line = d.line;
        edit.end_col = d.column + d.length;
        edit.replacement = repl;
        action.edits.push_back(std::move(edit));
        return std::vector<QuickFixAction>{action};
    });
}

namespace builtin_fixes {

QuickFixAction fix_missing_semicolon(QuickFixDiagnostic const& diag) {
    QuickFixAction action;
    action.label = "Insert ';'";
    action.description = "Add missing semicolon at the end of the statement";
    action.is_preferred = true;
    TextEdit edit;
    edit.start_line = diag.line;
    edit.start_col = diag.column + diag.length;
    edit.end_line = diag.line;
    edit.end_col = diag.column + diag.length;
    edit.replacement = ";";
    action.edits.push_back(std::move(edit));
    return action;
}

QuickFixAction fix_unbalanced_brace(QuickFixDiagnostic const& diag) {
    QuickFixAction action;
    action.label = "Insert '}'";
    action.description = "Add closing brace to balance block";
    action.is_preferred = true;
    TextEdit edit;
    edit.start_line = diag.line;
    edit.start_col = diag.column + diag.length;
    edit.end_line = diag.line;
    edit.end_col = diag.column + diag.length;
    edit.replacement = "\n}";
    action.edits.push_back(std::move(edit));
    return action;
}

QuickFixAction fix_unterminated_string(QuickFixDiagnostic const& diag) {
    QuickFixAction action;
    action.label = "Add closing quote";
    action.description = "Add missing closing double-quote to string literal";
    action.is_preferred = true;
    TextEdit edit;
    edit.start_line = diag.line;
    edit.start_col = diag.column + diag.length;
    edit.end_line = diag.line;
    edit.end_col = diag.column + diag.length;
    edit.replacement = "\"";
    action.edits.push_back(std::move(edit));
    return action;
}

QuickFixAction fix_type_mismatch_cast(QuickFixDiagnostic const& diag) {
    QuickFixAction action;
    action.label = "Add explicit cast";
    action.description = "Insert a type cast to resolve the mismatch";
    action.is_preferred = false;
    TextEdit edit;
    edit.start_line = diag.line;
    edit.start_col = diag.column;
    edit.end_line = diag.line;
    edit.end_col = diag.column;
    edit.replacement = "as<type>(";
    action.edits.push_back(std::move(edit));
    TextEdit close;
    close.start_line = diag.line;
    close.start_col = diag.column + diag.length;
    close.end_line = diag.line;
    close.end_col = diag.column + diag.length;
    close.replacement = ")";
    action.edits.push_back(std::move(close));
    return action;
}

QuickFixAction fix_unknown_name_import(QuickFixDiagnostic const& /*diag*/) {
    QuickFixAction action;
    action.label = "Add import declaration";
    action.description = "Add an import for the unknown name at the top of the file";
    action.is_preferred = false;
    TextEdit edit;
    edit.start_line = 0;
    edit.start_col = 0;
    edit.end_line = 0;
    edit.end_col = 0;
    edit.replacement = "import unknown;\n";
    action.edits.push_back(std::move(edit));
    return action;
}

QuickFixAction fix_duplicate_name_rename(QuickFixDiagnostic const& diag) {
    QuickFixAction action;
    action.label = "Rename to unique identifier";
    action.description = "Append a numeric suffix to make the name unique";
    action.is_preferred = false;
    TextEdit edit;
    edit.start_line = diag.line;
    edit.start_col = diag.column;
    edit.end_line = diag.line;
    edit.end_col = diag.column + diag.length;
    edit.replacement = "_2";
    action.edits.push_back(std::move(edit));
    return action;
}

} // namespace builtin_fixes

} // namespace gspl::studio
