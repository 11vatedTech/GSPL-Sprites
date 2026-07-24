#include "gspl/studio/combat_editor_model.hpp"
#include "gspl/semantics.hpp"

namespace gspl::studio {

CombatEditorModel::CombatEditorModel(gspl::CanonicalEntity& entity) : entity_(entity) {}

void CombatEditorModel::refresh() {
    abilities_.clear();
    for (auto const& a : entity_.abilities) {
        CombatAbilityEntry e;
        e.ability_id = a.id;
        e.effect = a.effect;
        e.cost = a.cost;
        e.cooldown_ticks = a.cooldown_ticks;
        e.active_ticks = a.active_ticks;
        e.speed = a.speed_mm_per_tick;
        e.collision_radius = a.collision_radius_mm;
        e.origin_socket = a.origin_socket;
        e.is_storm = false;
        abilities_.push_back(std::move(e));
    }
    for (auto const& a : entity_.storm_abilities) {
        CombatAbilityEntry e;
        e.ability_id = a.id;
        e.effect = a.effect;
        e.cost = a.cost;
        e.cooldown_ticks = a.cooldown_ticks;
        e.active_ticks = a.active_ticks;
        e.speed = a.speed_mm_per_tick;
        e.collision_radius = a.collision_radius_mm;
        e.origin_socket = a.origin_socket;
        e.is_storm = true;
        abilities_.push_back(std::move(e));
    }
}

CombatAbilityEntry const* CombatEditorModel::ability(size_t index) const {
    if (index >= abilities_.size()) return nullptr;
    return &abilities_[index];
}

CombatAbilityEntry const* CombatEditorModel::find_by_id(std::string_view id) const {
    for (auto const& a : abilities_) {
        if (a.ability_id == id) return &a;
    }
    return nullptr;
}

bool CombatEditorModel::add_ability(std::string_view id, std::string_view effect, std::uint32_t cost) {
    // Check duplicate
    for (auto const& a : abilities_) if (a.ability_id == id) return false;
    gspl::CanonicalAbility ab;
    ab.id = std::string(id);
    ab.effect = std::string(effect);
    ab.cost = cost;
    entity_.abilities.push_back(ab);
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

bool CombatEditorModel::remove_ability(std::string_view id) {
    auto remove_from = [&](std::vector<gspl::CanonicalAbility>& vec) -> bool {
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it->id == id) { vec.erase(it); return true; }
        }
        return false;
    };
    bool ok = remove_from(entity_.abilities) || remove_from(entity_.storm_abilities);
    if (ok) { refresh(); if (change_cb_) change_cb_(); }
    return ok;
}

bool CombatEditorModel::update_cooldown(std::string_view id, std::uint32_t ticks) {
    auto update = [&](std::vector<gspl::CanonicalAbility>& vec) -> bool {
        for (auto& a : vec) { if (a.id == id) { a.cooldown_ticks = ticks; return true; } }
        return false;
    };
    bool ok = update(entity_.abilities) || update(entity_.storm_abilities);
    if (ok) { refresh(); if (change_cb_) change_cb_(); }
    return ok;
}

std::vector<CombatAbilityEntry> CombatEditorModel::storm_abilities() const {
    std::vector<CombatAbilityEntry> result;
    for (auto const& a : abilities_) {
        if (a.is_storm) result.push_back(a);
    }
    return result;
}

} // namespace gspl::studio
