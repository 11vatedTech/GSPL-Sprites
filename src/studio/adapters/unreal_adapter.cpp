#include "gspl/studio/adapters/unreal_adapter.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace gspl::studio::adapters {

UnrealAdapter::UnrealAdapter() = default;

std::string UnrealAdapter::required_engine_version() {
    return "5.4";
}

TargetAdapterInfo UnrealAdapter::info() const {
    TargetAdapterInfo info;
    info.id = "unreal-5";
    info.name = "Unreal Engine 5.x";
    info.version = "1.0.0";
    info.description = "Export GSPL entities to Unreal Engine 5 with DataAsset, animation blueprint, and skeletal mesh support";
    info.supported_platforms = {"windows", "linux", "macos", "android", "ios"};
    info.requires_sdk = true;
    info.sdk_name = "UNREAL";
    info.sdk_min_version = "5.4";

    TargetProfile debug;
    debug.name = "debug";
    debug.compile_flags = {"-O0", "-g", "-DDEBUG"};
    debug.output_format = "unreal-plugin";

    TargetProfile release;
    release.name = "release";
    release.compile_flags = {"-O2", "-DNDEBUG"};
    release.output_format = "unreal-plugin";

    info.profiles = {std::move(debug), std::move(release)};
    return info;
}

bool UnrealAdapter::export_entity(const std::string& entity_id,
                                   const std::string& output_dir,
                                   const std::string& profile) const {
    namespace fs = std::filesystem;
    auto plugin_dir = output_dir + "/Plugins/GSPL/" + entity_id;
    auto src_dir = plugin_dir + "/Source/GSPL" + entity_id;
    fs::create_directories(src_dir);

    // Write Build.cs
    {
        std::ofstream build(src_dir + "/GSPL" + entity_id + ".Build.cs");
        if (!build) return false;
        build << "using UnrealBuildTool;\n\n"
              << "public class GSPL" << entity_id << " : ModuleRules\n"
              << "{\n"
              << "    public GSPL" << entity_id << "(ReadOnlyTargetRules Target) : base(Target)\n"
              << "    {\n"
              << "        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;\n"
              << "        PublicDependencyModuleNames.AddRange(new string[] {\n"
              << "            \"Core\", \"CoreUObject\", \"Engine\", \"InputCore\"\n"
              << "        });\n"
              << "    }\n"
              << "}\n";
    }

    // Write DataAsset header
    {
        std::ofstream h(src_dir + "/" + entity_id + "DataAsset.h");
        if (!h) return false;
        h << "#pragma once\n\n"
          << "#include \"CoreMinimal.h\"\n"
          << "#include \"Engine/DataAsset.h\"\n"
          << "#include \"" << entity_id << "DataAsset.generated.h\"\n\n"
          << "UCLASS(BlueprintType)\n"
          << "class GSPL" << entity_id << "_API U" << entity_id << "DataAsset : public UDataAsset\n"
          << "{\n"
          << "    GENERATED_BODY()\n"
          << "public:\n"
          << "    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = \"GSPL\")\n"
          << "    FString EntityId = \"" << entity_id << "\";\n"
          << "    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = \"GSPL\")\n"
          << "    FString CurrentForm = TEXT(\"default\");\n"
          << "    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = \"GSPL\")\n"
          << "    float AnimationTime = 0.0f;\n"
          << "};\n";
    }

    // Write DataAsset implementation
    {
        std::ofstream cpp(src_dir + "/" + entity_id + "DataAsset.cpp");
        if (!cpp) return false;
        cpp << "#include \"" << entity_id << "DataAsset.h\"\n";
    }

    // Write module definition
    {
        std::ofstream def(src_dir + "/GSPL" + entity_id + ".def");
        if (!def) return false;
        def << "; Profile: " << profile << "\n"
            << "LIBRARY GSPL" << entity_id << "\n"
            << "EXPORTS\n"
            << "  Init" << entity_id << "\n";
    }

    // Write plugin descriptor
    {
        std::ofstream uplugin(plugin_dir + "/GSPL" + entity_id + ".uplugin");
        if (!uplugin) return false;
        uplugin << "{\n"
                << "    \"FileVersion\": 3,\n"
                << "    \"VersionName\": \"1.0\",\n"
                << "    \"FriendlyName\": \"GSPL " << entity_id << "\",\n"
                << "    \"Description\": \"GSPL entity export for " << entity_id << " (" << profile << ")\",\n"
                << "    \"Category\": \"GSPL\",\n"
                << "    \"CreatedBy\": \"GSPL Studio\",\n"
                << "    \"EngineVersion\": \"" << required_engine_version() << ".0\",\n"
                << "    \"Modules\": [\n"
                << "        {\n"
                << "            \"Name\": \"GSPL" << entity_id << "\",\n"
                << "            \"Type\": \"Runtime\",\n"
                << "            \"LoadingPhase\": \"Default\"\n"
                << "        }\n"
                << "    ]\n"
                << "}\n";
    }

    return true;
}

bool UnrealAdapter::validate_export(const std::string& output_dir) const {
    namespace fs = std::filesystem;
    if (!fs::exists(output_dir + "/Plugins/GSPL")) return false;
    for (const auto& entry : fs::recursive_directory_iterator(output_dir + "/Plugins/GSPL")) {
        if (entry.path().extension() == ".uplugin") return true;
    }
    return false;
}

} // namespace gspl::studio::adapters
