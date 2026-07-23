#include "gspl/studio/project.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gspl::studio {

namespace {

constexpr auto manifest_filename = "gspl-project.jsonc";

[[nodiscard]] auto read_file(const std::filesystem::path& path) -> std::string {
    std::ifstream file(path);
    if (!file) {
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

[[nodiscard]] auto write_file(const std::filesystem::path& path, std::string_view content) -> bool {
    std::ofstream file(path);
    if (!file) {
        return false;
    }
    file << content;
    return file.good();
}

[[nodiscard]] auto parse_jsonc_field(std::string_view json, std::string_view key) -> std::string {
    auto key_pos = json.find(key);
    if (key_pos == std::string_view::npos) {
        return {};
    }
    auto colon = json.find(':', key_pos + key.size());
    if (colon == std::string_view::npos) {
        return {};
    }
    auto start = json.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string_view::npos) {
        return {};
    }
    if (json[start] == '"') {
        auto end = json.find('"', start + 1);
        if (end == std::string_view::npos) {
            return {};
        }
        return std::string(json.substr(start + 1, end - start - 1));
    }
    auto end = json.find_first_of(",\n\r}", start);
    if (end == std::string_view::npos) {
        end = json.size();
    }
    auto val = json.substr(start, end - start);
    while (!val.empty() && (val.back() == ' ' || val.back() == '\t' || val.back() == '\r' || val.back() == '\n')) {
        val.remove_suffix(1);
    }
    return std::string(val);
}

[[nodiscard]] auto parse_jsonc_array(std::string_view json, std::string_view key) -> std::vector<std::string> {
    std::vector<std::string> result;
    auto key_pos = json.find(key);
    if (key_pos == std::string_view::npos) {
        return result;
    }
    auto colon = json.find(':', key_pos + key.size());
    if (colon == std::string_view::npos) {
        return result;
    }
    auto bracket = json.find('[', colon);
    if (bracket == std::string_view::npos) {
        return result;
    }
    auto close = json.find(']', bracket);
    if (close == std::string_view::npos) {
        return result;
    }
    auto inner = json.substr(bracket + 1, close - bracket - 1);
    auto pos = inner.find('"');
    while (pos != std::string_view::npos) {
        auto end = inner.find('"', pos + 1);
        if (end == std::string_view::npos) {
            break;
        }
        result.emplace_back(inner.substr(pos + 1, end - pos - 1));
        pos = inner.find('"', end + 1);
    }
    return result;
}

[[nodiscard]] auto serialize_manifest(const ProjectManifest& m) -> std::string {
    std::string result = "{\n";
    result += "    \"name\": \"" + m.name + "\",\n";
    result += "    \"version\": \"" + m.version + "\",\n";
    result += "    \"description\": \"" + m.description + "\",\n";

    result += "    \"authors\": [\n";
    for (size_t i = 0; i < m.authors.size(); ++i) {
        result += "        \"" + m.authors[i] + "\"";
        if (i + 1 < m.authors.size()) {
            result += ",";
        }
        result += "\n";
    }
    result += "    ],\n";

    result += "    \"dependencies\": [\n";
    for (size_t i = 0; i < m.dependencies.size(); ++i) {
        result += "        \"" + m.dependencies[i] + "\"";
        if (i + 1 < m.dependencies.size()) {
            result += ",";
        }
        result += "\n";
    }
    result += "    ],\n";

    result += "    \"gspl_version\": \"" + m.gspl_version + "\"\n";
    result += "}\n";
    return result;
}

} // anonymous namespace

Project::Project(std::filesystem::path root_dir)
    : root_(std::move(root_dir))
{
}

auto Project::load() -> bool {
    auto path = manifest_path();
    auto content = read_file(path);
    if (content.empty()) {
        return false;
    }

    auto name = parse_jsonc_field(content, "\"name\"");
    auto version = parse_jsonc_field(content, "\"version\"");
    if (name.empty() || version.empty()) {
        return false;
    }

    manifest_.name = std::move(name);
    manifest_.version = std::move(version);
    manifest_.description = parse_jsonc_field(content, "\"description\"");
    manifest_.authors = parse_jsonc_array(content, "\"authors\"");
    manifest_.dependencies = parse_jsonc_array(content, "\"dependencies\"");
    manifest_.gspl_version = parse_jsonc_field(content, "\"gspl_version\"");

    return true;
}

auto Project::save() const -> bool {
    auto path = manifest_path();
    auto content = serialize_manifest(manifest_);
    return write_file(path, content);
}

auto Project::root() const -> const std::filesystem::path& {
    return root_;
}

auto Project::manifest() const -> const ProjectManifest& {
    return manifest_;
}

auto Project::manifest_path() const -> std::filesystem::path {
    return root_ / manifest_filename;
}

auto Project::source_dir() const -> std::filesystem::path {
    return root_ / "src";
}

auto Project::artifact_dir() const -> std::filesystem::path {
    return root_ / "artifacts";
}

auto Project::trace_dir() const -> std::filesystem::path {
    return root_ / "traces";
}

auto Project::package_dir() const -> std::filesystem::path {
    return root_ / "packages";
}

void Project::set_manifest(ProjectManifest m) {
    manifest_ = std::move(m);
}

auto Project::is_valid_project_dir(const std::filesystem::path& dir) -> bool {
    return std::filesystem::exists(dir / manifest_filename);
}

auto Project::create_at(const std::filesystem::path& dir, ProjectManifest m) -> Project {
    std::filesystem::create_directories(dir);
    std::filesystem::create_directories(dir / "src");
    std::filesystem::create_directories(dir / "artifacts");
    std::filesystem::create_directories(dir / "traces");
    std::filesystem::create_directories(dir / "packages");

    Project project(dir);
    project.set_manifest(std::move(m));
    project.save();
    return project;
}

} // namespace gspl::studio
