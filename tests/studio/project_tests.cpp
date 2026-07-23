#include "gspl/studio/project.hpp"
#include <cassert>
#include <cstdio>
#include <filesystem>

void test_create_and_load_project() {
    auto tmp = std::filesystem::temp_directory_path() / "gspl_test_project";
    std::filesystem::remove_all(tmp);

    gspl::studio::ProjectManifest m;
    m.name = "test-project";
    m.version = "0.1.0";
    m.description = "Test project";
    m.authors = {"tester"};
    m.gspl_version = "1.0";

    auto proj = gspl::studio::Project::create_at(tmp, m);
    assert(proj.save());

    assert(gspl::studio::Project::is_valid_project_dir(tmp));

    gspl::studio::Project loaded(tmp);
    assert(loaded.load());
    assert(loaded.manifest().name == "test-project");
    assert(loaded.manifest().version == "0.1.0");
    assert(loaded.manifest().gspl_version == "1.0");
    assert(loaded.source_dir() == tmp / "src");
    assert(loaded.artifact_dir() == tmp / "artifacts");

    std::filesystem::remove_all(tmp);
}

void test_invalid_project_dir() {
    auto tmp = std::filesystem::temp_directory_path() / "gspl_test_not_a_project";
    std::filesystem::remove_all(tmp);
    std::filesystem::create_directories(tmp);

    assert(!gspl::studio::Project::is_valid_project_dir(tmp));
    std::filesystem::remove_all(tmp);
}

int main() {
    test_create_and_load_project();
    test_invalid_project_dir();
    std::printf("All project tests passed.\n");
    return 0;
}
