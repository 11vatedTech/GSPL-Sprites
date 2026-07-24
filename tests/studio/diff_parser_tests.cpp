#include "gspl/studio/diff_parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) throw std::runtime_error("assertion failed: " #cond); } while(0)
#define ASSERT_EQ(a, b) do { auto va = (a); auto vb = (b); if (va != vb) throw std::runtime_error(std::string("assertion failed: " #a " == " #b)); } while(0)

using namespace gspl::studio;

static void test_diff_parse_empty() {
    auto files = DiffParser::parse("");
    ASSERT(files.empty());
}

static void test_diff_parse_single_hunk() {
    std::string diff =
        "--- a/src/test.gspl\n"
        "+++ b/src/test.gspl\n"
        "@@ -1,3 +1,4 @@\n"
        " line1\n"
        "-old_line\n"
        "+new_line\n"
        " line3\n";

    auto files = DiffParser::parse(diff);
    ASSERT_EQ(files.size(), 1u);
    ASSERT_EQ(files[0].old_path, "src/test.gspl");
    ASSERT_EQ(files[0].new_path, "src/test.gspl");
    ASSERT_EQ(files[0].hunks.size(), 1u);
    ASSERT_EQ(files[0].hunks[0].lines.size(), 4u);
}

static void test_diff_parse_line_types() {
    std::string diff =
        "--- a/x.gspl\n"
        "+++ b/x.gspl\n"
        "@@ -1,2 +1,3 @@\n"
        " ctx\n"
        "-rem\n"
        "+add\n";

    auto files = DiffParser::parse(diff);
    ASSERT_EQ(files.size(), 1u);
    auto& lines = files[0].hunks[0].lines;
    ASSERT_EQ(lines[0].type, DiffLineType::Context);
    ASSERT_EQ(lines[1].type, DiffLineType::Removed);
    ASSERT_EQ(lines[2].type, DiffLineType::Added);
}

static void test_diff_is_gspl() {
    DiffFile f;
    f.old_path = "src/entity.gspl";
    f.new_path = "src/entity.gspl";
    ASSERT(f.is_gspl());

    f.new_path = "src/main.cpp";
    ASSERT(!f.is_gspl());

    f.new_path = "gspl-project.jsonc";
    ASSERT(f.is_gspl());

    f.new_path = "src/test.gsplj";
    ASSERT(f.is_gspl());
}

static void test_diff_filename() {
    DiffFile f;
    f.new_path = "src/subdir/entity.gspl";
    ASSERT_EQ(f.filename(), "entity.gspl");

    f.new_path = "entity.gspl";
    ASSERT_EQ(f.filename(), "entity.gspl");
}

static void test_diff_filter_gspl() {
    std::string diff =
        "--- a/a.gspl\n"
        "+++ b/a.gspl\n"
        "@@ -1,1 +1,1 @@\n"
        "-old\n"
        "+new\n"
        "--- a/b.cpp\n"
        "+++ b/b.cpp\n"
        "@@ -1,1 +1,1 @@\n"
        "-x\n"
        "+y\n";

    auto files = DiffParser::parse(diff);
    ASSERT_EQ(files.size(), 2u);
    ASSERT(files[0].is_gspl());
    ASSERT(!files[1].is_gspl());
    auto gspl = DiffParser::filter_gspl(files);
    ASSERT_EQ(gspl.size(), 1u);
    ASSERT_EQ(gspl[0].filename(), "a.gspl");
}

static void test_diff_stats() {
    std::string diff =
        "--- a/a.gspl\n"
        "+++ b/a.gspl\n"
        "@@ -1,2 +1,3 @@\n"
        " ctx\n"
        "-rem\n"
        "+add1\n"
        "+add2\n";

    auto files = DiffParser::parse(diff);
    auto s = DiffParser::stats(files);
    ASSERT_EQ(s.files_changed, 1);
    ASSERT_EQ(s.gspl_files, 1);
    ASSERT_EQ(s.insertions, 2);
    ASSERT_EQ(s.deletions, 1);
}

static void test_diff_parse_multiple_files() {
    std::string diff =
        "--- a/f1.gspl\n"
        "+++ b/f1.gspl\n"
        "@@ -1,1 +1,1 @@\n"
        "-a\n"
        "+b\n"
        "--- a/f2.gspl\n"
        "+++ b/f2.gspl\n"
        "@@ -1,1 +1,1 @@\n"
        "-c\n"
        "+d\n";

    auto files = DiffParser::parse(diff);
    ASSERT_EQ(files.size(), 2u);
    ASSERT_EQ(files[0].filename(), "f1.gspl");
    ASSERT_EQ(files[1].filename(), "f2.gspl");
}

static void test_diff_hunk_header_parsing() {
    std::string diff =
        "--- a/x.gspl\n"
        "+++ b/x.gspl\n"
        "@@ -10,6 +12,8 @@ some context\n"
        " line10\n"
        "-line11_old\n"
        "+line11_new\n";

    auto files = DiffParser::parse(diff);
    ASSERT_EQ(files.size(), 1u);
    auto& hunk = files[0].hunks[0];
    ASSERT_EQ(hunk.old_start, 10);
    ASSERT_EQ(hunk.old_count, 6);
    ASSERT_EQ(hunk.new_start, 12);
    ASSERT_EQ(hunk.new_count, 8);
}

int main() {
    TEST(test_diff_parse_empty);
    TEST(test_diff_parse_single_hunk);
    TEST(test_diff_parse_line_types);
    TEST(test_diff_is_gspl);
    TEST(test_diff_filename);
    TEST(test_diff_filter_gspl);
    TEST(test_diff_stats);
    TEST(test_diff_parse_multiple_files);
    TEST(test_diff_hunk_header_parsing);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
