#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <functional>

namespace gspl::studio {

enum class DocumentKind {
    Text,       // GSPL source file
    Gene,       // Gene editor
    Morphology, // Morphology editor
    Form,       // Form editor
    Animation,  // Animation editor
    Behavior,   // Behavior editor
    Combat,     // Combat editor
    Graph,      // Graph/schematic editor
    Preview,    // Preview viewport
    Artifact    // Artifact inspector
};

enum class DocumentChange {
    Content,
    Cursor,
    Selection,
    Dirty,
    Saved,
    Undo,
    Redo
};

class Document {
public:
    using ChangeCallback = std::function<void(DocumentChange)>;

    Document(DocumentKind kind, std::string name);
    virtual ~Document() = default;

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    [[nodiscard]] auto kind() const -> DocumentKind;
    [[nodiscard]] auto name() const -> const std::string&;
    void set_name(std::string name);

    [[nodiscard]] bool is_dirty() const;
    void set_dirty(bool dirty);

    [[nodiscard]] auto file_path() const -> const std::string&;
    void set_file_path(std::string path);

    virtual bool load();
    virtual bool save();

    void set_change_callback(ChangeCallback cb);

protected:
    void notify_change(DocumentChange change);

private:
    DocumentKind kind_;
    std::string name_;
    bool dirty_{false};
    std::string file_path_;
    ChangeCallback change_callback_;
};

} // namespace gspl::studio
