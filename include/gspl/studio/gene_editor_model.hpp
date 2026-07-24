#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl {
struct CanonicalEntity;
struct GeneInstance;
enum class GeneKind : std::uint32_t;
}

namespace gspl::studio {

struct GeneEditorEntry {
    std::string type_id;
    int kind = 0;
    std::string source_module;
    bool is_override = false;
    bool selected = true;
    bool has_conflict = false;
    std::string conflict_description;
};

class GeneEditorModel {
public:
    explicit GeneEditorModel(gspl::CanonicalEntity& entity);
    void refresh();
    std::vector<GeneEditorEntry> const& entries() const { return entries_; }
    GeneEditorEntry const* entry(size_t index) const;
    GeneEditorEntry const* find_by_type(std::string_view type_id) const;
    bool toggle_selection(size_t index);
    bool resolve_conflict(size_t index);
    std::vector<GeneEditorEntry> conflicts() const;
    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }
private:
    gspl::CanonicalEntity& entity_;
    std::vector<GeneEditorEntry> entries_;
    ChangeCallback change_cb_;
};

} // namespace gspl::studio
