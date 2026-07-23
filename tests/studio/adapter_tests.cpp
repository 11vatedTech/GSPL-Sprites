#include "gspl/studio/adapters/godot_adapter.hpp"
#include "gspl/studio/adapters/unity_adapter.hpp"
#include "gspl/studio/adapters/unreal_adapter.hpp"
#include <cassert>
#include <cstdio>
#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace gspl::studio::adapters;
namespace fs = std::filesystem;

static int tests_run = 0;
static int tests_failed = 0;

#define TEST(name) do { ++tests_run; try { name(); std::printf("  PASS  %s\n", #name); } catch (const std::exception& e) { std::printf("  FAIL  %s: %s\n", #name, e.what()); ++tests_failed; } catch (...) { std::printf("  FAIL  %s: unknown exception\n", #name); ++tests_failed; } } while(0)
#define ASSERT(cond) do { if (!(cond)) { throw std::runtime_error("assertion failed: " #cond); } } while(0)
#define ASSERT_NO_THROW(expr) do { try { expr; } catch (...) { throw std::runtime_error("expected no exception: " #expr); } } while(0)

namespace {
void cleanup_dir(const fs::path& dir) {
    for (int attempt = 0; attempt < 5; ++attempt) {
        std::error_code ec;
        fs::remove_all(dir, ec);
        if (!ec) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
} // namespace

// === Godot Adapter ===

void test_godot_info() {
    GodotAdapter adapter;
    auto info = adapter.info();

    ASSERT(info.id == "godot-4");
    ASSERT(!info.name.empty());
    ASSERT(!info.version.empty());
    ASSERT(!info.description.empty());
    ASSERT(!info.supported_platforms.empty());
    ASSERT(info.requires_sdk);
    ASSERT(info.sdk_name == "GODOT");
    ASSERT(GodotAdapter::required_engine_version() == "4.3");

    bool has_debug = false, has_release = false;
    for (const auto& p : info.profiles) {
        if (p.name == "debug") has_debug = true;
        if (p.name == "release") has_release = true;
    }
    ASSERT(has_debug);
    ASSERT(has_release);
}

void test_godot_export_creates_files() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-godot";
    fs::create_directories(tmp_dir);
    auto output_dir = tmp_dir / "export";

    GodotAdapter adapter;
    bool ok = adapter.export_entity("TestEntity", output_dir.string(), "debug");
    ASSERT(ok);

    ASSERT(fs::exists(output_dir / "TestEntity.tres"));
    ASSERT(fs::exists(output_dir / "TestEntity_animations.tres"));
    ASSERT(fs::exists(output_dir / "TestEntity.gd"));

    {
        std::ifstream gd(output_dir / "TestEntity.gd");
        ASSERT(gd.is_open());
        std::string content((std::istreambuf_iterator<char>(gd)),
                             std::istreambuf_iterator<char>());
        ASSERT(content.find("class_name TestEntity") != std::string::npos);
    }

    cleanup_dir(tmp_dir);
}

void test_godot_validate_export() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-godot-val";
    fs::create_directories(tmp_dir);
    auto output_dir = tmp_dir / "export";

    GodotAdapter adapter;
    adapter.export_entity("ValEntity", output_dir.string(), "release");
    ASSERT(adapter.validate_export(output_dir.string()));
    ASSERT(!adapter.validate_export((tmp_dir / "nonexistent").string()));

    cleanup_dir(tmp_dir);
}

void test_godot_different_entity_names() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-godot-names";
    fs::create_directories(tmp_dir);

    GodotAdapter adapter;
    adapter.export_entity("EntityA", (tmp_dir / "A").string(), "debug");
    adapter.export_entity("EntityB", (tmp_dir / "B").string(), "release");

    ASSERT(fs::exists(tmp_dir / "A" / "EntityA.gd"));
    ASSERT(fs::exists(tmp_dir / "B" / "EntityB.gd"));
    ASSERT(!fs::exists(tmp_dir / "B" / "EntityA.gd"));

    cleanup_dir(tmp_dir);
}

// === Unity Adapter ===

void test_unity_info() {
    UnityAdapter adapter;
    auto info = adapter.info();

    ASSERT(info.id == "unity-2022");
    ASSERT(!info.name.empty());
    ASSERT(!info.supported_platforms.empty());
    ASSERT(info.requires_sdk);
    ASSERT(info.sdk_name == "UNITY");
    ASSERT(UnityAdapter::required_engine_version() == "2022.3 LTS");
}

