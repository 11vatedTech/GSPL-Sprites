#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl { struct CanonicalEntity; }

namespace gspl::studio {

struct BehaviorRuleEntry {
    std::string stimulus;
    std::string action;
    int priority = 5;
    std::string condition;
    bool active = true;
};

class BehaviorEditorModel {
public:
    explicit BehaviorEditorModel(gspl::CanonicalEntity& entity);
    void refresh();
    std::vector<BehaviorRuleEntry> const& rules() const { return rules_; }
    BehaviorRuleEntry const* rule(size_t index) const;
    bool add_rule(std::string_view stimulus, std::string_view action, int priority);
    bool remove_rule(size_t index);
    bool update_priority(size_t index, int priority);
    bool toggle_rule(size_t index);
    std::vector<BehaviorRuleEntry> sorted_by_priority() const;
    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }
private:
    gspl::CanonicalEntity& entity_;
    std::vector<BehaviorRuleEntry> rules_;
    ChangeCallback change_cb_;
};

} // namespace gspl::studio
