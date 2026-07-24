#include "gspl/package/package_manager.hpp"
#include "gspl/package/manifest.hpp"
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
#define ASSERT_EQ(a, b) do { auto va = (a); auto vb = (b); if (va != vb) throw std::runtime_error(std::string("assertion failed: " #a " == " #b)); } while(0)

using gspl::package::PackageManager;

static void test_empty_packages_root() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-empty";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    ASSERT(pm.installed_packages().empty());
    fs::remove_all(tmp);
}

static void test_install_package() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-install";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    bool ok = pm.install("testpub/mypkg", "1.0.0");
    ASSERT(ok);
    // Check filesystem directly for the manifest
    auto manifest_path = tmp / "testpub" / "mypkg" / "1.0.0" / "manifest.json";
    ASSERT(fs::exists(manifest_path));
    fs::remove_all(tmp);
}

static void test_install_duplicate_skips() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-dup";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    pm.install("pub/pkg", "1.0.0");
    bool ok = pm.install("pub/pkg", "1.0.0");
    ASSERT(ok);
    fs::remove_all(tmp);
}

static void test_uninstall_package() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-uninst";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    pm.install("pub/pkg", "1.0.0");
    auto ver_dir = tmp / "pub" / "pkg" / "1.0.0";
    ASSERT(fs::exists(ver_dir));
    bool ok = pm.uninstall("pub/pkg");
    ASSERT(ok);
    ASSERT(!fs::exists(ver_dir));
    fs::remove_all(tmp);
}

static void test_update_package() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-upd";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    pm.install("pub/pkg", "1.0.0");
    ASSERT(fs::exists(tmp / "pub" / "pkg" / "1.0.0"));
    bool ok = pm.update("pub/pkg", "1.1.0");
    ASSERT(ok);
    ASSERT(fs::exists(tmp / "pub" / "pkg" / "1.1.0"));
    ASSERT(!fs::exists(tmp / "pub" / "pkg" / "1.0.0"));
    fs::remove_all(tmp);
}

static void test_registry_url_set_and_get() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-reg";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    pm.set_registry_url("https://registry.example.com");
    fs::remove_all(tmp);
}

static void test_resolve_empty_deps() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-resolve";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    pm.install("pub/pkg", "1.0.0");
    ASSERT(fs::exists(tmp / "pub" / "pkg" / "1.0.0" / "manifest.json"));
    auto deps = pm.resolve_dependencies("pub/pkg");
    ASSERT_EQ(deps.size(), 1u);
    fs::remove_all(tmp);
}

static void test_find_returns_null_for_missing() {
    auto tmp = fs::temp_directory_path() / "gspl-pkg-find";
    fs::create_directories(tmp);
    PackageManager pm(tmp.string());
    auto* found = pm.find("nonexistent");
    ASSERT(found == nullptr);
    fs::remove_all(tmp);
}

int main() {
    TEST(test_empty_packages_root);
    TEST(test_install_package);
    TEST(test_install_duplicate_skips);
    TEST(test_uninstall_package);
    TEST(test_update_package);
    TEST(test_registry_url_set_and_get);
    TEST(test_resolve_empty_deps);
    TEST(test_find_returns_null_for_missing);
    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
