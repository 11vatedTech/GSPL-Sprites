#include "gspl_sprites/inference.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
constexpr std::array<std::uint8_t,130> model_bytes{0x08,0x03,0x12,0x06,0x63,0x68,0x65,0x6e,0x74,0x61,0x3a,0x70,0x0a,0x15,0x0a,0x01,0x58,0x0a,0x01,0x57,0x12,0x01,0x59,0x1a,0x05,0x6d,0x75,0x6c,0x5f,0x31,0x22,0x03,0x4d,0x75,0x6c,0x12,0x08,0x6d,0x75,0x6c,0x20,0x74,0x65,0x73,0x74,0x2a,0x23,0x08,0x03,0x08,0x02,0x10,0x01,0x22,0x18,0x00,0x00,0x80,0x3f,0x00,0x00,0x00,0x40,0x00,0x00,0x40,0x40,0x00,0x00,0x80,0x40,0x00,0x00,0xa0,0x40,0x00,0x00,0xc0,0x40,0x42,0x01,0x57,0x5a,0x13,0x0a,0x01,0x58,0x12,0x0e,0x0a,0x0c,0x08,0x01,0x12,0x08,0x0a,0x02,0x08,0x03,0x0a,0x02,0x08,0x02,0x62,0x13,0x0a,0x01,0x59,0x12,0x0e,0x0a,0x0c,0x08,0x01,0x12,0x08,0x0a,0x02,0x08,0x03,0x0a,0x02,0x08,0x02,0x42,0x04,0x0a,0x00,0x10,0x07};

ModelDescriptor descriptor() {
  return {"ort.mul","1.0","https://github.com/microsoft/onnxruntime/tree/v1.26.0/onnxruntime/test/testdata/mul_1.onnx","v1.26.0:0b6dc510261322f3ff63deaa4114a0a023b35246","71f431c4e9321ec6fbeb158d02ed240459a7dcc98673fa79a4f439ce42efaf10",{"MIT",true,true,true},{ModelTask::enhancement},{{{"X",TensorElement::float32,{{"rows",3,3},{"columns",2,2}}}},{{"Y",TensorElement::float32,{{"rows",3,3},{"columns",2,2}}}}},64ULL*1024ULL*1024ULL,0,{{DeviceBackend::cpu,NumericPrecision::fp32}},DeterminismClass::bitwise_reproducible};
}

struct FixtureFile {
  std::filesystem::path path;
  FixtureFile() {
    path=std::filesystem::temp_directory_path()/("gspl-ort-mul-"+std::to_string(std::hash<std::string>{}(std::filesystem::current_path().string()))+".onnx");
    std::ofstream stream(path,std::ios::binary|std::ios::trunc); stream.write(reinterpret_cast<const char*>(model_bytes.data()),static_cast<std::streamsize>(model_bytes.size())); if(!stream)throw std::runtime_error("failed to write ONNX fixture");
  }
  ~FixtureFile(){std::error_code ignored;std::filesystem::remove(path,ignored);}
};
}

int main(){try{
  FixtureFile fixture;std::array<float,6> values{1,2,3,4,5,6};InferenceTensor input{"X",TensorElement::float32,{3,2},std::vector<std::byte>(sizeof(values))};std::memcpy(input.data.data(),values.data(),sizeof(values));const std::array inputs{input};
  const auto result=run_onnx_cpu(descriptor(),fixture.path,inputs);check(result.provider=="onnxruntime-cpu"&&result.provider_version=="1.26.0","provider identity missing");check(result.model_sha256==descriptor().model_sha256&&result.outputs.size()==1,"inference provenance missing");std::array<float,6> output{};std::memcpy(output.data(),result.outputs[0].data.data(),sizeof(output));check(output==std::array<float,6>{1,4,9,16,25,36},"real ONNX inference output is incorrect");
  bool bad_hash=false;try{auto changed=descriptor();changed.model_sha256=std::string(64,'a');(void)run_onnx_cpu(changed,fixture.path,inputs);}catch(const std::runtime_error&){bad_hash=true;}check(bad_hash,"model hash mismatch accepted");
  bool bad_shape=false;try{auto malformed=input;malformed.shape={6,1};const std::array malformed_inputs{malformed};(void)run_onnx_cpu(descriptor(),fixture.path,malformed_inputs);}catch(const std::invalid_argument&){bad_shape=true;}check(bad_shape,"input contract mismatch accepted");
  bool undeclared=false;try{auto unsupported=descriptor();unsupported.backends={{DeviceBackend::cuda,NumericPrecision::fp32}};(void)run_onnx_cpu(unsupported,fixture.path,inputs);}catch(const std::invalid_argument&){undeclared=true;}check(undeclared,"undeclared CPU fallback accepted");
  bool output_limited=false;try{auto limits=InferenceLimits{};limits.max_total_output_bytes=4;(void)run_onnx_cpu(descriptor(),fixture.path,inputs,limits);}catch(const std::runtime_error&){output_limited=true;}check(output_limited,"output byte limit ignored");
  std::cout<<"all gspl sprites inference tests passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
