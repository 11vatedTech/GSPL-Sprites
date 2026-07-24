#include "gspl/studio/project_tree_model.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

static int tests_run = 0;
static int tests_failed = 0;
static auto test_dir = std::filesystem::temp_directory_path() / "gspl_tree_test_XXXXXX";

#define TEST(name) do { ++tests_run; \
  fprintf(stdout, "  " name "..."); fflush(stdout); } while(0)
#define PASS() fprintf(stdout, " PASS\n")
#define FAIL(msg) do { fprintf(stdout, " FAIL: %s\n", msg); ++tests_failed; } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)

static std::filesystem::path create_test_dir() {
    auto tmp = std::filesystem::temp_directory_path();
    for (int i = 0; i < 100; ++i) {
        auto p = tmp / ("gspl_tree_test_" + std::to_string(i));
        if (!std::filesystem::exists(p)) {
            std::filesystem::create_directories(p);
            return p;
        }
    }
    return tmp / "gspl_tree_test_fallback";
}

static void test_refresh_empty_dir() {
    TEST("refresh on empty directory");
    auto dir = create_test_dir();
    gspl::studio::ProjectTreeModel model(dir);
    model.refresh();
    ASSERT(model.root() != nullptr, "root exists");
    ASSERT(model.root()->is_directory, "root is directory");
    ASSERT(model.root()->children.empty(), "no children");
    ASSERT(model.flat_entries().size() == 1, "only root in flat");
    std::filesystem::remove_all(dir);
    PASS();
}

static void test_refresh_with_files() {
    TEST("refresh with source files");
    auto dir = create_test_dir();
    {
        std::ofstream(dir / "main.gspl") << "entity Test {}";
        std::ofstream(dir / "config.jsonc") << "{}";
        std::filesystem::create_directory(dir / "subdir");
        std::ofstream(dir / "subdir" / "helper.gspl") << "morph {}";
    }
    gspl::studio::ProjectTreeModel model(dir);
    model.refresh();
    ASSERT(model.root()->children.size() >= 3, "has files and dirs");
    ASSERT(model.find_by_path("main.gspl") != nullptr, "found main.gspl");
    ASSERT(model.find_by_path("config.jsonc") != nullptr, "found config.jsonc");
    ASSERT(model.find_by_path("subdir/helper.gspl") != nullptr, "found nested file");
    ASSERT(model.find_by_path("nonexistent.txt") == nullptr, "not found");
    std::filesystem::remove_all(dir);
    PASS();
}

static void test_create_file() {
    TEST("create file");
    auto dir = create_test_dir();
    gspl::studio::ProjectTreeModel model(dir);
    model.refresh();

    ASSERT(model.create_file("new.gspl", "entity X {}") == true, "create file");
    ASSERT(model.find_by_path("new.gspl") != nullptr, "found new file");
    ASSERT(model.read_file("new.gspl") == "entity X {}", "content matches");

    // Duplicate
    ASSERT(model.create_file("new.gspl", "") == false, "duplicate rejected");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_create_directory() {
    TEST("create directory");
    auto dir = create_test_dir();
    gspl::studio::ProjectTreeModel model(dir);
    model.refresh();

    ASSERT(model.create_directory("assets") == true, "create dir");
    ASSERT(model.create_directory("assets") == false, "duplicate dir rejected");
    ASSERT(model.create_file("assets/sprite.gspl", "test") == true, "file in new dir");
    ASSERT(model.find_by_path("assets/sprite.gspl") != nullptr, "found nested file");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_rename_and_remove() {
    TEST("rename and remove");
    auto dir = create_test_dir();
    {
        std::ofstream(dir / "old.gspl") << "old";
    }
    gspl::studio::ProjectTreeModel model(dir);
    model.refresh();

    ASSERT(model.rename("old.gspl", "new.gspl") == true, "rename");
    ASSERT(model.find_by_path("old.gspl") == nullptr, "old gone");
    ASSERT(model.find_by_path("new.gspl") != nullptr, "new exists");
    ASSERT(model.rename("nonexistent", "x.gspl") == false, "rename nonexistent");

    ASSERT(model.remove("new.gspl") == true, "remove");
    ASSERT(model.find_by_path("new.gspl") == nullptr, "removed gone");
    ASSERT(model.remove("nonexistent") == false, "remove nonexistent");

    std::filesystem::remove_all(dir);
    PASS();
}

static void test_filter_by_kind() {
    TEST("filter by kind");
    auto dir = create_test_dir();
    {
        std::ofstream(dir / "a.gspl") << "a";
        std::ofstream(dir / "b.gspl") << "b";
        std::ofstream(dir / "config.jsonc") << "{}";
        std::ofstream(dir / "readme.txt") << "note";
    }
    gspl::studio::ProjectTreeModel model(dir);
    model.refresh();
    auto sources = model.filter_by_kind(gspl::studio::TreeEntryKind::SourceFile);
    ASSERT(sources.size() == 2, "2 source files");
    auto configs = model.filter_by_kind(gspl::studio::TreeEntryKind::ConfigFile);
    ASSERT(configs.size() == 1, "1 config file");
    std::filesystem::remove_all(dir);
    PASS();
}

static void test_status_providers() {
    TEST("status providers");
    auto dir = create_test_dir();
    {
        std::ofstream(dir / "mod.gspl") << "mod";
        std::ofstream(dir / "ok.gspl") << "ok";
    }
    gspl::studio::ProjectTreeModel model(dir);
    model.set_git_status_provider([]() -> std::vector<std::string> {
        return {"mod.gspl"};
    });
    model.set_compile_error_provider([](std::string_view path) -> bool {
        return path == "mod.gspl";
    });
    model.refresh();

    auto* mod = model.find_by_path("mod.gspl");
    ASSERT(mod != nullptr && mod->has_git_changes, "mod has git changes");
    ASSERT(mod != nullptr && mod->has_compile_error, "mod has compile error");

    auto* ok = model.find_by_path("ok.gspl");
    ASSERT(ok != nullptr && !ok->has_git_changes, "ok no git changes");
    ASSERT(ok != nullptr && !ok->has_compile_error, "ok no compile error");

    std::filesystem::remove_all(dir);
    PASS();
}

int main() {
    test_refresh_empty_dir();
    test_refresh_with_files();
    test_create_file();
    test_create_directory();
    test_rename_and_remove();
    test_filter_by_kind();
    test_status_providers();

    fprintf(stdout, "\n%d tests run, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
