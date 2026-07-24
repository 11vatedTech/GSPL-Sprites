#include "gspl/studio/morphology_editor_model.hpp"
#include "gspl/semantics.hpp"

namespace gspl::studio {

MorphologyEditorModel::MorphologyEditorModel(gspl::CanonicalEntity& entity) : entity_(entity) {}

void MorphologyEditorModel::refresh() {
    entries_.clear();
    for (auto const& [name, part] : entity_.morphology) {
        MorphologyPartEntry e;
        e.name = name;
        e.parent = part.parent;
        e.x = part.x; e.y = part.y; e.z = part.z;
        e.size_x = part.size_x; e.size_y = part.size_y; e.size_z = part.size_z;
        e.color = part.color;
        e.rotation_degrees = part.rotation_degrees;
        // Count children
        for (auto const& [n2, p2] : entity_.morphology) {
            if (p2.parent == name) e.child_count++;
        }
        entries_.push_back(std::move(e));
    }
}

MorphologyPartEntry const* MorphologyEditorModel::entry(size_t index) const {
    if (index >= entries_.size()) return nullptr;
    return &entries_[index];
}

MorphologyPartEntry const* MorphologyEditorModel::find_by_name(std::string_view name) const {
    for (auto const& e : entries_) {
        if (e.name == name) return &e;
    }
    return nullptr;
}

std::vector<MorphologyPartEntry const*> MorphologyEditorModel::children_of(std::string_view parent_name) const {
    std::vector<MorphologyPartEntry const*> result;
    for (auto const& e : entries_) {
        if (e.parent == parent_name) result.push_back(&e);
    }
    return result;
}

bool MorphologyEditorModel::add_part(std::string_view name, std::string_view parent,
                                      double x, double y, double z,
                                      double sx, double sy, double sz) {
    if (entity_.morphology.find(std::string(name)) != entity_.morphology.end())
        return false;
    gspl::CanonicalPart part;
    part.name = std::string(name);
    part.parent = std::string(parent);
    part.x = x; part.y = y; part.z = z;
    part.size_x = sx; part.size_y = sy; part.size_z = sz;
    entity_.morphology[std::string(name)] = part;
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

bool MorphologyEditorModel::remove_part(std::string_view name) {
    auto it = entity_.morphology.find(std::string(name));
    if (it == entity_.morphology.end()) return false;
    entity_.morphology.erase(it);
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

bool MorphologyEditorModel::update_position(std::string_view name, double x, double y, double z) {
    auto it = entity_.morphology.find(std::string(name));
    if (it == entity_.morphology.end()) return false;
    it->second.x = x; it->second.y = y; it->second.z = z;
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

bool MorphologyEditorModel::update_size(std::string_view name, double sx, double sy, double sz) {
    auto it = entity_.morphology.find(std::string(name));
    if (it == entity_.morphology.end()) return false;
    it->second.size_x = sx; it->second.size_y = sy; it->second.size_z = sz;
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

} // namespace gspl::studio
