#include "gspl_sprites/core.hpp"
#include "gspl_sprites/gltf_export.hpp"
#include "gspl_sprites/gltf_verify.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
std::uint32_t u32(const std::vector<std::byte> &b, std::size_t o) {
  std::uint32_t v{};
  std::memcpy(&v, b.data() + o, 4);
  return v;
}
Projection3dDefinition fixture() {
  Projection3dDefinition p;
  p.id = "glb.entity";
  p.materials = {{"mat", 0xffffffffU, 0, 1000000, MaterialAlphaMode::opaque, 500000, false, {}, {}, {}}};
  p.skeleton = Skeleton3d{"rig", {{"root", {}, {0,0,0}}}};
  Vertex3d a{{0, 0, 0}, {0, 0, 1'000'000}, {0, 0}, {{"root", 1'000'000}}},
      b = a, c = a;
  b.position.x = 1'000'000;
  c.position.y = 1'000'000;
  b.texture_coordinate.u = 1'000'000;
  c.texture_coordinate.v = 1'000'000;
  p.meshes = {
      {"triangle", MeshPurpose::render, "mat", false, {a, b, c}, {0, 1, 2}}};
  p.morph_targets = {
      {"lower", "triangle", {{0, 0, 0}, {0, 0, 0}, {0, 0, -50'000}}},
      {"raise", "triangle", {{0, 0, 0}, {0, 0, 0}, {0, 0, 100'000}}}};
  return p;
}
AnimationClip3d animation() {
  return {"idle",
          60,
          10,
          true,
          {{"root",
            {{0, {}},
             {10,
              {{100'000, 0, 0},
               {0, 0, 0, 1'000'000},
               {1'000'000, 1'000'000, 1'000'000}}}}}},
          {{"lower", {{0, 0}, {3, 500'000}, {10, 0}}},
           {"raise", {{0, 0}, {5, 1'000'000}, {10, 0}}}},
          {{"cycle", 10}}};
}
Projection3dDefinition lod_fixture() {
  Projection3dDefinition p;
  p.id = "glb.lod";
  p.materials = {{"mat", 0xffffffffU, 0, 1000000, MaterialAlphaMode::opaque, 500000, false, {}, {}, {}}};
  Vertex3d a{{0, 0, 0}, {0, 0, 1'000'000}, {0, 0}, {}},
      b{{1'000'000, 0, 0}, {0, 0, 1'000'000}, {0, 0}, {}},
      c{{1'000'000, 1'000'000, 0}, {0, 0, 1'000'000}, {0, 0}, {}},
      d{{0, 1'000'000, 0}, {0, 0, 1'000'000}, {0, 0}, {}};
  p.meshes = {
      {"high",
       MeshPurpose::render,
       "mat",
       false,
       {a, b, c, d},
       {0, 1, 2, 0, 2, 3}},
      {"low", MeshPurpose::render, "mat", false, {a, b, c, d}, {0, 1, 2}}};
  p.lods = {{0, "high", 800'000}, {1, "low", 100'000}};
  return p;
}
SpriteSeed seed_fixture() {
  return parse_seed(R"(schema=gspl.sprite-seed/0.1
id=original.glb-source
name=GLB Source
classification=fictional
rights=ORIGINAL_USER_CREATION
entropy_root=53
primary_color=#334455
accent_color=#CCDDEE
ability=arc|electric.projectile|20|4|2
)");
}
} // namespace
int main(int argc, char **argv) try {
  const auto glb = export_projection3d_glb(fixture());
  const auto source_package = std::filesystem::temp_directory_path() /
                              "gspl-sprites-glb-source-package";
  std::filesystem::remove_all(source_package);
  build_package(seed_fixture(), source_package);
  const auto source = target_source_evidence_from_package(source_package);
  const auto governed_glb =
      export_projection3d_glb(fixture(), std::span<const AnimationClip3d>{},
                              std::span<const GltfTextureAsset>{}, {}, source);
  const auto governed_json_length = u32(governed_glb, 12);
  const std::string governed_json(
      reinterpret_cast<const char *>(governed_glb.data() + 20),
      governed_json_length);
  check(governed_json.find(source.package_identity()) != std::string::npos &&
            governed_json.find("gsplSourceEvidence") != std::string::npos,
        "GLB did not preserve verified source package evidence");
  const auto governed_verification = verify_projection3d_glb(governed_glb);
  check(governed_verification.ok() &&
            governed_verification.glb_identity.size() == 64,
        "standalone GLB verifier rejected governed output");
  auto forged_report = governed_glb;
  const std::string_view original_adapter{"glb-2.0"};
  const auto adapter =
      std::search(forged_report.begin(), forged_report.end(),
                  reinterpret_cast<const std::byte *>(original_adapter.data()),
                  reinterpret_cast<const std::byte *>(original_adapter.data() +
                                                      original_adapter.size()));
  check(adapter != forged_report.end(), "GLB target report fixture is absent");
  const std::string_view forged_adapter{"bad-2.0"};
  std::copy(reinterpret_cast<const std::byte *>(forged_adapter.data()),
            reinterpret_cast<const std::byte *>(forged_adapter.data() +
                                                forged_adapter.size()),
            adapter);
  check(!verify_projection3d_glb(forged_report).ok(),
        "forged embedded GLB target report was accepted");
  auto missing_mesh_claim = governed_glb;
  const std::string_view mesh_key{"\"meshes\":["};
  const auto mesh = std::search(
      missing_mesh_claim.begin(), missing_mesh_claim.end(),
      reinterpret_cast<const std::byte *>(mesh_key.data()),
      reinterpret_cast<const std::byte *>(mesh_key.data() + mesh_key.size()));
  check(mesh != missing_mesh_claim.end(),
        "GLB mesh structure fixture is absent");
  *(mesh + 2) = static_cast<std::byte>('a');
  check(!verify_projection3d_glb(missing_mesh_claim).ok(),
        "GLB verifier accepted a missing required mesh structure");
  if (argc == 3 && std::string_view(argv[1]) == "--output-governed") {
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output ||
        !output.write(reinterpret_cast<const char *>(governed_glb.data()),
                      static_cast<std::streamsize>(governed_glb.size())))
      throw std::runtime_error("failed to write governed GLB fixture");
  }
  const std::array animations{animation()};
  const auto animated = export_projection3d_glb(
      fixture(), animations, std::span<const GltfTextureAsset>{}, {});
  if (argc == 3 && std::string_view(argv[1]) == "--output") {
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output ||
        !output.write(reinterpret_cast<const char *>(animated.data()),
                      static_cast<std::streamsize>(animated.size())))
      throw std::runtime_error("failed to write GLB validation fixture");
  }
  check(glb.size() % 4 == 0 && u32(glb, 0) == 0x46546c67 && u32(glb, 4) == 2 &&
            u32(glb, 8) == glb.size(),
        "GLB header invalid");
  check(u32(glb, 16) == 0x4e4f534a, "GLB JSON chunk missing");
  const auto json_length = u32(glb, 12);
  check(u32(glb, 24 + json_length) == 0x004e4942, "GLB BIN chunk missing");
  const auto animated_json_length = u32(animated, 12);
  const std::string animated_json(
      reinterpret_cast<const char *>(animated.data() + 20),
      animated_json_length);
  check(animated_json.find("\"animations\"") != std::string::npos &&
            animated_json.find("\"gsplEvents\"") != std::string::npos,
        "GLB animation or semantic events missing");
  auto collapsed = animation();
  collapsed.joint_tracks[0].keys[1].pose.scale_xyz_ppm = {1, 1, 1};
  bool deformation_rejected = false;
  try {
    const std::array invalid_animations{collapsed};
    (void)export_projection3d_glb(fixture(), invalid_animations,
                                  std::span<const GltfTextureAsset>{}, {});
  } catch (const std::invalid_argument &) {
    deformation_rejected = true;
  }
  check(deformation_rejected, "collapsed GLB animation accepted");
  GltfExportLimits tiny;
  tiny.maximum_glb_bytes = 32;
  bool bounded = false;
  try {
    (void)export_projection3d_glb(fixture(), {}, tiny);
  } catch (const std::length_error &) {
    bounded = true;
  }
  check(bounded, "GLB output limit ignored");
  auto missing = fixture();
  missing.materials[0].normal_texture_id = "texture.normal";
  bool required = false;
  try {
    (void)export_projection3d_glb(missing);
  } catch (const std::invalid_argument &) {
    required = true;
  }
  check(required, "missing texture accepted");
  bool hostile = false;
  try {
    const std::array malformed{std::byte{0x01}, std::byte{0x02}};
    GltfTextureAsset texture{
        "texture.normal", "image/png",
        std::vector<std::byte>(malformed.begin(), malformed.end())};
    (void)export_projection3d_glb(missing, std::span{&texture, 1});
  } catch (const std::exception &) {
    hostile = true;
  }
  check(hostile, "malformed embedded PNG accepted");
  auto textured = fixture();
  textured.materials[0].normal_texture_id = "texture.normal";
  const ImageRgba8 normal_image{
      1, 1, ColorSpace::data, AlphaMode::opaque, {128, 128, 255, 255}};
  const GltfTextureAsset normal_texture{"texture.normal", "image/png",
                                        encode_png(normal_image)};
  const auto textured_glb =
      export_projection3d_glb(textured, std::span{&normal_texture, 1});
  const auto textured_json_length = u32(textured_glb, 12);
  const std::string textured_json(
      reinterpret_cast<const char *>(textured_glb.data() + 20),
      textured_json_length);
  check(textured_json.find("\"TANGENT\"") != std::string::npos,
        "normal-mapped GLB lacks generated tangents");
  const auto lod_glb = export_projection3d_glb(lod_fixture());
  const auto lod_json_length = u32(lod_glb, 12);
  const std::string lod_json(
      reinterpret_cast<const char *>(lod_glb.data() + 20), lod_json_length);
  check(lod_json.find("\"gsplLodLevel\"") != std::string::npos,
        "GLB lacks governed LOD metadata");
  if (argc == 3 && std::string_view(argv[1]) == "--output-lod") {
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output || !output.write(reinterpret_cast<const char *>(lod_glb.data()),
                                 static_cast<std::streamsize>(lod_glb.size())))
      throw std::runtime_error("failed to write LOD GLB fixture");
  }
  if (argc == 3 && std::string_view(argv[1]) == "--output-textured") {
    std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
    if (!output ||
        !output.write(reinterpret_cast<const char *>(textured_glb.data()),
                      static_cast<std::streamsize>(textured_glb.size())))
      throw std::runtime_error("failed to write textured GLB fixture");
  }
  std::filesystem::remove_all(source_package);
  std::cout << "all gspl sprites GLB export tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
