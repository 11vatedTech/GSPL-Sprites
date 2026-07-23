#pragma once

#include "gspl/studio/target_adapter.hpp"
#include <string>

namespace gspl::studio::adapters {

class GodotAdapter {
public:
    GodotAdapter();
    TargetAdapterInfo info() const;
    bool export_entity(const std::string& entity_id,
                       const std::string& output_dir,
                       const std::string& profile) const;
    bool validate_export(const std::string& output_dir) const;
    static std::string required_engine_version();
};

} // namespace gspl::studio::adapters
