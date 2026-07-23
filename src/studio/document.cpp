#include "gspl/studio/document.hpp"
#include <fstream>
#include <sstream>
#include <utility>

namespace gspl::studio {

Document::Document(DocumentKind kind, std::string name)
    : kind_(kind)
    , name_(std::move(name))
{
}

auto Document::kind() const -> DocumentKind {
    return kind_;
}

auto Document::name() const -> const std::string& {
    return name_;
}

void Document::set_name(std::string name) {
    name_ = std::move(name);
    notify_change(DocumentChange::Content);
}

auto Document::is_dirty() const -> bool {
    return dirty_;
}

void Document::set_dirty(bool dirty) {
    if (dirty_ != dirty) {
        dirty_ = dirty;
        notify_change(DocumentChange::Dirty);
    }
}

auto Document::file_path() const -> const std::string& {
    return file_path_;
}

void Document::set_file_path(std::string path) {
    file_path_ = std::move(path);
}

auto Document::load() -> bool {
    if (file_path_.empty()) {
        return false;
    }
    std::ifstream file(file_path_);
    if (!file) {
        return false;
    }
    // Subclasses override for content-specific parsing.
    // Base class simply validates the file is readable.
    dirty_ = false;
    notify_change(DocumentChange::Content);
    return true;
}

auto Document::save() -> bool {
    if (file_path_.empty()) {
        return false;
    }
    // Subclasses override to write content-specific data.
    // Base class opens the file to verify writability.
    std::ofstream file(file_path_, std::ios::app);
    if (!file) {
        return false;
    }
    dirty_ = false;
    notify_change(DocumentChange::Saved);
    return true;
}

void Document::set_change_callback(ChangeCallback cb) {
    change_callback_ = std::move(cb);
}

void Document::notify_change(DocumentChange change) {
    if (change_callback_) {
        change_callback_(change);
    }
}

} // namespace gspl::studio
