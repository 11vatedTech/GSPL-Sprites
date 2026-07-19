#include "gspl_sprites/core.hpp"
#include "gspl_sprites/package.hpp"
#include "gspl_sprites/target_contract.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
bool has_code(const PackageVerification &value, std::string_view code) {
  return std::any_of(value.validation.diagnostics.begin(),
      value.validation.diagnostics.end(),
      [&](const auto &diagnostic) { return diagnostic.code == code; });
}
SpriteSeed fixture() {
  return parse_seed(R"(schema=gspl.sprite-seed/0.1
id=original.package-test
name=Package Test
classification=fictional
rights=ORIGINAL_USER_CREATION
entropy_root=9
primary_color=#112233
accent_color=#AABBCC
ability=arc|electric.projectile|20|4|2
)");
}
void write_text(const std::filesystem::path &path, std::string_view value) {
  std::ofstream output(path, std::ios::binary);
  output.write(value.data(), static_cast<std::streamsize>(value.size()));
  if (!output)
    throw std::runtime_error("test write failed");
}
std::string read_text(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), {}};
}
std::string replace_once(std::string value, std::string_view old_value,
                         std::string_view new_value) {
  const auto position = value.find(old_value);
  if (position == std::string::npos)
    throw std::runtime_error("test replacement source absent");
  value.replace(position, old_value.size(), new_value);
  return value;
}
FrameSource visual() {
  ImageRgba8 image{3, 3, ColorSpace::srgb, AlphaMode::straight,
                   std::vector<std::uint8_t>(36, 0)};
  for (std::uint32_t y = 1; y < 3; ++y)
    for (std::uint32_t x = 1; x < 3; ++x) {
      const auto offset = (y * 3 + x) * 4;
      image.pixels[offset] = 255;
      image.pixels[offset + 3] = 255;
    }
  return {"idle.south.000", std::move(image), 1, 2, 3};
}
} // namespace

