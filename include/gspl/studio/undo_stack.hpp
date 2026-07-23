#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <vector>

namespace gspl::studio {

// Command-pattern undo stack with JSON delta snapshots
class UndoCommand {
public:
    virtual ~UndoCommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;
    [[nodiscard]] virtual auto description() const -> std::string = 0;
};

class UndoStack {
public:
    explicit UndoStack(std::size_t max_depth = 100);
    ~UndoStack() = default;

    void push(std::unique_ptr<UndoCommand> cmd);
    bool undo();
    bool redo();

    [[nodiscard]] bool can_undo() const;
    [[nodiscard]] bool can_redo() const;

    [[nodiscard]] auto undo_description() const -> std::string;
    [[nodiscard]] auto redo_description() const -> std::string;

    void clear();

private:
    std::vector<std::unique_ptr<UndoCommand>> commands_;
    std::size_t current_{0};
    std::size_t max_depth_;
};

} // namespace gspl::studio
