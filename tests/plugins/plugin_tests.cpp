#include "gspl/plugin/manifest.hpp"
#include "gspl/plugin/plugin_manager.hpp"
#include "gspl/plugin/plugin_sandbox.hpp"
#include "gspl/plugin/plugin_api.h"
#include "gspl/studio/ipc_envelope.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <stdexcept>

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
            std::string("assertion failed: " #a " == " #b " (" + \
            std::to_string(va) + " != " + std::to_string(vb) + ")")); \
    } \
} while(0)

// ============================================================
// Manifest Tests
// ============================================================

static void test_manifest_validate_empty_id() {
    gspl::plugin::PluginManifest mf;
    mf.id = "";
    mf.version = "1.0.0";
    ASSERT(!mf.validate());
}

static void test_manifest_validate_valid() {
    gspl::plugin::PluginManifest mf;
    mf.id = "test-plugin";
    mf.version = "1.0.0";
    mf.api_version = GSPL_PLUGIN_API_VERSION;
    ASSERT(mf.validate());
}

static void test_manifest_validate_bad_version() {
    gspl::plugin::PluginManifest mf;
    mf.id = "test-plugin";
    mf.version = "not-a-version";
    ASSERT(!mf.validate());
}

static void test_manifest_validate_api_version_zero() {
    gspl::plugin::PluginManifest mf;
    mf.id = "test-plugin";
    mf.version = "1.0.0";
    mf.api_version = 0;
    ASSERT(!mf.validate());
}

static void test_manifest_to_json_contains_id() {
    gspl::plugin::PluginManifest mf;
    mf.id = "my-plugin";
    mf.version = "2.1.0";
    mf.name = "My Plugin";
    auto json = mf.to_json();
    ASSERT(json.find("\"my-plugin\"") != std::string::npos);
    ASSERT(json.find("\"2.1.0\"") != std::string::npos);
    ASSERT(json.find("\"My Plugin\"") != std::string::npos);
}

static void test_manifest_load_missing_file() {
    auto mf = gspl::plugin::PluginManifest::load("nonexistent.jsonc");
    ASSERT(!mf.has_value());
}

// ============================================================
// PluginManager Tests
// ============================================================

static void test_plugin_manager_empty() {
    gspl::plugin::PluginManager mgr;
    ASSERT(mgr.plugins().empty());
}

static void test_plugin_manager_discover_empty_dir() {
    gspl::plugin::PluginManager mgr;
    mgr.set_plugin_directories({"."});
    bool found = mgr.discover_plugins();
    ASSERT(!found);
    ASSERT(mgr.plugins().empty());
}

static void test_plugin_manager_load_nonexistent() {
    gspl::plugin::PluginManager mgr;
    mgr.set_plugin_directories({"."});
    mgr.discover_plugins();
    bool loaded = mgr.load_plugin("nonexistent-plugin");
    ASSERT(!loaded);
}

static void test_plugin_manager_find_returns_null_for_missing() {
    gspl::plugin::PluginManager mgr;
    auto* found = mgr.find("missing");
    ASSERT(found == nullptr);
}

static void test_plugin_manager_shutdown_all_empty() {
    gspl::plugin::PluginManager mgr;
    mgr.shutdown_all();
    ASSERT(mgr.plugins().empty());
}

static void test_plugin_manager_deactivate_all_empty() {
    gspl::plugin::PluginManager mgr;
    mgr.deactivate_all();
    ASSERT(mgr.plugins().empty());
}

// ============================================================
// IpcEnvelope Tests (plugin IPC context)
// ============================================================

static void test_ipc_envelope_roundtrip() {
    gspl::studio::IpcEnvelope env;
    env.magic = 0x4753504C;
    env.version = 1;
    env.message_id = 42;
    env.method = "test_method";
    env.payload = R"({"key":"value"})";
    env.has_shared_memory = false;

    auto serialized = env.serialize();
    auto deserialized = gspl::studio::IpcEnvelope::deserialize(serialized);

    ASSERT_EQ(deserialized.magic, env.magic);
    ASSERT_EQ(deserialized.version, env.version);
    ASSERT_EQ(deserialized.message_id, env.message_id);
    ASSERT(deserialized.method == env.method);
    ASSERT(deserialized.payload == env.payload);
    ASSERT_EQ(deserialized.has_shared_memory, env.has_shared_memory);
}

static void test_ipc_envelope_with_special_chars() {
    gspl::studio::IpcEnvelope env;
    env.message_id = 1;
    env.method = "invoke_hook";
    env.payload = "line1\nline2\ttab\"quote\\backslash";

    auto serialized = env.serialize();
    auto deserialized = gspl::studio::IpcEnvelope::deserialize(serialized);

    ASSERT(deserialized.method == env.method);
    ASSERT(deserialized.payload == env.payload);
}

// ============================================================
// Sandbox Tests (without actual process spawning)
// ============================================================

static void test_sandbox_config_defaults() {
    gspl::plugin::SandboxConfig cfg;
    ASSERT(cfg.plugin_directories.empty());
    ASSERT(cfg.worker_id.empty());
    ASSERT_EQ(cfg.restart_limit, 3);
}

static void test_sandbox_create_destroy() {
    gspl::plugin::SandboxConfig cfg;
    cfg.plugin_directories = {"."};
    cfg.worker_id = "test-sandbox";
    {
        gspl::plugin::PluginSandbox sandbox(std::move(cfg));
        // sandbox is created and destroyed without crash
    }
}

// ============================================================
// GsplPluginInfo structure tests
// ============================================================

static void test_plugin_info_defaults() {
    GsplPluginInfo info{};
    ASSERT_EQ(info.api_version, 0u);
    ASSERT(info.plugin_id == nullptr);
    ASSERT(info.plugin_version == nullptr);
}

static void test_plugin_info_api_version_constant() {
    ASSERT_EQ(GSPL_PLUGIN_API_VERSION, 1);
}

// ============================================================

int main() {
    TEST(test_manifest_validate_empty_id);
    TEST(test_manifest_validate_valid);
    TEST(test_manifest_validate_bad_version);
    TEST(test_manifest_validate_api_version_zero);
    TEST(test_manifest_to_json_contains_id);
    TEST(test_manifest_load_missing_file);

    TEST(test_plugin_manager_empty);
    TEST(test_plugin_manager_discover_empty_dir);
    TEST(test_plugin_manager_load_nonexistent);
    TEST(test_plugin_manager_find_returns_null_for_missing);
    TEST(test_plugin_manager_shutdown_all_empty);
    TEST(test_plugin_manager_deactivate_all_empty);

    TEST(test_ipc_envelope_roundtrip);
    TEST(test_ipc_envelope_with_special_chars);

    TEST(test_sandbox_config_defaults);
    TEST(test_sandbox_create_destroy);

    TEST(test_plugin_info_defaults);
    TEST(test_plugin_info_api_version_constant);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
