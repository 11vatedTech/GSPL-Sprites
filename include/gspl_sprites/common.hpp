#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gspl::sprites {

/* Diagnostic severity: info (never fails), warning (quality, never fails),
 * error (identity/hard violation, fails closed). ValidationResult::ok()
 * returns false when ANY diagnostic carries severity == error. */
enum class DiagnosticSeverity : std::uint8_t { info = 0, warning = 1, error = 2 };

struct Diagnostic {
  std::string code;
  std::string message;
  DiagnosticSeverity severity{DiagnosticSeverity::error};
};

struct ValidationResult {
  std::vector<Diagnostic> diagnostics;
  [[nodiscard]] bool ok() const noexcept {
    for (const auto& d : diagnostics)
      if (d.severity == DiagnosticSeverity::error) return false;
    return true;
  }
  // Convenience: true when no error-severity diagnostics exist.
  [[nodiscard]] bool has_errors() const noexcept { return !ok(); }
};

} // namespace gspl::sprites

