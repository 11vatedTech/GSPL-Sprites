#include "gspl/studio/adapters/unity_adapter.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace gspl::studio::adapters {

UnityAdapter::UnityAdapter() = default;

std::string UnityAdapter::required_engine_version() {
    return "2022.3 LTS";
}

TargetAdapterInfo UnityAdapter::info() const {
    TargetAdapterInfo info;
    info.id = "unity-2022";
    info.name = "Unity Engine 2022 LTS";
    info.version = "1.0.0";
    info.description = "Export GSPL entities to Unity Engine with ScriptableObject assets, animation controllers, and prefabs";
    info.supported_platforms = {"windows", "linux", "macos", "android", "ios", "webgl"};
    info.requires_sdk = true;
    info.sdk_name = "UNITY";
    info.sdk_min_version = "2022.3";

    TargetProfile debug;
    debug.name = "debug";
    debug.compile_flags = {"-O0", "-g", "-DDEBUG"};
    debug.output_format = "unity-assets";

    TargetProfile release;
    release.name = "release";
    release.compile_flags = {"-O2", "-DNDEBUG"};
    release.output_format = "unity-assets";

    info.profiles = {std::move(debug), std::move(release)};
    return info;
}

bool UnityAdapter::export_entity(const std::string& entity_id,
                                  const std::string& output_dir,
                                  const std::string& profile) const {
    namespace fs = std::filesystem;
    auto assets_dir = output_dir + "/Assets/GSPL/" + entity_id;
    fs::create_directories(assets_dir);

    // Write ScriptableObject definition
    {
        std::ofstream cs(assets_dir + "/" + entity_id + "Data.cs");
        if (!cs) return false;
        cs << "using UnityEngine;\n"
           << "using System.Collections.Generic;\n"
           << "\n"
           << "[CreateAssetMenu(fileName = \"" << entity_id << "\", menuName = \"GSPL/" << entity_id << "\")]\n"
           << "public class " << entity_id << "Data : ScriptableObject\n"
           << "{\n"
           << "    public string entityId = \"" << entity_id << "\";\n"
           << "    public string currentForm = \"default\";\n"
           << "    public string currentAnimation = \"idle\";\n"
           << "    public float animationTime = 0f;\n"
           << "    public List<string> availableForms = new List<string>();\n"
           << "    public List<string> availableAnimations = new List<string>();\n"
           << "}\n";
    }

    // Write AnimationController
    {
        std::ofstream ac(assets_dir + "/" + entity_id + "Controller.controller");
        if (!ac) return false;
        ac << "%YAML 1.1\n%TAG !u! tag:unity3d.com,2011:\n"
           << "--- !u!91 &9100000\n"
           << "AnimatorController:\n"
           << "  m_Name: " << entity_id << "Controller\n"
           << "  m_Animations: []\n";
    }

    // Write README
    {
        std::ofstream readme(assets_dir + "/README.md");
        if (!readme) return false;
        readme << "# " << entity_id << " — GSPL Unity Export\n\n"
               << "Engine: Unity " << required_engine_version() << "\n"
               << "Profile: " << profile << "\n\n"
               << "## Import Instructions\n\n"
               << "1. Copy `Assets/GSPL/" << entity_id << "/` into your Unity project's `Assets/` folder\n"
               << "2. The ScriptableObject `" << entity_id << "Data` can be created via Assets > Create > GSPL\n"
               << "3. Attach the generated prefab components to your game objects\n";
    }

    return true;
}

bool UnityAdapter::validate_export(const std::string& output_dir) const {
    namespace fs = std::filesystem;
    auto assets_dir = output_dir + "/Assets/GSPL";
    if (!fs::exists(assets_dir)) return false;
    for (const auto& entry : fs::recursive_directory_iterator(assets_dir)) {
        if (entry.path().extension() == ".cs") return true;
    }
    return false;
}

} // namespace gspl::studio::adapters
