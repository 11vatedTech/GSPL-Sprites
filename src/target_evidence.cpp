#include "gspl_sprites/target_evidence.hpp"

#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace gspl::sprites {
namespace {
bool lowercase_sha256(std::string_view value) {
  return value.size() == 64 &&
         std::ranges::all_of(value, [](unsigned char character) {
           return (character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f');
         });
}

std::string read_bounded(const std::filesystem::path &path) {
  constexpr std::uint64_t maximum = 4ULL * 1024ULL * 1024ULL;
  if (!std::filesystem::is_regular_file(path) ||
      std::filesystem::is_symlink(std::filesystem::symlink_status(path)) ||
      std::filesystem::file_size(path) > maximum)
    throw std::runtime_error("target source evidence artifact is unsafe");
  std::string value(static_cast<std::size_t>(std::filesystem::file_size(path)),
                    '\0');
  std::ifstream input(path, std::ios::binary);
  input.read(value.data(), static_cast<std::streamsize>(value.size()));
  if (!input && !value.empty())
    throw std::runtime_error("target source evidence artifact read failed");
  return value;
}
} // namespace

TargetSourceEvidence::TargetSourceEvidence(
    std::string package_identity, std::string seed_identity,
    std::string authoring_provenance_sha256,
    std::string target_compatibility_sha256)
    : package_identity_(std::move(package_identity)),
      seed_identity_(std::move(seed_identity)),
      authoring_provenance_sha256_(std::move(authoring_provenance_sha256)),
      target_compatibility_sha256_(std::move(target_compatibility_sha256)) {}

const std::string &TargetSourceEvidence::package_identity() const noexcept {
  return package_identity_;
}
const std::string &TargetSourceEvidence::seed_identity() const noexcept {
  return seed_identity_;
}
const std::string &
TargetSourceEvidence::authoring_provenance_sha256() const noexcept {
  return authoring_provenance_sha256_;
}
const std::string &
TargetSourceEvidence::target_compatibility_sha256() const noexcept {
  return target_compatibility_sha256_;
}

TargetSourceEvidence
target_source_evidence_from_package(const std::filesystem::path &package_root) {
  const auto verification = verify_package(package_root);
  if (!verification.ok())
    throw std::invalid_argument(
        "target source package failed verification: " +
        verification.validation.diagnostics.front().code + ": " +
        verification.validation.diagnostics.front().message);
  return {verification.package_identity, verification.seed_identity,
          sha256(read_bounded(package_root / "authoring-provenance.json")),
          sha256(read_bounded(package_root / "target-compatibility.json"))};
}

std::string canonicalize_target_source_evidence(
    const std::optional<TargetSourceEvidence> &evidence) {
  if (!evidence)
    return "{\"sourcePackage\":null}";
  return "{\"sourcePackage\":{\"authoringProvenanceSha256\":\"" +
         evidence->authoring_provenance_sha256() + "\",\"packageIdentity\":\"" +
         evidence->package_identity() + "\",\"seedIdentity\":\"" +
         evidence->seed_identity() + "\",\"targetCompatibilitySha256\":\"" +
         evidence->target_compatibility_sha256() + "\"}}";
}

bool is_canonical_target_source_evidence(std::string_view value) noexcept {
  if (value == "{\"sourcePackage\":null}")
    return true;
  constexpr std::array<std::string_view, 5> tokens{
      "{\"sourcePackage\":{\"authoringProvenanceSha256\":\"",
      "\",\"packageIdentity\":\"", "\",\"seedIdentity\":\"",
      "\",\"targetCompatibilitySha256\":\"", "\"}}"};
  std::size_t cursor{};
  for (std::size_t index = 0; index < 4; ++index) {
    if (value.substr(cursor, tokens[index].size()) != tokens[index])
      return false;
    cursor += tokens[index].size();
    if (!lowercase_sha256(value.substr(cursor, 64)))
      return false;
    cursor += 64;
  }
  return value.substr(cursor) == tokens.back();
}

} // namespace gspl::sprites
