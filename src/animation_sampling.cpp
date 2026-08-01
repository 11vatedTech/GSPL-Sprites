#include "gspl_sprites/animation_sampling.hpp"

#include <tuple>

namespace gspl::sprites {

std::optional<std::reference_wrapper<const GeneratedFrameSample>>
select_first_retained_sample_at_or_after(
    std::span<const GeneratedFrameSample> samples,
    std::string_view clip_id,
    std::uint32_t authored_tick) {
  const GeneratedFrameSample* best = nullptr;
  for (auto const& s : samples) {
    if (s.clip_id == clip_id && s.source_tick >= authored_tick) {
      if (!best ||
          std::make_tuple(s.source_tick, s.frame_index, s.frame_id) <
          std::make_tuple(best->source_tick, best->frame_index, best->frame_id)) {
        best = &s;
      }
    }
  }
  if (best) return std::cref(*best);
  return std::nullopt;
}

} // namespace gspl::sprites
