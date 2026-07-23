#include "gspl/studio/project.hpp"
#include "gspl/studio/workspace.hpp"
#include "gspl/studio/document.hpp"
#include "gspl/studio/undo_stack.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>

void test_undo_stack() {
    gspl::studio::UndoStack stack(10);
    assert(!stack.can_undo());
    assert(!stack.can_redo());
    
    struct TestCommand : gspl::studio::UndoCommand {
        int& value;
        int old_val;
        int new_val;
        TestCommand(int& v, int old_v, int new_v) : value(v), old_val(old_v), new_val(new_v) {}
        void execute() override { value = new_val; }
        void undo() override { value = old_val; }
        void redo() override { value = new_val; }
        std::string description() const override { return "test"; }
    };
    
    int val = 0;
    stack.push(std::make_unique<TestCommand>(val, 0, 42));
    assert(val == 42);
    assert(stack.can_undo());
    assert(stack.undo_description() == "test");
    
    stack.undo();
    assert(val == 0);
    assert(stack.can_redo());
    
    stack.redo();
    assert(val == 42);
}

void test_workspace_create() {
    auto tmp = std::filesystem::temp_directory_path() / "gspl_test_workspace";
    std::filesystem::remove_all(tmp);
    
    gspl::studio::Workspace::Config cfg;
    cfg.workspace_dir = tmp;
    cfg.db_filename = ":memory:"; // In-memory for testing
    
    gspl::studio::Workspace ws(cfg);
    assert(ws.open());
    assert(ws.is_open());
    assert(ws.projects().empty());
    
    ws.close();
    std::filesystem::remove_all(tmp);
}

int main() {
    test_undo_stack();
    test_workspace_create();
    std::printf("All project comprehensive tests passed.\n");
    return 0;
}
