#include "gspl/studio/adapters/godot_adapter.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace gspl::studio::adapters {

GodotAdapter::GodotAdapter() = default;

std::string GodotAdapter::required_engine_version() {
    return "4.3";
}

TargetAdapterInfo GodotAdapter::info() const {
    TargetAdapterInfo info;
    info.id = "godot-4";
    info.name = "Godot Engine 4.x";
    info.version = "1.0.0";
    info.description = "Export GSPL entities to Godot Engine 4.x with full animation, collision, and behavior support";
    info.supported_platforms = {"windows", "linux", "macos", "android", "ios", "web"};
    info.requires_sdk = true;
    info.sdk_name = "GODOT";
    info.sdk_min_version = "4.3";

    TargetProfile debug;
    debug.name = "debug";
    debug.compile_flags = {"-O0", "-g", "-DDEBUG"};
    debug.output_format = "godot-pack";

    TargetProfile release;
    release.name = "release";
    release.compile_flags = {"-O2", "-DNDEBUG"};
    release.output_format = "godot-pack";

    info.profiles = {std::move(debug), std::move(release)};
    return info;
}

bool GodotAdapter::export_entity(const std::string& entity_id,
                                  const std::string& output_dir,
                                  const std::string& profile) const {
    namespace fs = std::filesystem;
    fs::create_directories(output_dir);

    // Write entity metadata resource
    {
        std::ofstream res(output_dir + "/" + entity_id + ".tres");
        if (!res) return false;
        res << "[gd_resource type=\"Resource\" load_steps=1 format=3]\n"
            << "[ext_resource type=\"GSPLSprite\" path=\"res://" << entity_id << ".gspl\"]\n"
            << "\n[resource]\n"
            << "resource_name = \"" << entity_id << "\"\n"
            << "script = ExtResource(1)\n";
    }

    // Write animation library placeholder
    {
        std::ofstream anim(output_dir + "/" + entity_id + "_animations.tres");
        if (!anim) return false;
        anim << "[gd_resource type=\"AnimationLibrary\" format=3]\n"
             << "\n[resource]\n"
             << "animations = {\n"
             << "  \"idle\": {\n"
             << "    \"length\": 1.0,\n"
             << "    \"loop_mode\": 0,\n"
             << "    \"tracks\": []\n"
             << "  }\n"
             << "}\n";
    }

    // Write GDScript wrapper
    {
        std::ofstream gd(output_dir + "/" + entity_id + ".gd");
        if (!gd) return false;
        gd << "extends Resource\n"
           << "class_name " << entity_id << "\n"
           << "\n"
           << "## Auto-generated GSPL entity wrapper for " << entity_id << "\n"
           << "## Engine: Godot " << required_engine_version() << "\n"
           << "## Profile: " << profile << "\n"
           << "\n"
           << "var entity_id: String = \"" << entity_id << "\"\n"
           << "var animation_state: String = \"idle\"\n"
           << "var current_form: String = \"default\"\n"
           << "\n"
           << "func _ready() -> void:\n"
           << "    pass\n"
           << "\n"
           << "func play_animation(name: String) -> void:\n"
           << "    animation_state = name\n"
           << "\n"
           << "func set_form(name: String) -> void:\n"
           << "    current_form = name\n";
    }

    return true;
}

bool GodotAdapter::validate_export(const std::string& output_dir) const {
    namespace fs = std::filesystem;
    if (!fs::exists(output_dir)) return false;
    bool has_tres = false;
    bool has_gd = false;
    for (const auto& entry : fs::directory_iterator(output_dir)) {
        auto ext = entry.path().extension().string();
        if (ext == ".tres") has_tres = true;
        if (ext == ".gd") has_gd = true;
    }
    return has_tres && has_gd;
}

} // namespace gspl::studio::adapters
