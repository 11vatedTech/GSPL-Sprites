#pragma once

#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace gspl::sprites {

[[nodiscard]] std::optional<int>
run_authoring_cli(std::span<const std::string_view> arguments,
                  std::ostream &output, std::ostream &error);

} // namespace gspl::sprites
