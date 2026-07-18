#pragma once

#include "gspl_sprites/animation3d.hpp"
#include "gspl_sprites/deformation_quality.hpp"
#include "gspl_sprites/image.hpp"
#include "gspl_sprites/mesh_quality.hpp"
#include "gspl_sprites/projection3d.hpp"

#include <cstddef>
#include <span>

namespace gspl::sprites {

struct GltfTextureAsset {
  std::string id;
  std::string mime_type;
  std::vector<std::byte> encoded_bytes;
};
struct GltfExportLimits {
  std::uint64_t maximum_glb_bytes{512ULL * 1024ULL * 1024ULL};
  std::uint64_t maximum_texture_bytes{256ULL * 1024ULL * 1024ULL};
  ImageLimits texture_image;
  MeshQualityPolicy mesh_quality;
  DeformationQualityPolicy deformation_quality;
};

[[nodiscard]] std::vector<std::byte>
export_projection3d_glb(const Projection3dDefinition &projection,
                        std::span<const GltfTextureAsset> textures = {},
                        const GltfExportLimits &limits = {});
[[nodiscard]] std::vector<std::byte>
export_projection3d_glb(const Projection3dDefinition &projection,
                        std::span<const AnimationClip3d> animations,
                        std::span<const GltfTextureAsset> textures,
                        const GltfExportLimits &limits);

} // namespace gspl::sprites
