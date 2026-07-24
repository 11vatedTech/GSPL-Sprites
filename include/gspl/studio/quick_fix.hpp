#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::studio {

struct TextEdit {
    size_t start_line = 0;
    size_t start_col = 0;
    size_t end_line = 0;
    size_t end_col = 0;
    std::string replacement;
};

struct QuickFixAction {
    std::string label;
    std::string description;
    std::vector<TextEdit> edits;
    bool is_preferred = false;
};

struct QuickFixDiagnostic {
    int code = 0;
    std::string message;
    size_t line = 0;
    size_t column = 0;
    size_t length = 0;
};

using QuickFixGenerator = std::function<std::vector<QuickFixAction>(QuickFixDiagnostic const&)>;

class QuickFixProvider {
public:
    QuickFixProvider();

    void register_fixer(int diagnostic_code, QuickFixGenerator generator);

    std::vector<QuickFixAction> get_fixes(QuickFixDiagnostic const& diagnostic) const;

    void register_builtins();

    void clear();

    // Convenience: register a simple replace-range fixer
    void register_replace_fixer(int diagnostic_code, std::string label, std::string_view replacement);

private:
    struct FixerEntry {
        int code;
        QuickFixGenerator generator;
    };
    std::vector<FixerEntry> fixers_;
};

namespace builtin_fixes {

QuickFixAction fix_missing_semicolon(QuickFixDiagnostic const& diag);
QuickFixAction fix_unbalanced_brace(QuickFixDiagnostic const& diag);
QuickFixAction fix_unterminated_string(QuickFixDiagnostic const& diag);
QuickFixAction fix_type_mismatch_cast(QuickFixDiagnostic const& diag);
QuickFixAction fix_unknown_name_import(QuickFixDiagnostic const& diag);
QuickFixAction fix_duplicate_name_rename(QuickFixDiagnostic const& diag);

} // namespace builtin_fixes

} // namespace gspl::studio
