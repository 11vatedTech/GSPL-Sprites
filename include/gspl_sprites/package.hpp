#pragma once

#include "gspl_sprites/common.hpp"
#include "gspl_sprites/synthesis.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>

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

/* ── Living Visual Package ── */

struct PackageReadLimits {
  std::uint64_t max_artifact_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint32_t max_artifacts{4096};
  std::uint32_t max_frames{256};
};

struct LivingVisualPackageReadResult {
  std::optional<LivingVisualPackageInput> value;
  ValidationResult diagnostics;
  [[nodiscard]] bool ok() const { return value.has_value() && diagnostics.ok(); }
};

struct PackageVerificationOptions {
  bool verify_pixel_hashes{true};
  bool verify_channel_dimensions{true};
  bool strict_collision_refs{true};
};

struct LivingVisualPackageVerificationResult {
  ValidationResult validation;
  std::string package_identity;
  std::string seed_identity;
  std::uint32_t frame_count{};
  std::uint32_t clip_count{};
  std::uint32_t sample_count{};
  std::uint32_t event_count{};
  std::uint32_t channel_count{};
  std::uint32_t morphology_count{};
  [[nodiscard]] bool ok() const noexcept { return validation.ok(); }
};

void build_living_visual_package(const LivingVisualPackageInput& input,
                                 const std::filesystem::path& output);

[[nodiscard]] LivingVisualPackageReadResult read_living_visual_package(
    const std::filesystem::path& package_path,
    const PackageReadLimits& limits = {});

[[nodiscard]] LivingVisualPackageVerificationResult verify_living_visual_package(
    const std::filesystem::path& package_path,
    const PackageVerificationOptions& options = {});

} // namespace gspl::sprites
