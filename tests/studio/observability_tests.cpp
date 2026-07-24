#include "gspl/studio/telemetry_bus.hpp"
#include "gspl/studio/diagnostics_export.hpp"
#include "gspl/studio/crash_reporter.hpp"
#include "gspl/studio/metrics.hpp"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>

namespace fs = std::filesystem;
static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) throw std::runtime_error("assertion failed: " #cond); } while(0)
#define ASSERT_EQ(a, b) do { auto va = (a); auto vb = (b); if (va != vb) throw std::runtime_error(std::string("assertion failed: " #a " == " #b)); } while(0)

using gspl::studio::TelemetryBus;
using gspl::studio::DiagnosticsExport;
using gspl::studio::CrashReporter;
using gspl::studio::MetricsCollector;

// --- TelemetryBus ---

static void test_telemetry_not_active_by_default() {
    TelemetryBus bus;
    ASSERT(!bus.is_active());
}

static void test_telemetry_start_stop_session() {
    TelemetryBus bus;
    bus.start_session();
    ASSERT(bus.is_active());
    bus.end_session();
    ASSERT(!bus.is_active());
}

static void test_telemetry_buffer_events() {
    TelemetryBus bus;
    bus.emit("test_event", "testing", "data");
    ASSERT_EQ(bus.buffered_events().size(), 1u);
    ASSERT(bus.buffered_events()[0].name == "test_event");
}

static void test_telemetry_clear_buffer() {
    TelemetryBus bus;
    bus.emit("e1", "cat1");
    bus.clear_buffer();
    ASSERT(bus.buffered_events().empty());
}

static void test_telemetry_subscribe() {
    TelemetryBus bus;
    int call_count = 0;
    bus.subscribe("cat", [&](const auto&) { ++call_count; });
    bus.emit("e1", "cat");
    ASSERT_EQ(call_count, 1);
}

static void test_telemetry_unsubscribe_all() {
    TelemetryBus bus;
    int c = 0;
    bus.subscribe("cat", [&](const auto&) { ++c; });
    bus.unsubscribe_all("cat");
    bus.emit("e1", "cat");
    ASSERT_EQ(c, 0);
}

// --- DiagnosticsExport ---

static void test_diagnostics_export_creates_file() {
    auto path = fs::temp_directory_path() / "gspl-test-diag-export.txt";
    DiagnosticsExport de;
    de.set_include_logs(true);
    de.set_include_system_info(true);
    bool ok = de.export_to_file(path.string());
    ASSERT(ok);
    ASSERT(fs::exists(path));
    fs::remove(path);
}

static void test_diagnostics_system_info_not_empty() {
    DiagnosticsExport de;
    auto info = de.collect_system_info();
    ASSERT(!info.empty());
}

// --- CrashReporter ---

static void test_crash_reporter_no_pending_by_default() {
    CrashReporter cr;
    ASSERT(!cr.has_pending_report());
}

static void test_crash_reporter_generate_report() {
    CrashReporter cr;
    cr.generate_report("SegFault", "access violation", "0x1234");
    ASSERT(cr.has_pending_report());
    ASSERT(cr.last_report().exception_type == "SegFault");
}

static void test_crash_reporter_clear_pending() {
    CrashReporter cr;
    cr.generate_report("OOM", "out of memory", "");
    ASSERT(cr.has_pending_report());
    cr.clear_pending();
    ASSERT(!cr.has_pending_report());
}

static void test_crash_reporter_save_load() {
    auto path = fs::temp_directory_path() / "gspl-test-crash.txt";
    CrashReporter cr;
    cr.set_dump_directory(fs::temp_directory_path().string());
    cr.generate_report("Test", "test crash", "trace123");

    bool saved = cr.save_report(cr.last_report(), path.string());
    ASSERT(saved);
    ASSERT(fs::exists(path));

    auto loaded = cr.load_report(path.string());
    ASSERT(loaded.exception_type == "Test");
    fs::remove(path);
}

// --- MetricsCollector ---

static void test_metrics_counter_default_zero() {
    MetricsCollector mc;
    ASSERT_EQ(mc.get_counter("nonexistent"), 0.0);
}

static void test_metrics_increment() {
    MetricsCollector mc;
    mc.increment("hits");
    mc.increment("hits");
    ASSERT_EQ(mc.get_counter("hits"), 2.0);
}

static void test_metrics_gauge() {
    MetricsCollector mc;
    mc.gauge("temperature", 36.6);
    ASSERT_EQ(mc.get_gauge("temperature"), 36.6);
}

static void test_metrics_timer() {
    MetricsCollector mc;
    {
        MetricsCollector::Timer t(mc, "op");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    auto avg = mc.get_timing_avg("op");
    ASSERT(avg > 0.0);
}

static void test_metrics_snapshot() {
    MetricsCollector mc;
    mc.increment("a", 1);
    mc.gauge("b", 2.5);
    auto snap = mc.snapshot();
    ASSERT(snap.find("counter_a") != snap.end());
    ASSERT(snap.find("gauge_b") != snap.end());
}

static void test_metrics_reset() {
    MetricsCollector mc;
    mc.increment("x");
    mc.reset();
    ASSERT_EQ(mc.get_counter("x"), 0.0);
}

int main() {
    TEST(test_telemetry_not_active_by_default);
    TEST(test_telemetry_start_stop_session);
    TEST(test_telemetry_buffer_events);
    TEST(test_telemetry_clear_buffer);
    TEST(test_telemetry_subscribe);
    TEST(test_telemetry_unsubscribe_all);

    TEST(test_diagnostics_export_creates_file);
    TEST(test_diagnostics_system_info_not_empty);

    TEST(test_crash_reporter_no_pending_by_default);
    TEST(test_crash_reporter_generate_report);
    TEST(test_crash_reporter_clear_pending);
    TEST(test_crash_reporter_save_load);

    TEST(test_metrics_counter_default_zero);
    TEST(test_metrics_increment);
    TEST(test_metrics_gauge);
    TEST(test_metrics_timer);
    TEST(test_metrics_snapshot);
    TEST(test_metrics_reset);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