int main() {
  const auto base =
      std::filesystem::temp_directory_path() / "gspl-sprites-package-tests";
  try {
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base);
    const auto valid = base / "valid";
    build_package(fixture(), valid);
    const auto verified = verify_package(valid);
    for (const auto &diagnostic : verified.validation.diagnostics)
      std::cerr << diagnostic.code << ": " << diagnostic.message << '\n';
    check(verified.ok(), "valid package rejected");
    check(verified.artifact_count == 9, "governance artifacts not declared");
    const auto governed = base / "governed";
    build_package(fixture(),
                  {"{\"project\":{\"id\":\"authoring.test\",\"revision\":0,"
                   "\"revisionIdentity\":\"0123456789abcdef0123456789abcdef"
                   "0123456789abcdef0123456789abcdef\"},\"references\":[]}",
                   "{\"reports\":[]}"},
                  governed);
    const auto governed_verified = verify_package(governed);
    check(governed_verified.ok() &&
              governed_verified.seed_identity == verified.seed_identity &&
              governed_verified.package_identity != verified.package_identity &&
              read_text(governed / "asset-graph.json")
                      .find("bind-authoring-provenance/1") != std::string::npos,
          "governance evidence did not participate in package identity and "
          "graph");
    const auto malformed_governance = base / "malformed-governance";
    bool malformed_governance_rejected = false;
    try {
      build_package(fixture(), {"{}", "{}"}, malformed_governance);
    } catch (const std::invalid_argument &) {
      malformed_governance_rejected = true;
    }
    check(malformed_governance_rejected &&
              !std::filesystem::exists(malformed_governance),
          "malformed governance evidence was packaged");
    const auto forged_evidence = base / "forged-evidence";
    std::filesystem::copy(valid, forged_evidence,
                          std::filesystem::copy_options::recursive);
    const auto evidence_path = forged_evidence / "authoring-provenance.json";
    const auto old_evidence = read_text(evidence_path);
    const std::string invalid_evidence = "{}";
    write_text(evidence_path, invalid_evidence);
    const auto forged_manifest =
        replace_once(read_text(forged_evidence / "manifest.json"),
                     sha256(old_evidence), sha256(invalid_evidence));
    write_text(forged_evidence / "manifest.json", forged_manifest);
    const auto forged_verification = verify_package(forged_evidence);
    check(!forged_verification.ok() &&
              has_code(forged_verification,
                       "SPRITE_PACKAGE_AUTHORING_PROVENANCE_INVALID"),
          "hash-consistent malformed authoring evidence passed verification");
    const auto forged_graph = base / "forged-graph";
    std::filesystem::copy(valid, forged_graph,
                          std::filesystem::copy_options::recursive);
    const auto graph_path = forged_graph / "asset-graph.json";
    const auto old_graph = read_text(graph_path);
    const auto hash_field = old_graph.find("\"contentHash\":\"");
    check(hash_field != std::string::npos,
          "graph fixture content hash missing");
    const auto hash_start = hash_field + 15;
    check(hash_start + 64 <= old_graph.size(), "graph fixture hash truncated");
    auto invalid_graph = old_graph;
    invalid_graph.replace(hash_start, 64, 64, '0');
    write_text(graph_path, invalid_graph);
    write_text(forged_graph / "manifest.json",
               replace_once(read_text(forged_graph / "manifest.json"),
                            sha256(old_graph), sha256(invalid_graph)));
    const auto graph_verification = verify_package(forged_graph);
    check(!graph_verification.ok() &&
              has_code(graph_verification,
                       "SPRITE_PACKAGE_GRAPH_IDENTITY_MISMATCH"),
          "hash-consistent forged graph identity passed verification");
    const auto forged_target = base / "forged-target";
    std::filesystem::copy(valid, forged_target,
                          std::filesystem::copy_options::recursive);
    const auto requirement_path =
        forged_target / "package-target-requirements.json";
    const auto report_path = forged_target / "package-target-report.json";
    const auto old_requirements = read_text(requirement_path);
    const auto old_report = read_text(report_path);
    const std::array forged_requirements{
        TargetRequirement{TargetFeature::canonical_seed, true},
        TargetRequirement{TargetFeature::rights_and_provenance, true},
        TargetRequirement{TargetFeature::raster_2d, true}};
    const auto new_requirements =
        canonicalize_target_requirements(forged_requirements);
    const auto new_report =
        canonicalize_target_compatibility(evaluate_target_compatibility(
            builtin_target_adapter("portable-package"), forged_requirements));
    write_text(requirement_path, new_requirements);
    write_text(report_path, new_report);
    auto target_manifest = read_text(forged_target / "manifest.json");
    target_manifest = replace_once(target_manifest, sha256(old_requirements),
                                   sha256(new_requirements));
    target_manifest =
        replace_once(target_manifest, sha256(old_report), sha256(new_report));
    write_text(forged_target / "manifest.json", target_manifest);
    const auto target_verification = verify_package(forged_target);
    check(!target_verification.ok() &&
              has_code(target_verification,
                       "SPRITE_PACKAGE_TARGET_STRUCTURE_MISSING"),
          "hash-consistent unsupported package feature claim was accepted");
    const auto visual_package = base / "visual";
    const std::array visual_frames{visual()};
    build_package(fixture(), visual_frames, {16, 16, 1, true, 0},
                  visual_package);
    const auto visual_verified = verify_package(visual_package);
    check(visual_verified.ok() && visual_verified.artifact_count == 13,
          "visual package rejected or incomplete");
    check(std::filesystem::exists(visual_package / "assets" /
                                  "sprite-atlas.png") &&
              std::filesystem::exists(visual_package / "assets" /
                                      "sprite-alpha-mask.png") &&
              std::filesystem::exists(visual_package / "assets" /
                                      "sprite-outline-mask.png") &&
              std::filesystem::exists(visual_package / "atlas.json"),
          "visual package artifacts missing");
    const auto atlas_png =
        read_text(visual_package / "assets" / "sprite-atlas.png");
    const auto decoded = decode_png(std::as_bytes(std::span(atlas_png)));
    check(decoded.width == 16 && decoded.height == 16,
          "packaged atlas is not decodable");
    check(
        read_text(visual_package / "provenance.json").find("ingest-frame/1") !=
                std::string::npos &&
            read_text(visual_package / "asset-graph.json")
                    .find("compile-atlas/1") != std::string::npos,
        "visual provenance or dependency graph missing");
    const auto visual_changed = base / "visual-changed";
    auto changed_frame = visual();
    changed_frame.image.pixels[(1 * 3 + 1) * 4] = 0;
    const std::array changed_frames{changed_frame};
    build_package(fixture(), changed_frames, {16, 16, 1, true, 0},
                  visual_changed);
    const auto changed_verified = verify_package(visual_changed);
    check(changed_verified.ok() &&
              visual_verified.seed_identity == changed_verified.seed_identity &&
              visual_verified.package_identity !=
                  changed_verified.package_identity,
          "visual source change did not preserve seed identity and alter "
          "package identity");
    const auto failed_visual = base / "failed-visual";
    ImageRgba8 empty{1, 1, ColorSpace::srgb, AlphaMode::straight, {0, 0, 0, 0}};
    const std::array empty_frames{
        FrameSource{"idle.empty.000", empty, 0, 0, 1}};
    bool visual_failure = false;
    try {
      build_package(fixture(), empty_frames, {16, 16, 1, true, 0},
                    failed_visual);
    } catch (const std::invalid_argument &) {
      visual_failure = true;
    }
    check(visual_failure && !std::filesystem::exists(failed_visual) &&
              !std::filesystem::exists(failed_visual.string() + ".staging"),
          "failed visual package was published or left staging state");
    auto tiny = PackageLimits{};
    tiny.max_total_bytes = 1;
    const auto limited = verify_package(valid, tiny);
    check(!limited.ok() && has_code(limited, "SPRITE_PACKAGE_TOTAL_LIMIT"),
          "total byte limit ignored");
    write_text(valid / "undeclared.txt", "x");
    const auto undeclared = verify_package(valid);
    check(!undeclared.ok() &&
              has_code(undeclared, "SPRITE_PACKAGE_FILE_SET_MISMATCH"),
          "undeclared file accepted");
    std::filesystem::remove(valid / "undeclared.txt");
    {
      std::ofstream tamper(valid / "assets" / "entity.svg",
                           std::ios::binary | std::ios::app);
      tamper << "x";
    }
    const auto changed = verify_package(valid);
    check(!changed.ok() && has_code(changed, "SPRITE_PACKAGE_HASH_MISMATCH"),
          "tampered artifact accepted");

    const auto traversal = base / "traversal";
    std::filesystem::create_directories(traversal);
    write_text(traversal / "manifest.json",
               "{\"artifacts\":[{\"path\":\"../"
               "escape\",\"sha256\":"
               "\"0000000000000000000000000000000000000000000000000000000000000"
               "000\"}],\"assetGraph\":\"asset-graph.json\",\"entityId\":\"x\","
               "\"format\":\"gspl.sprite-package/"
               "0.1\",\"provenance\":\"provenance.json\",\"rights\":\"rights."
               "json\",\"seedIdentity\":"
               "\"0000000000000000000000000000000000000000000000000000000000000"
               "000\"}");
    const auto escaped = verify_package(traversal);
    check(!escaped.ok() && has_code(escaped, "SPRITE_PACKAGE_PATH_UNSAFE"),
          "traversal path accepted");
    const auto noncanonical = base / "noncanonical";
    std::filesystem::create_directories(noncanonical);
    write_text(noncanonical / "manifest.json", "{}");
    const auto malformed = verify_package(noncanonical);
    check(!malformed.ok() && has_code(malformed, "SPRITE_PACKAGE_MALFORMED"),
          "noncanonical manifest accepted");
    std::filesystem::remove_all(base);
    std::cout << "all gspl sprites package tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::filesystem::remove_all(base);
    std::cerr << error.what() << '\n';
    return 1;
  }
}
