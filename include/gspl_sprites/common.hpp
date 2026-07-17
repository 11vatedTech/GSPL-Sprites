#pragma once

#include <string>
#include <vector>

namespace gspl::sprites {

struct Diagnostic { std::string code; std::string message; };
struct ValidationResult {
  std::vector<Diagnostic> diagnostics;
  [[nodiscard]] bool ok() const noexcept { return diagnostics.empty(); }
};

} // namespace gspl::sprites

