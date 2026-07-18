#include "gspl_sprites/gltf_export.hpp"

#include <cstring>
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
  p.materials = {{"mat"}};
  p.skeleton = Skeleton3d{"rig", {{"root", {}}}};
  Vertex3d a{{0, 0, 0}, {0, 0, 1'000'000}, {0, 0}, {{"root", 1'000'000}}},
      b = a, c = a;
  b.position.x = 1'000'000;
  c.position.y = 1'000'000;
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
} // namespace
int main(int argc, char **argv) try {
  const auto glb = export_projection3d_glb(fixture());
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
  std::cout << "all gspl sprites GLB export tests passed\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
