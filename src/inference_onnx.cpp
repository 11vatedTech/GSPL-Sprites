#include "gspl_sprites/inference.hpp"

#include "gspl_sprites/core.hpp"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace gspl::sprites {
namespace {
std::size_t element_size(TensorElement element) {
  switch (element) {
    case TensorElement::uint8: return 1;
    case TensorElement::int64: return 8;
    case TensorElement::float16: return 2;
    case TensorElement::float32: return 4;
  }
  throw std::logic_error("unreachable tensor element");
}

ONNXTensorElementDataType ort_element(TensorElement element) {
  switch (element) {
    case TensorElement::uint8: return ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8;
    case TensorElement::int64: return ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64;
    case TensorElement::float16: return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16;
    case TensorElement::float32: return ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT;
  }
  throw std::logic_error("unreachable tensor element");
}

std::uint64_t element_count(std::span<const std::int64_t> shape, std::uint64_t limit) {
  if (shape.empty()) throw std::invalid_argument("scalar tensors are not supported by the initial provider");
  std::uint64_t count = 1;
  for (const auto dimension : shape) {
    if (dimension <= 0 || count > limit / static_cast<std::uint64_t>(dimension))
      throw std::invalid_argument("tensor shape is invalid or exceeds the element limit");
    count *= static_cast<std::uint64_t>(dimension);
  }
  return count;
}

void validate_shape(const TensorSpec& spec, std::span<const std::int64_t> shape) {
  if (shape.size() != spec.dimensions.size()) throw std::invalid_argument("tensor rank disagrees with governed contract: " + spec.name);
  for (std::size_t i = 0; i < shape.size(); ++i) {
    if (shape[i] < 0 || static_cast<std::uint64_t>(shape[i]) < spec.dimensions[i].minimum || static_cast<std::uint64_t>(shape[i]) > spec.dimensions[i].maximum)
      throw std::invalid_argument("tensor dimension disagrees with governed contract: " + spec.name);
  }
}

std::vector<std::byte> read_model(const std::filesystem::path& path, std::uint64_t limit) {
  std::error_code error;
  const auto status = std::filesystem::symlink_status(path, error);
  if (error || !std::filesystem::is_regular_file(status) || std::filesystem::is_symlink(status)) throw std::runtime_error("model path must name a regular non-symlink file");
  const auto size = std::filesystem::file_size(path, error);
  if (error || size == 0 || size > limit || size > std::numeric_limits<std::size_t>::max()) throw std::runtime_error("model file is empty or exceeds its byte limit");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  std::ifstream stream(path, std::ios::binary);
  if (!stream || !stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) throw std::runtime_error("failed to read complete model file");
  return bytes;
}

const TensorSpec& contract_tensor(std::span<const TensorSpec> specs, std::string_view name) {
  const auto found = std::ranges::find(specs, name, &TensorSpec::name);
  if (found == specs.end()) throw std::invalid_argument("tensor is absent from governed contract: " + std::string(name));
  return *found;
}

void validate_session_contract(Ort::Session& session, const ModelDescriptor& descriptor) {
  Ort::AllocatorWithDefaultOptions allocator;
  if (session.GetInputCount() != descriptor.contract.inputs.size() || session.GetOutputCount() != descriptor.contract.outputs.size()) throw std::runtime_error("loaded model tensor count disagrees with governed contract");
  const auto validate_side = [&](bool input) {
    const auto count = input ? session.GetInputCount() : session.GetOutputCount();
    const auto& specs = input ? descriptor.contract.inputs : descriptor.contract.outputs;
    for (std::size_t i = 0; i < count; ++i) {
      auto name = input ? session.GetInputNameAllocated(i, allocator) : session.GetOutputNameAllocated(i, allocator);
      const auto& spec = contract_tensor(specs, name.get());
      const auto info = (input ? session.GetInputTypeInfo(i) : session.GetOutputTypeInfo(i)).GetTensorTypeAndShapeInfo();
      if (info.GetElementType() != ort_element(spec.element)) throw std::runtime_error("loaded model tensor element type disagrees with governed contract: " + spec.name);
      const auto shape = info.GetShape();
      if (shape.size() != spec.dimensions.size()) throw std::runtime_error("loaded model tensor rank disagrees with governed contract: " + spec.name);
      for (std::size_t d = 0; d < shape.size(); ++d) if (shape[d] > 0 && (static_cast<std::uint64_t>(shape[d]) < spec.dimensions[d].minimum || static_cast<std::uint64_t>(shape[d]) > spec.dimensions[d].maximum)) throw std::runtime_error("loaded model tensor dimension disagrees with governed contract: " + spec.name);
    }
  };
  validate_side(true); validate_side(false);
}
} // namespace

