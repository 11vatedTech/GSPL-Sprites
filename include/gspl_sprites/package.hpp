#pragma once

#include "gspl_sprites/common.hpp"

#include <cstdint>
#include <filesystem>

namespace gspl::sprites {

struct PackageLimits {
  std::uint64_t max_manifest_bytes{4ULL * 1024ULL * 1024ULL};
  std::uint64_t max_artifact_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_bytes{2ULL * 1024ULL * 1024ULL * 1024ULL};
  std::uint32_t max_artifacts{4096};
  std::uint32_t max_directory_entries{8192};
  std::uint32_t max_path_bytes{1024};
};

struct PackageVerification {
  ValidationResult validation;
  std::string entity_id;
  std::string seed_identity;
  std::string package_identity;
  std::uint32_t artifact_count{};
  std::uint64_t total_artifact_bytes{};
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

[[nodiscard]] PackageVerification verify_package(const std::filesystem::path& root,
                                                 const PackageLimits& limits = {});

} // namespace gspl::sprites
