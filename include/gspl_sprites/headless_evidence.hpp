#pragma once
#include "gspl_sprites/core.hpp"
#include "gspl_sprites/combat.hpp"
#include "gspl_sprites/transformation.hpp"
#include "gspl_sprites/living_runtime.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace gspl::sprites {

enum class EvidenceEventKind { spawn, form_change, ability_use, damage_taken, status_applied, despawn };

struct EvidenceEvent {
  std::uint64_t tick{};
  EvidenceEventKind kind{};
  std::string form_before;
  std::string form_after;
  std::uint32_t health{};
  std::string status_id;
  std::string ability_name;
};

struct EvidenceTrace {
  std::string entity_id;
  std::uint64_t seed{};
  std::uint64_t total_ticks{};
  std::vector<EvidenceEvent> events;
};

[[nodiscard]] EvidenceTrace run_headless_evidence(const SpriteIr& ir);

[[nodiscard]] std::string write_trace_json(const EvidenceTrace& trace);

} // namespace gspl::sprites
