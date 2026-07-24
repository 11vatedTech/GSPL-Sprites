#include "gspl/studio/authority_model.hpp"
#include <algorithm>
#include <filesystem>

namespace gspl::studio {
namespace fs = std::filesystem;

struct AuthorityModel::Impl {
    std::string workspace_root_;
    std::vector<DirectoryAuthority> authorities_;

    bool is_subpath(const std::string& base, const std::string& candidate) const {
        auto bp = fs::weakly_canonical(fs::path(base));
        auto cp = fs::weakly_canonical(fs::path(candidate));
        auto rel = cp.lexically_relative(bp);
        return !rel.empty() && rel.native()[0] != '.';
    }
};

AuthorityModel::AuthorityModel() : impl_(std::make_unique<Impl>()) {}

AuthorityModel::AuthorityModel(std::string workspace_root)
    : impl_(std::make_unique<Impl>()) {
    impl_->workspace_root_ = std::move(workspace_root);
    reset_to_defaults();
}

AuthorityModel::~AuthorityModel() = default;

bool AuthorityModel::grant_access(const std::string& path, AuthorityLevel level, bool recursive) {
    auto can = fs::weakly_canonical(fs::path(path)).string();
    for (auto& a : impl_->authorities_) {
        if (a.path == can) {
            a.level = level;
            a.recursive = recursive;
            return true;
        }
    }
    impl_->authorities_.push_back({can, level, recursive});
    return true;
}

bool AuthorityModel::revoke_access(const std::string& path) {
    auto can = fs::weakly_canonical(fs::path(path)).string();
    auto it = std::remove_if(impl_->authorities_.begin(), impl_->authorities_.end(),
        [&](const DirectoryAuthority& a) { return a.path == can; });
    impl_->authorities_.erase(it, impl_->authorities_.end());
    return true;
}

AuthorityLevel AuthorityModel::check_access(const std::string& path) const {
    auto can = fs::weakly_canonical(fs::path(path)).string();
    AuthorityLevel best = AuthorityLevel::None;
    for (const auto& a : impl_->authorities_) {
        if (a.path == can || (a.recursive && impl_->is_subpath(a.path, can))) {
            if (static_cast<int>(a.level) > static_cast<int>(best)) {
                best = a.level;
            }
        }
    }
    return best;
}

auto AuthorityModel::authorities() const -> const std::vector<DirectoryAuthority>& {
    return impl_->authorities_;
}

void AuthorityModel::set_workspace_root(const std::string& root) {
    impl_->workspace_root_ = root;
    reset_to_defaults();
}

auto AuthorityModel::workspace_root() const -> const std::string& {
    return impl_->workspace_root_;
}

void AuthorityModel::reset_to_defaults() {
    impl_->authorities_.clear();
    if (!impl_->workspace_root_.empty()) {
        impl_->authorities_.push_back({impl_->workspace_root_, AuthorityLevel::Admin, true});
    }
}

} // namespace gspl::studio