void test_unity_export_creates_files() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-unity";
    fs::create_directories(tmp_dir);

    UnityAdapter adapter;
    bool ok = adapter.export_entity("MySprite", tmp_dir.string(), "release");
    ASSERT(ok);

    auto assets_dir = tmp_dir / "Assets" / "GSPL" / "MySprite";
    ASSERT(fs::exists(assets_dir / "MySpriteData.cs"));
    ASSERT(fs::exists(assets_dir / "MySpriteController.controller"));

    {
        std::ifstream cs(assets_dir / "MySpriteData.cs");
        ASSERT(cs.is_open());
        std::string content((std::istreambuf_iterator<char>(cs)),
                             std::istreambuf_iterator<char>());
        ASSERT(content.find("class MySpriteData") != std::string::npos);
    }

    cleanup_dir(tmp_dir);
}

void test_unity_validate_export() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-unity-val";
    fs::create_directories(tmp_dir);

    UnityAdapter adapter;
    adapter.export_entity("ValSprite", tmp_dir.string(), "debug");
    // validate_export checks for Assets/GSPL/ inside its argument directory
    ASSERT(adapter.validate_export(tmp_dir.string()));

    fs::remove_all(tmp_dir);
}

// === Unreal Adapter ===

void test_unreal_info() {
    UnrealAdapter adapter;
    auto info = adapter.info();

    ASSERT(info.id == "unreal-5");
    ASSERT(!info.name.empty());
    ASSERT(!info.supported_platforms.empty());
    ASSERT(info.requires_sdk);
    ASSERT(info.sdk_name == "UNREAL");
    ASSERT(UnrealAdapter::required_engine_version() == "5.4");
}

void test_unreal_export_creates_files() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-unreal";
    fs::create_directories(tmp_dir);

    UnrealAdapter adapter;
    bool ok = adapter.export_entity("MyCharacter", tmp_dir.string(), "debug");
    ASSERT(ok);

    auto src_dir = tmp_dir / "Plugins" / "GSPL" / "MyCharacter" / "Source" / "GSPLMyCharacter";
    ASSERT(fs::exists(src_dir / "GSPLMyCharacter.Build.cs"));
    ASSERT(fs::exists(src_dir / "MyCharacterDataAsset.h"));
    ASSERT(fs::exists(src_dir / "MyCharacterDataAsset.cpp"));
    ASSERT(fs::exists(src_dir / "GSPLMyCharacter.def"));
    ASSERT(fs::exists(tmp_dir / "Plugins" / "GSPL" / "MyCharacter" / "GSPLMyCharacter.uplugin"));

    std::string content;
    {
        std::ifstream up(tmp_dir / "Plugins" / "GSPL" / "MyCharacter" / "GSPLMyCharacter.uplugin");
        ASSERT(up.is_open());
        content.assign((std::istreambuf_iterator<char>(up)),
                        std::istreambuf_iterator<char>());
    }
    ASSERT(content.find("\"FriendlyName\": \"GSPL MyCharacter\"") != std::string::npos);

    cleanup_dir(tmp_dir);
}

void test_unreal_validate_export() {
    auto tmp_dir = fs::temp_directory_path() / "gspl-test-unreal-val";
    fs::create_directories(tmp_dir);

    UnrealAdapter adapter;
    adapter.export_entity("CharX", tmp_dir.string(), "release");
    ASSERT(adapter.validate_export(tmp_dir.string()));

    cleanup_dir(tmp_dir);
}

// === Cross-adapter consistency ===

void test_all_adapters_have_debug_release_profiles() {
    GodotAdapter godot;
    UnityAdapter unity;
    UnrealAdapter unreal;

    for (const auto& adapter_info : {godot.info(), unity.info(), unreal.info()}) {
        bool has_debug = false, has_release = false;
        for (const auto& p : adapter_info.profiles) {
            if (p.name == "debug") has_debug = true;
            if (p.name == "release") has_release = true;
        }
        ASSERT(has_debug);
        ASSERT(has_release);
    }
}

void test_all_adapters_have_unique_ids() {
    GodotAdapter godot;
    UnityAdapter unity;
    UnrealAdapter unreal;

    ASSERT(godot.info().id == "godot-4");
    ASSERT(unity.info().id == "unity-2022");
    ASSERT(unreal.info().id == "unreal-5");
    ASSERT(godot.info().id != unity.info().id);
    ASSERT(unity.info().id != unreal.info().id);
    ASSERT(godot.info().id != unreal.info().id);
}

int main() {
    std::printf("Engine Adapter Tests\n");

    TEST(test_godot_info);
    TEST(test_godot_export_creates_files);
    TEST(test_godot_validate_export);
    TEST(test_godot_different_entity_names);
    TEST(test_unity_info);
    TEST(test_unity_export_creates_files);
    TEST(test_unity_validate_export);
    TEST(test_unreal_info);
    TEST(test_unreal_export_creates_files);
    TEST(test_unreal_validate_export);
    TEST(test_all_adapters_have_debug_release_profiles);
    TEST(test_all_adapters_have_unique_ids);

    std::printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
