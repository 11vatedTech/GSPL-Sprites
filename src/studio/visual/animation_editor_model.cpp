#include "gspl/studio/animation_editor_model.hpp"
#include "gspl/semantics.hpp"

namespace gspl::studio {

AnimationEditorModel::AnimationEditorModel(gspl::CanonicalEntity& entity) : entity_(entity) {}

void AnimationEditorModel::refresh() {
    clips_.clear();
    for (auto const& c : entity_.clips) {
        AnimationClipEntry e;
        e.name = c.name;
        e.loop = c.loop;
        e.track_count = static_cast<int>(c.tracks.size());
        e.keyframe_count = 0;
        for (auto const& t : c.tracks) {
            e.keyframe_count += static_cast<int>(t.keys.size());
            e.bone_names.push_back(t.bone);
        }
        clips_.push_back(std::move(e));
    }
}

AnimationClipEntry const* AnimationEditorModel::clip(size_t index) const {
    if (index >= clips_.size()) return nullptr;
    return &clips_[index];
}

AnimationClipEntry const* AnimationEditorModel::find_clip(std::string_view name) const {
    for (auto const& c : clips_) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

bool AnimationEditorModel::add_clip(std::string_view name, bool loop) {
    for (auto const& c : entity_.clips) {
        if (c.name == name) return false;
    }
    gspl::CanonicalAnimationClip clip;
    clip.name = std::string(name);
    clip.loop = loop;
    entity_.clips.push_back(clip);
    refresh();
    if (change_cb_) change_cb_();
    return true;
}

bool AnimationEditorModel::remove_clip(std::string_view name) {
    for (auto it = entity_.clips.begin(); it != entity_.clips.end(); ++it) {
        if (it->name == name) {
            entity_.clips.erase(it);
            refresh();
            if (change_cb_) change_cb_();
            return true;
        }
    }
    return false;
}

bool AnimationEditorModel::set_looping(std::string_view name, bool loop) {
    for (auto& c : entity_.clips) {
        if (c.name == name) { c.loop = loop; refresh(); if (change_cb_) change_cb_(); return true; }
    }
    return false;
}

} // namespace gspl::studio
