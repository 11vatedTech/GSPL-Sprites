#include "gspl/studio/git_integration.hpp"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>

namespace fs = std::filesystem;

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { \
    ++tests_run; \
    try { \
        name(); \
        std::printf("  PASS  %s\n", #name); \
    } catch (const std::exception& e) { \
        std::printf("  FAIL  %s: %s\n", #name, e.what()); \
        ++tests_failed; \
    } catch (...) { \
        std::printf("  FAIL  %s: unknown exception\n", #name); \
        ++tests_failed; \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error("assertion failed: " #cond); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    auto va = (a); auto vb = (b); \
    if (va != vb) { \
        throw std::runtime_error( \
            std::string("assertion failed: " #a " == " #b)); \
    } \
} while(0)

struct GitTestFixture {
    fs::path dir;

    GitTestFixture(const std::string& name) {
        dir = fs::temp_directory_path() / name;
        fs::remove_all(dir);
        fs::create_directories(dir);
        run("init -b main");
        run("config user.name \"Test User\"");
        run("config user.email \"test@gspl.dev\"");
        // Create an initial commit so HEAD resolves
        run("commit --allow-empty -m \"root\"");
    }

    ~GitTestFixture() {
        fs::remove_all(dir);
    }

    std::string run(const std::string& args) {
        std::string cmd = "git -C \"" + dir.string() + "\" " + args + " 2>&1";
        std::array<char, 4096> buffer{};
        std::string result;
        auto pipe = _popen(cmd.c_str(), "r");
        if (!pipe) return {};
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            result += buffer.data();
        }
        _pclose(pipe);
        return result;
    }

    void write_file(const std::string& name, const std::string& content) {
        std::ofstream f(dir / name);
        f << content;
    }
};

static void test_not_a_git_repo() {
    auto tmp = fs::temp_directory_path() / "gspl-test-not-repo";
    fs::remove_all(tmp);
    fs::create_directories(tmp);
    gspl::studio::GitIntegration git(tmp.string());
    ASSERT(!git.is_git_repo());
    fs::remove_all(tmp);
}

static void test_is_git_repo() {
    GitTestFixture fix("gspl-test-is-repo");
    gspl::studio::GitIntegration git(fix.dir.string());
    ASSERT(git.is_git_repo());
}

static void test_current_branch_default() {
    GitTestFixture fix("gspl-test-default-branch");
    gspl::studio::GitIntegration git(fix.dir.string());
    ASSERT(git.current_branch() == "main");
}

static void test_status_empty_repo() {
    GitTestFixture fix("gspl-test-empty-status");
    gspl::studio::GitIntegration git(fix.dir.string());
    auto st = git.status();
    ASSERT(st.empty());
}

static void test_status_untracked_file() {
    GitTestFixture fix("gspl-test-untracked");
    fix.write_file("test.txt", "hello");
    gspl::studio::GitIntegration git(fix.dir.string());
    auto st = git.status();
    ASSERT_EQ(st.size(), 1u);
    ASSERT(st[0].is_untracked());
    ASSERT(st[0].file_path.find("test.txt") != std::string::npos);
}

static void test_stage_and_commit() {
    GitTestFixture fix("gspl-test-commit");
    fix.write_file("hello.txt", "world");
    gspl::studio::GitIntegration git(fix.dir.string());

    bool staged = git.stage("hello.txt");
    ASSERT(staged);

    auto st = git.status();
    ASSERT(!st.empty());

    bool committed = git.commit("second commit");
    ASSERT(committed);

    st = git.status();
    ASSERT(st.empty());
}

static void test_log_after_commit() {
    GitTestFixture fix("gspl-test-log");
    fix.write_file("file.txt", "content");
    gspl::studio::GitIntegration git(fix.dir.string());
    git.stage("file.txt");
    git.commit("first real commit");

    auto commits = git.log(5);
    ASSERT(commits.size() >= 2u);  // root + our commit
    // Our commit should be the first in the log
    ASSERT(commits[0].message.find("first real commit") != std::string::npos);
    ASSERT(!commits[0].hash.empty());
}

static void test_multiple_commits_in_log() {
    GitTestFixture fix("gspl-test-multi-log");
    gspl::studio::GitIntegration git(fix.dir.string());

    fix.write_file("a.txt", "a");
    git.stage("a.txt");
    git.commit("commit a");

    fix.write_file("b.txt", "b");
    git.stage("b.txt");
    git.commit("commit b");

    auto commits = git.log(5);
    ASSERT(commits.size() >= 3u);
    ASSERT(commits[0].message.find("commit b") != std::string::npos);
    ASSERT(commits[1].message.find("commit a") != std::string::npos);
}

static void test_diff_modified_file() {
    GitTestFixture fix("gspl-test-diff");
    fix.write_file("file.txt", "original");
    gspl::studio::GitIntegration git(fix.dir.string());
    git.stage("file.txt");
    git.commit("initial");

    fix.write_file("file.txt", "modified");
    auto d = git.diff("file.txt");
    ASSERT(!d.empty());
}

static void test_diff_staged() {
    GitTestFixture fix("gspl-test-diff-staged");
    fix.write_file("f.txt", "version1");
    gspl::studio::GitIntegration git(fix.dir.string());
    git.stage("f.txt");
    git.commit("first");

    fix.write_file("f.txt", "version2");
    git.stage("f.txt");
    auto d = git.diff_staged("f.txt");
    ASSERT(!d.empty());
}

static void test_blame() {
    GitTestFixture fix("gspl-test-blame");
    fix.write_file("src.txt", "line1\nline2\nline3\n");
    gspl::studio::GitIntegration git(fix.dir.string());
    git.stage("src.txt");
    git.commit("write src");

    auto b = git.blame("src.txt", 1);
    ASSERT(!b.empty());
    ASSERT(b.find("Test User") != std::string::npos || b.find("test@gspl.dev") != std::string::npos);
}

static void test_unstage() {
    GitTestFixture fix("gspl-test-unstage");
    fix.write_file("stage.txt", "data");
    gspl::studio::GitIntegration git(fix.dir.string());
    git.stage("stage.txt");

    bool unstaged = git.unstage("stage.txt");
    ASSERT(unstaged);
}

static void test_checkout_branch() {
    GitTestFixture fix("gspl-test-branch");
    fix.run("branch new-branch");
    gspl::studio::GitIntegration git(fix.dir.string());

    bool ok = git.checkout_branch("new-branch");
    ASSERT(ok);
    ASSERT(git.current_branch() == "new-branch");
}

static void test_status_modified() {
    GitTestFixture fix("gspl-test-mod-status");
    fix.write_file("mod.txt", "original");
    gspl::studio::GitIntegration git(fix.dir.string());
    git.stage("mod.txt");
    git.commit("initial");

    fix.write_file("mod.txt", "changed");
    auto st = git.status();
    ASSERT(!st.empty());
    bool found_modified = false;
    for (const auto& s : st) {
        if (s.is_modified()) found_modified = true;
    }
    ASSERT(found_modified);
}

int main() {
    TEST(test_not_a_git_repo);
    TEST(test_is_git_repo);
    TEST(test_current_branch_default);
    TEST(test_status_empty_repo);
    TEST(test_status_untracked_file);
    TEST(test_stage_and_commit);
    TEST(test_log_after_commit);
    TEST(test_multiple_commits_in_log);
    TEST(test_diff_modified_file);
    TEST(test_diff_staged);
    TEST(test_blame);
    TEST(test_unstage);
    TEST(test_checkout_branch);
    TEST(test_status_modified);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
