#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl { struct CanonicalEntity; struct CanonicalForm; }

namespace gspl::studio {

struct FormEditorEntry {
    std::string form_id;
    std::uint32_t resource_capacity = 100;
    double collision_scale = 1.0;
    double ability_envelope = 1.0;
    std::uint32_t max_health = 100;
    std::vector<std::string> transformation_ids;
};

class FormEditorModel {
public:
    explicit FormEditorModel(gspl::CanonicalEntity& entity);
    void refresh();
    std::vector<FormEditorEntry> const& entries() const { return entries_; }
    FormEditorEntry const* entry(size_t index) const;
    FormEditorEntry const* find_by_id(std::string_view id) const;
    bool add_form(std::string_view id);
    bool remove_form(std::string_view id);
    bool update_capacity(std::string_view id, std::uint32_t cap);
    bool update_max_health(std::string_view id, std::uint32_t hp);
    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }
private:
    gspl::CanonicalEntity& entity_;
    std::vector<FormEditorEntry> entries_;
    ChangeCallback change_cb_;
};

} // namespace gspl::studio
