#include "gspl_sprites/authoring_cli.hpp"

#include "gspl_sprites/authoring.hpp"
#include "gspl_sprites/package.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace gspl::sprites {
namespace {
std::string read_seed_source(const std::filesystem::path &path) {
  if (!std::filesystem::is_regular_file(path) ||
      std::filesystem::is_symlink(std::filesystem::symlink_status(path)) ||
      std::filesystem::file_size(path) > 1024ULL * 1024ULL)
    throw std::runtime_error(
        "seed import path is absent, unsafe, or oversized");
  std::ifstream input(path, std::ios::binary);
  const std::string source{std::istreambuf_iterator<char>(input), {}};
  if (input.bad())
    throw std::runtime_error("seed import read failed");
  return source;
}

void publish_text(std::string_view value, const std::filesystem::path &path) {
  if (path.empty() || std::filesystem::exists(path))
    throw std::runtime_error(
        "publication destination is empty or already exists");
  auto staging = path;
  staging += ".staging";
  if (std::filesystem::exists(staging))
    throw std::runtime_error("publication staging path already exists");
  try {
    std::ofstream file(staging, std::ios::binary | std::ios::trunc);
    file.write(value.data(), static_cast<std::streamsize>(value.size()));
    if (!file)
      throw std::runtime_error("publication staging write failed");
    file.close();
    std::ifstream verification(staging, std::ios::binary);
    const std::string published{std::istreambuf_iterator<char>(verification),
                                {}};
    if (verification.bad() || published != value)
      throw std::runtime_error("publication staging verification failed");
    verification.close();
    std::filesystem::rename(staging, path);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(staging, ignored);
    throw;
  }
}

void build_verified_package(const SpriteSeed &seed,
                            const std::filesystem::path &path) {
  if (path.empty() || std::filesystem::exists(path))
    throw std::runtime_error("package destination is empty or already exists");
  auto staging = path;
  staging += ".authoring-cli.staging";
  if (std::filesystem::exists(staging))
    throw std::runtime_error("package staging path already exists");
  try {
    build_package(seed, staging);
    const auto verification = verify_package(staging);
    if (!verification.ok())
      throw std::runtime_error(
          "authoring package failed independent verification: " +
          verification.validation.diagnostics.front().message);
    std::filesystem::rename(staging, path);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove_all(staging, ignored);
    auto compiler_staging = staging;
    compiler_staging += ".staging";
    std::filesystem::remove_all(compiler_staging, ignored);
    throw;
  }
}

std::optional<std::string_view>
variant_argument(std::span<const std::string_view> arguments,
                 std::size_t index) {
  if (arguments.size() == index)
    return {};
  if (arguments.size() != index + 1 || arguments[index].empty())
    throw std::invalid_argument(
        "authoring command has invalid variant arguments");
  return arguments[index];
}

AuthoringLoweringResult lower(std::span<const std::string_view> arguments,
                              std::size_t variant_index) {
  const auto project = load_authoring_project(arguments[1]);
  return lower_authoring_project(project,
                                 variant_argument(arguments, variant_index));
}

void require_lowered(const AuthoringLoweringResult &result) {
  if (!result.ok())
    throw std::invalid_argument(result.validation.diagnostics.front().code +
                                ": " +
                                result.validation.diagnostics.front().message);
}
} // namespace

std::optional<int>
run_authoring_cli(std::span<const std::string_view> arguments,
                  std::ostream &output, std::ostream &error) {
  if (arguments.empty() || !arguments.front().starts_with("authoring-"))
    return {};
  try {
    if (arguments[0] == "authoring-import-seed") {
      if (arguments.size() != 5)
        throw std::invalid_argument(
            "usage: authoring-import-seed <seed.sprite> <project-id> <intent> "
            "<output>");
      const auto seed = parse_seed(read_seed_source(arguments[1]));
      const auto project = authoring_project_from_seed(
          seed, std::string(arguments[2]), std::string(arguments[3]));
      save_authoring_project(project, arguments[4]);
      output << "imported " << project.id
             << " identity=" << authoring_revision_identity(project) << '\n';
      return 0;
    }
    if (arguments[0] == "authoring-inspect") {
      if (arguments.size() != 2)
        throw std::invalid_argument("usage: authoring-inspect <project>");
      const auto project = load_authoring_project(arguments[1]);
      output << "{\"identity\":\"" << authoring_revision_identity(project)
             << "\",\"project\":" << canonicalize_authoring_project(project)
             << "}\n";
      return 0;
    }
    if (arguments[0] == "authoring-revise") {
      if (arguments.size() < 7 || (arguments.size() - 4) % 3 != 0)
        throw std::invalid_argument(
            "usage: authoring-revise <project> <expected-identity> <output> "
            "<path> <selected-or-keep> <lock|unlock|keep> [...]");
      const auto project = load_authoring_project(arguments[1]);
      std::vector<AuthoringEdit> edits;
      for (std::size_t index = 4; index < arguments.size(); index += 3) {
        AuthoringEdit edit{std::string(arguments[index]), {}, {}};
        if (arguments[index + 1] != "keep")
          edit.selected_value = std::string(arguments[index + 1]);
        if (arguments[index + 2] == "lock")
          edit.locked = true;
        else if (arguments[index + 2] == "unlock")
          edit.locked = false;
        else if (arguments[index + 2] != "keep")
          throw std::invalid_argument("authoring lock operation is invalid");
        edits.push_back(std::move(edit));
      }
      const auto revised = revise_authoring_project(
          project, arguments[2], std::span<const AuthoringEdit>{edits});
      save_authoring_project(revised, arguments[3]);
      output << "revised " << revised.id << " revision=" << revised.revision
             << " identity=" << authoring_revision_identity(revised) << '\n';
      return 0;
    }
    if (arguments[0] == "authoring-lower") {
      if (arguments.size() < 3 || arguments.size() > 4)
        throw std::invalid_argument(
            "usage: authoring-lower <project> <output-seed-json> [variant]");
      const auto result = lower(arguments, 3);
      require_lowered(result);
      const auto canonical = canonicalize(*result.seed);
      publish_text(canonical, arguments[2]);
      output << "lowered " << result.seed->stable_id
             << " seed=" << sha256(canonical) << '\n';
      return 0;
    }
    if (arguments[0] == "authoring-build") {
      if (arguments.size() < 3 || arguments.size() > 4)
        throw std::invalid_argument(
            "usage: authoring-build <project> <output-package> [variant]");
      const auto result = lower(arguments, 3);
      require_lowered(result);
      build_verified_package(*result.seed, arguments[2]);
      output << "built authoring project " << result.seed->stable_id << '\n';
      return 0;
    }
    throw std::invalid_argument("unknown authoring command");
  } catch (const std::exception &exception) {
    error << "GSPL_SPRITES_AUTHORING: " << exception.what() << '\n';
    return 1;
  }
}

} // namespace gspl::sprites