InferenceResult run_onnx_cpu(const ModelDescriptor& descriptor, const std::filesystem::path& model_path, std::span<const InferenceTensor> inputs, const InferenceLimits& limits) {
  const auto descriptor_validation = validate_model_descriptor(descriptor);
  if (!descriptor_validation.ok()) throw std::invalid_argument(descriptor_validation.diagnostics.front().code + ": " + descriptor_validation.diagnostics.front().message);
  if (limits.intra_op_threads == 0 || limits.intra_op_threads > 256 || limits.max_tensor_elements == 0 || limits.max_total_input_bytes == 0 || limits.max_total_output_bytes == 0) throw std::invalid_argument("inference limits are invalid");
  const auto support = std::ranges::find_if(descriptor.backends, [](const BackendSupport& value) { return value.backend == DeviceBackend::cpu && value.precision == NumericPrecision::fp32; });
  if (support == descriptor.backends.end()) throw std::invalid_argument("model does not declare CPU FP32 support");
  const auto model_bytes = read_model(model_path, limits.max_model_bytes);
  const auto model_hash = sha256(std::string_view(reinterpret_cast<const char*>(model_bytes.data()), model_bytes.size()));
  if (model_hash != descriptor.model_sha256) throw std::runtime_error("model SHA-256 disagrees with governed descriptor");
  if (inputs.size() != descriptor.contract.inputs.size()) throw std::invalid_argument("input tensor count disagrees with governed contract");

  std::map<std::string_view, const InferenceTensor*, std::less<>> ordered_inputs;
  std::uint64_t total_input_bytes = 0;
  for (const auto& input : inputs) {
    const auto& spec = contract_tensor(descriptor.contract.inputs, input.name);
    if (input.element != spec.element) throw std::invalid_argument("input element type disagrees with governed contract: " + input.name);
    validate_shape(spec, input.shape);
    const auto count = element_count(input.shape, limits.max_tensor_elements);
    if (count > std::numeric_limits<std::uint64_t>::max() / element_size(input.element) || input.data.size() != count * element_size(input.element)) throw std::invalid_argument("input tensor byte size is invalid: " + input.name);
    if (!ordered_inputs.emplace(input.name, &input).second || input.data.size() > limits.max_total_input_bytes || total_input_bytes > limits.max_total_input_bytes - input.data.size()) throw std::invalid_argument("inputs are duplicate or exceed total byte limit");
    total_input_bytes += input.data.size();
  }

  Ort::Env environment{ORT_LOGGING_LEVEL_WARNING, "gspl-sprites"};
  Ort::SessionOptions options;
  options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
  options.SetIntraOpNumThreads(static_cast<int>(limits.intra_op_threads));
  options.SetInterOpNumThreads(1);
  options.AddConfigEntry("session.intra_op.allow_spinning", "0");
  options.AddConfigEntry("session.inter_op.allow_spinning", "0");
  options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
  Ort::Session session{environment, model_bytes.data(), model_bytes.size(), options};
  validate_session_contract(session, descriptor);

  Ort::MemoryInfo memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  std::vector<const char*> input_names; std::vector<Ort::Value> input_values;
  input_names.reserve(descriptor.contract.inputs.size()); input_values.reserve(descriptor.contract.inputs.size());
  for (const auto& spec : descriptor.contract.inputs) {
    const auto& input = *ordered_inputs.at(spec.name);
    input_names.push_back(input.name.c_str());
    input_values.push_back(Ort::Value::CreateTensor(memory, const_cast<std::byte*>(input.data.data()), input.data.size(), input.shape.data(), input.shape.size(), ort_element(input.element)));
  }
  std::vector<const char*> output_names; output_names.reserve(descriptor.contract.outputs.size());
  for (const auto& output : descriptor.contract.outputs) output_names.push_back(output.name.c_str());
  auto output_values = session.Run(Ort::RunOptions{nullptr}, input_names.data(), input_values.data(), input_values.size(), output_names.data(), output_names.size());

  InferenceResult result{"onnxruntime-cpu", Ort::GetVersionString(), model_hash, DeviceBackend::cpu, NumericPrecision::fp32, {}};
  std::uint64_t total_output_bytes = 0;
  for (std::size_t i = 0; i < output_values.size(); ++i) {
    if (!output_values[i].IsTensor()) throw std::runtime_error("provider returned a non-tensor output");
    const auto& spec = descriptor.contract.outputs[i]; const auto info = output_values[i].GetTensorTypeAndShapeInfo();
    if (info.GetElementType() != ort_element(spec.element)) throw std::runtime_error("provider output element type disagrees with governed contract: " + spec.name);
    const auto shape = info.GetShape(); validate_shape(spec, shape);
    const auto count = element_count(shape, limits.max_tensor_elements); const auto bytes = count * element_size(spec.element);
    if (bytes > limits.max_total_output_bytes || total_output_bytes > limits.max_total_output_bytes - bytes) throw std::runtime_error("provider outputs exceed total byte limit");
    InferenceTensor output{spec.name, spec.element, shape, std::vector<std::byte>(static_cast<std::size_t>(bytes))};
    std::memcpy(output.data.data(), output_values[i].GetTensorRawData(), output.data.size()); total_output_bytes += bytes; result.outputs.push_back(std::move(output));
  }
  return result;
}

} // namespace gspl::sprites
