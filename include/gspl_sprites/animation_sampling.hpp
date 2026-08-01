#pragma once

#include "gspl_sprites/synthesis.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

namespace gspl::sprites {

[[nodiscard]] inline std::string canonical_double(double value) {
  if (std::isnan(value)) throw std::invalid_argument("canonical_double: NaN");
  if (std::isinf(value)) throw std::invalid_argument("canonical_double: infinite");
  if (value == 0.0) value = 0.0;
  char buf[32]{};
  auto r = std::to_chars(buf, buf + sizeof(buf), value,
      std::chars_format::general, std::numeric_limits<double>::max_digits10);
  if (r.ec != std::errc{}) throw std::invalid_argument("canonical_double: to_chars failed");
  return std::string(buf, r.ptr);
}

/* Neutral module: select_first_retained_sample_at_or_after implemented in animation_sampling.cpp.
   Selects minimum retained sample with source_tick >= authored_tick. */
[[nodiscard]] std::optional<std::reference_wrapper<const GeneratedFrameSample>>
select_first_retained_sample_at_or_after(
    std::span<const GeneratedFrameSample> samples,
    std::string_view clip_id,
    std::uint32_t authored_tick);

} // namespace gspl::sprites
