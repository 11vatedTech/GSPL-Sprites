#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl { struct CanonicalEntity; struct CanonicalAnimationClip; }

namespace gspl::studio {

struct AnimationClipEntry {
    std::string name;
    bool loop = false;
    int track_count = 0;
    int keyframe_count = 0;
    std::vector<std::string> bone_names;
};

class AnimationEditorModel {
public:
    explicit AnimationEditorModel(gspl::CanonicalEntity& entity);
    void refresh();
    std::vector<AnimationClipEntry> const& clips() const { return clips_; }
    AnimationClipEntry const* clip(size_t index) const;
    AnimationClipEntry const* find_clip(std::string_view name) const;
    bool add_clip(std::string_view name, bool loop);
    bool remove_clip(std::string_view name);
    bool set_looping(std::string_view name, bool loop);
    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }
private:
    gspl::CanonicalEntity& entity_;
    std::vector<AnimationClipEntry> clips_;
    ChangeCallback change_cb_;
};

} // namespace gspl::studio
