#include "gspl_sprites/rights.hpp"

namespace gspl::sprites {

std::string_view rights_class_to_string(RightsClass classification) noexcept {
  switch (classification) {
    case RightsClass::original_user_creation: return "ORIGINAL_USER_CREATION";
    case RightsClass::user_owned:            return "USER_OWNED_REFERENCE";
    case RightsClass::licensed:              return "LICENSED_REFERENCE";
    case RightsClass::public_domain:         return "PUBLIC_DOMAIN";
    case RightsClass::permissive:            return "PERMISSIVELY_LICENSED";
    case RightsClass::research_only:         return "RESEARCH_ONLY_REFERENCE";
    case RightsClass::restricted:            return "RESTRICTED_REFERENCE";
    case RightsClass::unknown:               return "UNKNOWN_RIGHTS";
    case RightsClass::prohibited:            return "PROHIBITED";
  }
  return "UNKNOWN_RIGHTS";
}

std::optional<RightsClass> rights_class_from_string(std::string_view text) noexcept {
  if (text == "ORIGINAL_USER_CREATION") return RightsClass::original_user_creation;
  if (text == "USER_OWNED_REFERENCE")  return RightsClass::user_owned;
  if (text == "LICENSED_REFERENCE")    return RightsClass::licensed;
  if (text == "PUBLIC_DOMAIN")         return RightsClass::public_domain;
  if (text == "PERMISSIVELY_LICENSED") return RightsClass::permissive;
  if (text == "RESEARCH_ONLY_REFERENCE") return RightsClass::research_only;
  if (text == "RESTRICTED_REFERENCE")  return RightsClass::restricted;
  if (text == "UNKNOWN_RIGHTS")        return RightsClass::unknown;
  if (text == "PROHIBITED")            return RightsClass::prohibited;
  return std::nullopt;
}

RightsClass lower_rights_class(std::string_view classification) noexcept {
  if (classification.find("ORIGINAL_USER_CREATION") != std::string_view::npos)
    return RightsClass::original_user_creation;
  if (classification.find("USER_OWNED") != std::string_view::npos)
    return RightsClass::user_owned;
  if (classification.find("LICENSED") != std::string_view::npos)
    return RightsClass::licensed;
  if (classification.find("PUBLIC_DOMAIN") != std::string_view::npos)
    return RightsClass::public_domain;
  if (classification.find("PERMISSIVELY_LICENSED") != std::string_view::npos ||
      classification.find("PERMISSIVE") != std::string_view::npos)
    return RightsClass::permissive;
  if (classification.find("RESEARCH_ONLY") != std::string_view::npos)
    return RightsClass::research_only;
  if (classification.find("RESTRICTED") != std::string_view::npos)
    return RightsClass::restricted;
  if (classification.find("PROHIBITED") != std::string_view::npos)
    return RightsClass::prohibited;
  return RightsClass::unknown;
}

} // namespace gspl::sprites
