#include "gspl/studio/gene_editor_model.hpp"
#include "gspl/semantics.hpp"
#include "gspl/genes.hpp"

namespace gspl::studio {

GeneEditorModel::GeneEditorModel(gspl::CanonicalEntity& entity) : entity_(entity) {}

void GeneEditorModel::refresh() {
    entries_.clear();
    for (auto const& gi : entity_.genes) {
        GeneEditorEntry e;
        e.type_id = gi.descriptor.type_id;
        e.kind = static_cast<int>(gi.descriptor.kind);
        e.source_module = gi.source_module;
        e.is_override = gi.is_override;
        e.selected = true;
        entries_.push_back(std::move(e));
    }
}

GeneEditorEntry const* GeneEditorModel::entry(size_t index) const {
    if (index >= entries_.size()) return nullptr;
    return &entries_[index];
}

GeneEditorEntry const* GeneEditorModel::find_by_type(std::string_view type_id) const {
    for (auto const& e : entries_) {
        if (e.type_id == type_id) return &e;
    }
    return nullptr;
}

bool GeneEditorModel::toggle_selection(size_t index) {
    if (index >= entries_.size()) return false;
    entries_[index].selected = !entries_[index].selected;
    if (change_cb_) change_cb_();
    return true;
}

bool GeneEditorModel::resolve_conflict(size_t index) {
    if (index >= entries_.size()) return false;
    if (!entries_[index].has_conflict) return false;
    entries_[index].has_conflict = false;
    entries_[index].conflict_description.clear();
    if (change_cb_) change_cb_();
    return true;
}

std::vector<GeneEditorEntry> GeneEditorModel::conflicts() const {
    std::vector<GeneEditorEntry> result;
    for (auto const& e : entries_) {
        if (e.has_conflict) result.push_back(e);
    }
    return result;
}

} // namespace gspl::studio
