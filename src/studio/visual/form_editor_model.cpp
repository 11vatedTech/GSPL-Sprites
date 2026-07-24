#include "gspl/studio/form_editor_model.hpp"
#include "gspl/semantics.hpp"

namespace gspl::studio {

FormEditorModel::FormEditorModel(gspl::CanonicalEntity& entity) : entity_(entity) {}

void FormEditorModel::refresh() {
    entries_.clear();
    for (auto const& f : entity_.forms) {
        FormEditorEntry e;
        e.form_id = f.id;
        e.resource_capacity = f.resource_capacity;
        e.collision_scale = f.collision_scale;
        e.ability_envelope = f.ability_envelope;
        e.max_health = f.max_health;
        e.transformation_ids = f.transformation_ids;
        entries_.push_back(std::move(e));
    }
}

FormEditorEntry const* FormEditorModel::entry(size_t index) const {
    if (index >= entries_.size()) return nullptr;
    return &entries_[index];
}

FormEditorEntry const* FormEditorModel::find_by_id(std::string_view id) const {
    for (auto const& e : entries_) {
        if (e.form_id == id) return &e;
    }
    return nullptr;
}

bool FormEditorModel::add_form(std::string_view id) {
    for (auto const& f : entity_.forms) {
        if (f.id == id) return false;
    }
    gspl::CanonicalForm form;
    form.id = std::string(id);
    entity_.forms.push_back(form);
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

bool FormEditorModel::remove_form(std::string_view id) {
    for (auto it = entity_.forms.begin(); it != entity_.forms.end(); ++it) {
        if (it->id == id) {
            entity_.forms.erase(it);
            refresh();
            if (change_cb_) change_cb_();
            return true;
        }
    }
    return false;
}

bool FormEditorModel::update_capacity(std::string_view id, std::uint32_t cap) {
    for (auto& f : entity_.forms) {
        if (f.id == id) { f.resource_capacity = cap; refresh(); if (change_cb_) change_cb_(); return true; }
    }
    return false;
}

bool FormEditorModel::update_max_health(std::string_view id, std::uint32_t hp) {
    for (auto& f : entity_.forms) {
        if (f.id == id) { f.max_health = hp; refresh(); if (change_cb_) change_cb_(); return true; }
    }
    return false;
}

} // namespace gspl::studio
