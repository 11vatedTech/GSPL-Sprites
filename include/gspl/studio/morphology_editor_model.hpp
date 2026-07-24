#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl { struct CanonicalEntity; struct CanonicalPart; }

namespace gspl::studio {

struct MorphologyPartEntry {
    std::string name;
    std::string parent;
    double x{}, y{}, z{};
    double size_x{1}, size_y{1}, size_z{1};
    std::string color;
    double rotation_degrees{};
    int child_count = 0;
};

class MorphologyEditorModel {
public:
    explicit MorphologyEditorModel(gspl::CanonicalEntity& entity);
    void refresh();
    std::vector<MorphologyPartEntry> const& entries() const { return entries_; }
    MorphologyPartEntry const* entry(size_t index) const;
    MorphologyPartEntry const* find_by_name(std::string_view name) const;
    std::vector<MorphologyPartEntry const*> children_of(std::string_view parent_name) const;
    bool add_part(std::string_view name, std::string_view parent,
                  double x, double y, double z,
                  double sx, double sy, double sz);
    bool remove_part(std::string_view name);
    bool update_position(std::string_view name, double x, double y, double z);
    bool update_size(std::string_view name, double sx, double sy, double sz);
    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }
private:
    gspl::CanonicalEntity& entity_;
    std::vector<MorphologyPartEntry> entries_;
    ChangeCallback change_cb_;
};

} // namespace gspl::studio
