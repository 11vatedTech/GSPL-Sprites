#pragma once

#include "gspl_sprites/common.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <span>

namespace gspl::sprites {

enum class ModelTask { image_generation, image_editing, segmentation, matting, enhancement, pose_estimation, depth_estimation, normal_estimation, multi_view_generation, reconstruction_3d, texture_generation, motion_generation, motion_retargeting, audio_generation };
enum class DeviceBackend { cpu, cuda, tensor_rt, winml, openvino, coreml };
enum class NumericPrecision { fp32, fp16, bf16, int8 };
enum class DeterminismClass { bitwise_reproducible, artifact_reproducible, structurally_reproducible, semantically_reproducible, best_effort };
enum class TensorElement { uint8, int64, float16, float32 };

struct TensorDimension { std::string name; std::uint64_t minimum{}; std::uint64_t maximum{}; };
struct TensorSpec { std::string name; TensorElement element{}; std::vector<TensorDimension> dimensions; };
struct ModelContract { std::vector<TensorSpec> inputs; std::vector<TensorSpec> outputs; };
struct ModelLicense { std::string spdx_expression; bool commercial_use{}; bool redistribution{}; bool derivatives{}; };
struct BackendSupport { DeviceBackend backend{}; NumericPrecision precision{}; };

struct ModelDescriptor {
  std::string id;
  std::string version;
  std::string source;
  std::string source_revision;
  std::string model_sha256;
  ModelLicense license;
  std::vector<ModelTask> tasks;
  ModelContract contract;
  std::uint64_t minimum_ram_bytes{};
  std::uint64_t minimum_vram_bytes{};
  std::vector<BackendSupport> backends;
  DeterminismClass determinism{DeterminismClass::best_effort};
};

struct ModelRequest {
  ModelTask task{};
  std::vector<DeviceBackend> backend_preference;
  std::vector<NumericPrecision> precision_preference;
  std::uint64_t available_ram_bytes{};
  std::uint64_t available_vram_bytes{};
  bool commercial_output{};
  DeterminismClass weakest_acceptable_determinism{DeterminismClass::best_effort};
};

[[nodiscard]] ValidationResult validate_model_descriptor(const ModelDescriptor& descriptor);
[[nodiscard]] std::string canonicalize_model_descriptor(const ModelDescriptor& descriptor);

class ModelRegistry final {
public:
  void register_model(ModelDescriptor descriptor);
  [[nodiscard]] const ModelDescriptor* find(std::string_view id, std::string_view version) const noexcept;
  [[nodiscard]] const ModelDescriptor* select(const ModelRequest& request) const noexcept;
  [[nodiscard]] std::string canonical_manifest() const;
  [[nodiscard]] std::size_t size() const noexcept { return models_.size(); }
private:
  std::map<std::string, ModelDescriptor, std::less<>> models_;
};

} // namespace gspl::sprites

