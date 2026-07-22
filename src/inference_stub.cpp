#include "gspl_sprites/inference.hpp"
#include <stdexcept>

namespace gspl::sprites {

InferenceResult run_onnx_cpu(const ModelDescriptor&, const std::filesystem::path&,
                             std::span<const InferenceTensor>, const InferenceLimits&) {
    throw std::runtime_error("GSPL compiled without ONNX Runtime (GSPL_CORE_ONLY)");
}

}
