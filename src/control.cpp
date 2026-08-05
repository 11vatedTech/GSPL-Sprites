#include "gspl_sprites/control.hpp"

#include <sstream>
#include <stdexcept>

namespace gspl::sprites {

std::string_view control_authority_name(ControlAuthority authority) noexcept {
  switch (authority) {
    case ControlAuthority::player:   return "player";
    case ControlAuthority::living:   return "living";
    case ControlAuthority::ai:       return "ai";
    case ControlAuthority::scripted: return "scripted";
    case ControlAuthority::network:  return "network";
    case ControlAuthority::cinematic: return "cinematic";
    case ControlAuthority::hybrid:   return "hybrid";
  }
  return "unknown";
}

ValidationResult validate_control_policy(const ControlPolicy& policy) {
  ValidationResult result;
  if (policy.policy_id.empty())
    result.diagnostics.push_back(Diagnostic{"control_policy", "policy_id is required"});
  if (policy.entity_instance_id.empty())
    result.diagnostics.push_back(Diagnostic{"control_policy", "entity_instance_id is required"});
  return result;
}

std::string canonicalize_control_policy(const ControlPolicy& policy) {
  std::ostringstream out;
  out << control_authority_name(policy.authority)
      << "|" << policy.policy_id
      << "|" << policy.entity_instance_id
      << "|" << policy.priority
      << "|" << (policy.deterministic ? "1" : "0");
  return out.str();
}

ControlPolicy
living_control_policy(const LivingRuntimeProgram& program,
                      std::string_view entity_instance_id) {
  ControlPolicy policy;
  policy.authority         = ControlAuthority::living;
  policy.policy_id         = program.id;
  policy.entity_instance_id = std::string(entity_instance_id);
  policy.priority          = 0;
  policy.deterministic     = true;
  return policy;
}

} // namespace gspl::sprites
