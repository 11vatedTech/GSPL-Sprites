#include "gspl/studio/bracket_matcher.hpp"
#include "gspl/studio/snippet_engine.hpp"
#include "gspl/studio/search_engine.hpp"
#include "gspl/studio/squiggle_generator.hpp"
#include "gspl/studio/code_folding.hpp"
#include "gspl/studio/quick_fix.hpp"
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

static void test_bracket_matcher_find_match() {
    ASSERT_EQ(BracketMatcher::find_match("a(b)c", 1).value_or(999), 3u);
    ASSERT_EQ(BracketMatcher::find_match("a(b)c", 3).value_or(999), 1u);
    ASSERT_EQ(BracketMatcher::find_match("x{y}z", 1).value_or(999), 3u);
    ASSERT(!BracketMatcher::find_match("abc", 0).has_value());
}

static void test_bracket_matcher_skip_strings() {
    ASSERT_EQ(BracketMatcher::find_match("\"(\" ( )", 4).value_or(999), 6u);
    ASSERT_EQ(BracketMatcher::find_match("\"(\" ( )", 6).value_or(999), 4u);
}

static void test_bracket_matcher_all_pairs() {
    auto pairs = BracketMatcher::find_all_pairs("a(b)c{d}e");
    ASSERT_EQ(pairs.size(), 2u);
    ASSERT(pairs[0].matched);
    ASSERT(pairs[1].matched);
}

static void test_bracket_matcher_containing_pair() {
    auto pair = BracketMatcher::containing_pair("a(bcd)e", 3);
    ASSERT(pair.has_value());
    ASSERT_EQ(pair->open_char, '(');
    ASSERT_EQ(pair->close_char, ')');
}

static void test_bracket_matcher_auto_indent() {
    auto r = BracketMatcher::auto_indent("if (x) {", "", 4);
    ASSERT_EQ(r.indent_level, 1u);
    ASSERT_EQ(r.line, "    ");

    r = BracketMatcher::auto_indent("    } else {", "", 4);
    ASSERT_EQ(r.indent_level, 1u);
}

static void test_snippet_engine_basic() {
    SnippetEngine engine;
    auto snips = engine.find("entity");
    ASSERT(!snips.empty());
    ASSERT_EQ(snips[0]->prefix, "entity");
}

static void test_snippet_engine_expand() {
    auto exp = SnippetEngine::expand("hello ${1:world}");
    ASSERT_EQ(exp.text, "hello world");
    ASSERT_EQ(exp.tab_stops.size(), 1u);
    ASSERT_EQ(exp.tab_stops[0].default_value, "world");
}

static void test_snippet_engine_expand_simple() {
    auto exp = SnippetEngine::expand("foo $1 bar");
    ASSERT_EQ(exp.text, "foo  bar");
    ASSERT_EQ(exp.tab_stops.size(), 1u);
    ASSERT_EQ(exp.tab_stops[0].index, 1u);
}

static void test_search_engine_find() {
    SearchEngine se("hello world\nfoo bar\nhello again");
    SearchQuery q;
    q.pattern = "hello";
    auto matches = se.find_all(q);
    ASSERT_EQ(matches.size(), 2u);
    ASSERT_EQ(matches[0].line, 0u);
    ASSERT_EQ(matches[1].line, 2u);
}

static void test_search_engine_case_sensitive() {
    SearchEngine se("Hello HELLO hello");
    SearchQuery q;
    q.pattern = "hello";
    q.case_sensitive = true;
    auto matches = se.find_all(q);
    ASSERT_EQ(matches.size(), 1u);
}

static void test_search_engine_replace_all() {
    SearchEngine se("a b a c a");
    SearchQuery q;
    q.pattern = "a";
    auto r = se.replace_all(q, "x");
    ASSERT_EQ(r.text, "x b x c x");
    ASSERT_EQ(r.replacements, 3u);
}

static void test_search_engine_incremental() {
    SearchEngine se("cat dog cat bird cat");
    auto r = se.incremental_search("cat");
    ASSERT_EQ(r.match_count, 3u);
    ASSERT(r.current_match.has_value());
}

static void test_squiggle_generator_basic() {
    SquiggleGenerator gen;
    gen.add_source([]() -> std::vector<SquiggleRange> {
        SquiggleRange r;
        r.line = 0; r.start_col = 5; r.end_col = 10;
        r.severity = SquiggleSeverity::Error;
        r.message = "test error";
        return {r};
    });
    auto squiggles = gen.generate();
    ASSERT_EQ(squiggles.size(), 1u);
    ASSERT_EQ(squiggles[0].line, 0u);
    ASSERT_EQ(squiggles[0].message, "test error");
}

static void test_squiggle_generator_for_line() {
    auto all = std::vector<SquiggleRange>{};
    SquiggleRange r1, r2;
    r1.line = 0; r2.line = 1;
    all.push_back(r1); all.push_back(r2);
    SquiggleGenerator gen;
    auto line0 = gen.for_line(all, 0);
    ASSERT_EQ(line0.size(), 1u);
}

