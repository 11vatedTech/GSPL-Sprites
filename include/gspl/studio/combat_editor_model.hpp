#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl { struct CanonicalEntity; struct CanonicalAbility; }

namespace gspl::studio {

struct CombatAbilityEntry {
    std::string ability_id;
    std::string effect;
    std::uint32_t cost = 0;
    std::uint32_t cooldown_ticks = 0;
    std::uint32_t active_ticks = 0;
    double speed = 0;
    double collision_radius = 0;
    std::string origin_socket;
    bool is_storm = false;
};

class CombatEditorModel {
public:
    explicit CombatEditorModel(gspl::CanonicalEntity& entity);
    void refresh();
    std::vector<CombatAbilityEntry> const& abilities() const { return abilities_; }
    CombatAbilityEntry const* ability(size_t index) const;
    CombatAbilityEntry const* find_by_id(std::string_view id) const;
    bool add_ability(std::string_view id, std::string_view effect, std::uint32_t cost);
    bool remove_ability(std::string_view id);
    bool update_cooldown(std::string_view id, std::uint32_t ticks);
    std::vector<CombatAbilityEntry> storm_abilities() const;
    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }
private:
    gspl::CanonicalEntity& entity_;
    std::vector<CombatAbilityEntry> abilities_;
    ChangeCallback change_cb_;
};

} // namespace gspl::studio
