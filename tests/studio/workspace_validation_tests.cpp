#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) throw std::runtime_error("assertion failed: " #cond); } while(0)

static fs::path repo_root() {
    // Walk up from cwd to find .git directory
    fs::path p = fs::current_path();
    while (!p.empty() && !fs::exists(p / ".git")) p = p.parent_path();
    return p;
}

static void test_blank_workspace_exists() {
    auto root = repo_root();
    auto ws = root / "examples" / "blank-workspace";
    ASSERT(fs::exists(ws));
    ASSERT(fs::is_directory(ws));
    ASSERT(fs::exists(ws / "gspl-project.jsonc"));
    ASSERT(fs::exists(ws / "src"));
    ASSERT(fs::exists(ws / "src" / "main.gspl"));
}

static void test_voltfox_workspace_exists() {
    auto root = repo_root();
    auto ws = root / "examples" / "voltfox-workspace";
    ASSERT(fs::exists(ws));
    ASSERT(fs::exists(ws / "gspl-project.jsonc"));
    ASSERT(fs::exists(ws / "src"));
    ASSERT(fs::exists(ws / "src" / "voltfox.gspl"));
    ASSERT(fs::exists(ws / "src" / "morphology.gspl"));
    ASSERT(fs::exists(ws / "src" / "animation.gspl"));
}

static void test_sprite_kit_workspace_exists() {
    auto root = repo_root();
    auto ws = root / "examples" / "sprite-kit-workspace";
    ASSERT(fs::exists(ws));
    ASSERT(fs::exists(ws / "gspl-project.jsonc"));
    ASSERT(fs::exists(ws / "src"));
    ASSERT(fs::exists(ws / "src" / "patterns.gspl"));
}

int main() {
    TEST(test_blank_workspace_exists);
    TEST(test_voltfox_workspace_exists);
    TEST(test_sprite_kit_workspace_exists);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