static void test_squiggle_from_diagnostic() {
    auto sq = SquiggleGenerator::from_diagnostic(2001, "expected ';'", 3, 10, 1);
    ASSERT_EQ(sq.line, 3u);
    ASSERT_EQ(sq.message, "expected ';'");
    ASSERT(sq.severity == SquiggleSeverity::Warning);
}

static void test_code_folding_regions() {
    CodeFoldingModel model("{\n    x\n}");
    ASSERT(!model.regions().empty());
}

static void test_code_folding_toggle() {
    CodeFoldingModel model("{\n    x\n    y\n}");
    auto const& r = model.regions();
    if (!r.empty()) {
        model.toggle(r[0].start_line);
        ASSERT(r[0].collapsed);
        ASSERT(model.is_hidden(r[0].start_line + 1));
        model.toggle(r[0].start_line);
        ASSERT(!r[0].collapsed);
    }
}

static void test_quick_fix_provider_creation() {
    QuickFixProvider provider;
    // Should have built-in fixers registered
    QuickFixDiagnostic diag;
    diag.code = 2002; // missing semicolon
    diag.line = 5;
    diag.column = 10;
    diag.length = 1;
    auto fixes = provider.get_fixes(diag);
    ASSERT(!fixes.empty());
}

static void test_quick_fix_missing_semicolon() {
    auto action = builtin_fixes::fix_missing_semicolon({2002, "expected ';'", 3, 15, 1});
    ASSERT_EQ(action.label, "Insert ';'");
    ASSERT(!action.edits.empty());
    ASSERT_EQ(action.edits[0].replacement, ";");
    ASSERT(action.is_preferred);
}

static void test_quick_fix_unterminated_string() {
    auto action = builtin_fixes::fix_unterminated_string({1002, "", 0, 0, 8});
    ASSERT_EQ(action.label, "Add closing quote");
    ASSERT_EQ(action.edits[0].replacement, "\"");
}

static void test_quick_fix_unbalanced_brace() {
    auto action = builtin_fixes::fix_unbalanced_brace({2003, "", 0, 0, 1});
    ASSERT_EQ(action.edits[0].replacement, "\n}");
}

static void test_quick_fix_type_mismatch() {
    auto action = builtin_fixes::fix_type_mismatch_cast({5001, "", 1, 5, 4});
    ASSERT_EQ(action.edits.size(), 2u);
    ASSERT_EQ(action.edits[0].replacement, "as<type>(");
    ASSERT_EQ(action.edits[1].replacement, ")");
}

static void test_quick_fix_register_custom() {
    QuickFixProvider provider;
    int test_code = 99999;
    bool called = false;
    provider.register_fixer(test_code, [&](QuickFixDiagnostic const&) {
        called = true;
        return std::vector<QuickFixAction>{};
    });
    provider.get_fixes({test_code, "", 0, 0, 0});
    ASSERT(called);
}

static void test_quick_fix_register_replace() {
    QuickFixProvider provider;
    provider.clear();
    provider.register_replace_fixer(2002, "Replace with colon", ":");
    auto fixes = provider.get_fixes({2002, "", 0, 5, 1});
    ASSERT(!fixes.empty());
    ASSERT_EQ(fixes[0].label, "Replace with colon");
    ASSERT_EQ(fixes[0].edits[0].replacement, ":");
}

static void test_quick_fix_clear() {
    QuickFixProvider provider;
    provider.clear();
    auto fixes = provider.get_fixes({2002, "", 0, 0, 0});
    ASSERT(fixes.empty());
}

int main() {
    TEST(test_bracket_matcher_find_match);
    TEST(test_bracket_matcher_skip_strings);
    TEST(test_bracket_matcher_all_pairs);
    TEST(test_bracket_matcher_containing_pair);
    TEST(test_bracket_matcher_auto_indent);
    TEST(test_snippet_engine_basic);
    TEST(test_snippet_engine_expand);
    TEST(test_snippet_engine_expand_simple);
    TEST(test_search_engine_find);
    TEST(test_search_engine_case_sensitive);
    TEST(test_search_engine_replace_all);
    TEST(test_search_engine_incremental);
    TEST(test_squiggle_generator_basic);
    TEST(test_squiggle_generator_for_line);
    TEST(test_squiggle_from_diagnostic);
    TEST(test_code_folding_regions);
    TEST(test_code_folding_toggle);
    TEST(test_quick_fix_provider_creation);
    TEST(test_quick_fix_missing_semicolon);
    TEST(test_quick_fix_unterminated_string);
    TEST(test_quick_fix_unbalanced_brace);
    TEST(test_quick_fix_type_mismatch);
    TEST(test_quick_fix_register_custom);
    TEST(test_quick_fix_register_replace);
    TEST(test_quick_fix_clear);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
