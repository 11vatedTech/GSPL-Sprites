#pragma once

#include "gspl_sprites/model.hpp"

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace gspl::sprites {

struct InferenceTensor {
  std::string name;
  TensorElement element{TensorElement::float32};
  std::vector<std::int64_t> shape;
  std::vector<std::byte> data;
};

struct InferenceLimits {
  std::uint64_t max_model_bytes{4ULL * 1024ULL * 1024ULL * 1024ULL};
  std::uint64_t max_tensor_elements{256ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_input_bytes{1024ULL * 1024ULL * 1024ULL};
  std::uint64_t max_total_output_bytes{1024ULL * 1024ULL * 1024ULL};
  std::uint32_t intra_op_threads{1};
};

struct InferenceResult {
  std::string provider;
  std::string provider_version;
  std::string model_sha256;
  DeviceBackend backend{DeviceBackend::cpu};
  NumericPrecision precision{NumericPrecision::fp32};
  std::vector<InferenceTensor> outputs;
};

[[nodiscard]] InferenceResult run_onnx_cpu(const ModelDescriptor& descriptor,
                                           const std::filesystem::path& model_path,
                                           std::span<const InferenceTensor> inputs,
                                           const InferenceLimits& limits = {});

} // namespace gspl::sprites
