#include "gspl_sprites/model.hpp"

#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace{void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}ModelDescriptor model(std::string id,DeviceBackend backend,NumericPrecision precision,bool commercial,DeterminismClass determinism,std::uint64_t vram){return{id,"1.0","https://models.example.invalid/"+id,"revision-immutable","0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",{"Apache-2.0",commercial,true,true},{ModelTask::segmentation},{{{"image",TensorElement::float32,{{"batch",1,1},{"height",64,4096},{"width",64,4096},{"channels",3,3}}}},{{"mask",TensorElement::uint8,{{"batch",1,1},{"height",64,4096},{"width",64,4096}}}}},512ULL*1024*1024,vram,{{backend,precision}},determinism};}}

int main(){try{
  auto cuda=model("seg.cuda",DeviceBackend::cuda,NumericPrecision::fp16,true,DeterminismClass::artifact_reproducible,2ULL*1024*1024*1024);auto cpu=model("seg.cpu",DeviceBackend::cpu,NumericPrecision::fp32,true,DeterminismClass::bitwise_reproducible,0);check(validate_model_descriptor(cuda).ok(),"valid descriptor rejected");
  ModelRegistry registry;registry.register_model(cuda);registry.register_model(cpu);registry.register_model(cuda);check(registry.size()==2,"idempotent registration failed");
  const ModelRequest fast{ModelTask::segmentation,{DeviceBackend::cuda,DeviceBackend::cpu},{NumericPrecision::fp16,NumericPrecision::fp32},8ULL*1024*1024*1024,4ULL*1024*1024*1024,true,DeterminismClass::artifact_reproducible};check(registry.select(fast)!=nullptr&&registry.select(fast)->id=="seg.cuda","preferred compatible model not selected");
  auto constrained=fast;constrained.available_vram_bytes=1ULL*1024*1024*1024;check(registry.select(constrained)!=nullptr&&registry.select(constrained)->id=="seg.cpu","resource fallback failed");
  auto strict=fast;strict.weakest_acceptable_determinism=DeterminismClass::bitwise_reproducible;check(registry.select(strict)!=nullptr&&registry.select(strict)->id=="seg.cpu","determinism requirement ignored");
  auto noncommercial=model("seg.research",DeviceBackend::cuda,NumericPrecision::fp16,false,DeterminismClass::bitwise_reproducible,0);registry.register_model(noncommercial);auto only_cuda=strict;only_cuda.backend_preference={DeviceBackend::cuda};check(registry.select(only_cuda)==nullptr,"noncommercial model selected for commercial output");
  auto changed=cuda;changed.model_sha256=std::string(64,'a');bool mutation_rejected=false;try{registry.register_model(changed);}catch(const std::logic_error&){mutation_rejected=true;}check(mutation_rejected,"descriptor mutation accepted");
  auto invalid=cuda;invalid.contract.inputs[0].dimensions[1].maximum=0;check(!validate_model_descriptor(invalid).ok(),"invalid tensor bound accepted");check(canonicalize_model_descriptor(cuda)==canonicalize_model_descriptor(cuda),"model canonicalization nondeterministic");check(registry.canonical_manifest().find("seg.cuda")!=std::string::npos,"registry manifest missing model");
  std::cout<<"all gspl sprites model tests passed\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}

