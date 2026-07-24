#include "gspl/studio/behavior_editor_model.hpp"
#include "gspl/semantics.hpp"
#include <algorithm>

namespace gspl::studio {

BehaviorEditorModel::BehaviorEditorModel(gspl::CanonicalEntity& entity) : entity_(entity) {}

void BehaviorEditorModel::refresh() {
    rules_.clear();
    // CanonicalEntity has no explicit behavior rules array.
    // We derive behavior entries from the runtime stats and animation intents.
    if (entity_.runtime) {
        BehaviorRuleEntry r;
        r.stimulus = "always";
        r.action = "idle";
        r.priority = 1;
        r.active = true;
        rules_.push_back(r);

        if (entity_.runtime->aggression > 30) {
            BehaviorRuleEntry a;
            a.stimulus = "threat_nearby";
            a.action = "aggressive";
            a.priority = 8;
            a.active = true;
            rules_.push_back(a);
        }
        if (entity_.runtime->curiosity > 30) {
            BehaviorRuleEntry c;
            c.stimulus = "object_nearby";
            c.action = "investigate";
            c.priority = 5;
            c.active = true;
            rules_.push_back(c);
        }
    }
}

BehaviorRuleEntry const* BehaviorEditorModel::rule(size_t index) const {
    if (index >= rules_.size()) return nullptr;
    return &rules_[index];
}

bool BehaviorEditorModel::add_rule(std::string_view stimulus, std::string_view action, int priority) {
    BehaviorRuleEntry r;
    r.stimulus = std::string(stimulus);
    r.action = std::string(action);
    r.priority = priority;
    r.active = true;
    rules_.push_back(std::move(r));
    if (change_cb_) change_cb_();
    return true;
}

bool BehaviorEditorModel::remove_rule(size_t index) {
    if (index >= rules_.size()) return false;
    rules_.erase(rules_.begin() + static_cast<std::ptrdiff_t>(index));
    if (change_cb_) change_cb_();
    return true;
}

bool BehaviorEditorModel::update_priority(size_t index, int priority) {
    if (index >= rules_.size()) return false;
    rules_[index].priority = priority;
    if (change_cb_) change_cb_();
    return true;
}

bool BehaviorEditorModel::toggle_rule(size_t index) {
    if (index >= rules_.size()) return false;
    rules_[index].active = !rules_[index].active;
    if (change_cb_) change_cb_();
    return true;
}

std::vector<BehaviorRuleEntry> BehaviorEditorModel::sorted_by_priority() const {
    auto result = rules_;
    std::sort(result.begin(), result.end(), [](auto const& a, auto const& b) {
        return a.priority > b.priority;
    });
    return result;
}

} // namespace gspl::studio
