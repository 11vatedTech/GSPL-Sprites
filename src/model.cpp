#include "gspl_sprites/model.hpp"

#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
std::string escape_json(std::string_view value){std::string out;for(const unsigned char c:value){switch(c){case '\\':out+="\\\\";break;case '"':out+="\\\"";break;case '\n':out+="\\n";break;case '\r':out+="\\r";break;case '\t':out+="\\t";break;default:if(c<0x20)throw std::invalid_argument("control character in model descriptor");out+=static_cast<char>(c);}}return out;}
bool stable_id(std::string_view value){return !value.empty()&&value.size()<=128&&std::ranges::all_of(value,[](unsigned char c){return std::isalnum(c)!=0||c=='.'||c=='_'||c=='-';});}
bool sha(std::string_view value){return value.size()==64&&std::ranges::all_of(value,[](unsigned char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
std::string task_text(ModelTask value){switch(value){case ModelTask::image_generation:return"IMAGE_GENERATION";case ModelTask::image_editing:return"IMAGE_EDITING";case ModelTask::segmentation:return"SEGMENTATION";case ModelTask::matting:return"MATTING";case ModelTask::enhancement:return"ENHANCEMENT";case ModelTask::pose_estimation:return"POSE_ESTIMATION";case ModelTask::depth_estimation:return"DEPTH_ESTIMATION";case ModelTask::normal_estimation:return"NORMAL_ESTIMATION";case ModelTask::multi_view_generation:return"MULTI_VIEW_GENERATION";case ModelTask::reconstruction_3d:return"RECONSTRUCTION_3D";case ModelTask::texture_generation:return"TEXTURE_GENERATION";case ModelTask::motion_generation:return"MOTION_GENERATION";case ModelTask::motion_retargeting:return"MOTION_RETARGETING";case ModelTask::audio_generation:return"AUDIO_GENERATION";}throw std::logic_error("unreachable model task");}
std::string backend_text(DeviceBackend value){switch(value){case DeviceBackend::cpu:return"CPU";case DeviceBackend::cuda:return"CUDA";case DeviceBackend::tensor_rt:return"TENSOR_RT";case DeviceBackend::winml:return"WINML";case DeviceBackend::openvino:return"OPENVINO";case DeviceBackend::coreml:return"COREML";}throw std::logic_error("unreachable backend");}
std::string precision_text(NumericPrecision value){switch(value){case NumericPrecision::fp32:return"FP32";case NumericPrecision::fp16:return"FP16";case NumericPrecision::bf16:return"BF16";case NumericPrecision::int8:return"INT8";}throw std::logic_error("unreachable precision");}
std::string element_text(TensorElement value){switch(value){case TensorElement::uint8:return"UINT8";case TensorElement::int64:return"INT64";case TensorElement::float16:return"FLOAT16";case TensorElement::float32:return"FLOAT32";}throw std::logic_error("unreachable tensor element");}
std::string determinism_text(DeterminismClass value){switch(value){case DeterminismClass::bitwise_reproducible:return"BITWISE_REPRODUCIBLE";case DeterminismClass::artifact_reproducible:return"ARTIFACT_REPRODUCIBLE";case DeterminismClass::structurally_reproducible:return"STRUCTURALLY_REPRODUCIBLE";case DeterminismClass::semantically_reproducible:return"SEMANTICALLY_REPRODUCIBLE";case DeterminismClass::best_effort:return"BEST_EFFORT";}throw std::logic_error("unreachable determinism class");}
std::size_t rank(std::span<const DeviceBackend> values,DeviceBackend value){const auto found=std::ranges::find(values,value);return found==values.end()?std::numeric_limits<std::size_t>::max():static_cast<std::size_t>(found-values.begin());}
std::size_t rank(std::span<const NumericPrecision> values,NumericPrecision value){const auto found=std::ranges::find(values,value);return found==values.end()?std::numeric_limits<std::size_t>::max():static_cast<std::size_t>(found-values.begin());}
}

ValidationResult validate_model_descriptor(const ModelDescriptor& descriptor){
  ValidationResult result;auto add=[&](std::string code,std::string message){result.diagnostics.push_back({std::move(code),std::move(message)});};
  if(!stable_id(descriptor.id)||descriptor.version.empty()||descriptor.version.size()>64)add("SPRITE_MODEL_ID_INVALID","model id or version is invalid");
  if(descriptor.source.empty()||descriptor.source.size()>2048||descriptor.source_revision.empty()||descriptor.source_revision.size()>256)add("SPRITE_MODEL_SOURCE_INVALID","model source and immutable revision are required and bounded");
  if(!sha(descriptor.model_sha256))add("SPRITE_MODEL_HASH_INVALID","model file SHA-256 must be lowercase hexadecimal");
  if(descriptor.license.spdx_expression.empty()||descriptor.license.spdx_expression.size()>256)add("SPRITE_MODEL_LICENSE_INVALID","SPDX license expression is required");
  if(descriptor.tasks.empty()||descriptor.tasks.size()>32)add("SPRITE_MODEL_TASKS_INVALID","model must declare 1..32 tasks");else{auto tasks=descriptor.tasks;std::ranges::sort(tasks);if(std::ranges::adjacent_find(tasks)!=tasks.end())add("SPRITE_MODEL_TASK_DUPLICATE","model tasks must be unique");}
  if(descriptor.backends.empty()||descriptor.backends.size()>32)add("SPRITE_MODEL_BACKENDS_INVALID","model must declare 1..32 backend/precision pairs");else{auto backends=descriptor.backends;std::ranges::sort(backends,{},[](const auto& value){return std::pair{value.backend,value.precision};});if(std::ranges::adjacent_find(backends,{},[](const auto& value){return std::pair{value.backend,value.precision};})!=backends.end())add("SPRITE_MODEL_BACKEND_DUPLICATE","backend/precision pairs must be unique");}
  const auto validate_tensors=[&](const std::vector<TensorSpec>& tensors,std::string_view side){if(tensors.empty()||tensors.size()>64){add("SPRITE_MODEL_CONTRACT_INVALID",std::string(side)+" tensor count must be 1..64");return;}std::set<std::string> names;for(const auto& tensor:tensors){if(!stable_id(tensor.name)||!names.insert(tensor.name).second||tensor.dimensions.empty()||tensor.dimensions.size()>8){add("SPRITE_MODEL_TENSOR_INVALID",std::string(side)+" tensor is duplicate or malformed");continue;}for(const auto& dimension:tensor.dimensions)if(dimension.minimum==0||dimension.maximum<dimension.minimum||dimension.maximum>1'000'000||dimension.name.size()>64)add("SPRITE_MODEL_DIMENSION_INVALID",std::string(side)+" tensor dimension is unbounded or malformed");}};
  validate_tensors(descriptor.contract.inputs,"input");validate_tensors(descriptor.contract.outputs,"output");
  if(descriptor.minimum_ram_bytes==0)add("SPRITE_MODEL_MEMORY_INVALID","minimum RAM requirement must be explicit and nonzero");
  return result;
}

std::string canonicalize_model_descriptor(const ModelDescriptor& descriptor){
  const auto validation=validate_model_descriptor(descriptor);if(!validation.ok())throw std::invalid_argument(validation.diagnostics.front().code+": "+validation.diagnostics.front().message);
  auto tasks=descriptor.tasks;auto backends=descriptor.backends;auto inputs=descriptor.contract.inputs;auto outputs=descriptor.contract.outputs;std::ranges::sort(tasks);std::ranges::sort(backends,{},[](const auto& value){return std::pair{value.backend,value.precision};});std::ranges::sort(inputs,{},&TensorSpec::name);std::ranges::sort(outputs,{},&TensorSpec::name);
  const auto tensors_json=[](const std::vector<TensorSpec>& tensors){std::ostringstream out;out<<'[';for(std::size_t i=0;i<tensors.size();++i){if(i)out<<',';const auto&t=tensors[i];out<<"{\"dimensions\":[";for(std::size_t d=0;d<t.dimensions.size();++d){if(d)out<<',';const auto&dim=t.dimensions[d];out<<"{\"maximum\":"<<dim.maximum<<",\"minimum\":"<<dim.minimum<<",\"name\":\""<<escape_json(dim.name)<<"\"}";}out<<"],\"element\":\""<<element_text(t.element)<<"\",\"name\":\""<<escape_json(t.name)<<"\"}";}out<<']';return out.str();};
  std::ostringstream out;out<<"{\"backends\":[";for(std::size_t i=0;i<backends.size();++i){if(i)out<<',';out<<"{\"backend\":\""<<backend_text(backends[i].backend)<<"\",\"precision\":\""<<precision_text(backends[i].precision)<<"\"}";}out<<"],\"contract\":{\"inputs\":"<<tensors_json(inputs)<<",\"outputs\":"<<tensors_json(outputs)<<"},\"determinism\":\""<<determinism_text(descriptor.determinism)<<"\",\"id\":\""<<escape_json(descriptor.id)<<"\",\"license\":{\"commercialUse\":"<<(descriptor.license.commercial_use?"true":"false")<<",\"derivatives\":"<<(descriptor.license.derivatives?"true":"false")<<",\"redistribution\":"<<(descriptor.license.redistribution?"true":"false")<<",\"spdx\":\""<<escape_json(descriptor.license.spdx_expression)<<"\"},\"minimumRamBytes\":"<<descriptor.minimum_ram_bytes<<",\"minimumVramBytes\":"<<descriptor.minimum_vram_bytes<<",\"modelSha256\":\""<<descriptor.model_sha256<<"\",\"source\":\""<<escape_json(descriptor.source)<<"\",\"sourceRevision\":\""<<escape_json(descriptor.source_revision)<<"\",\"tasks\":[";for(std::size_t i=0;i<tasks.size();++i){if(i)out<<',';out<<'"'<<task_text(tasks[i])<<'"';}out<<"],\"version\":\""<<escape_json(descriptor.version)<<"\"}";return out.str();
}

void ModelRegistry::register_model(ModelDescriptor descriptor){const auto validation=validate_model_descriptor(descriptor);if(!validation.ok())throw std::invalid_argument(validation.diagnostics.front().code+": "+validation.diagnostics.front().message);const auto key=descriptor.id+'@'+descriptor.version;const auto found=models_.find(key);if(found!=models_.end()){if(canonicalize_model_descriptor(found->second)!=canonicalize_model_descriptor(descriptor))throw std::logic_error("model descriptor identity is immutable: "+key);return;}models_.emplace(key,std::move(descriptor));}
const ModelDescriptor* ModelRegistry::find(std::string_view id,std::string_view version)const noexcept{const auto found=models_.find(std::string(id)+'@'+std::string(version));return found==models_.end()?nullptr:&found->second;}

const ModelDescriptor* ModelRegistry::select(const ModelRequest& request)const noexcept{
  const ModelDescriptor* best=nullptr;std::tuple<std::size_t,std::size_t,std::string_view,std::string_view> best_score{std::numeric_limits<std::size_t>::max(),std::numeric_limits<std::size_t>::max(),{}, {}};
  for(const auto& [key,model]:models_){if(request.commercial_output&&!model.license.commercial_use)continue;if(model.minimum_ram_bytes>request.available_ram_bytes||model.minimum_vram_bytes>request.available_vram_bytes)continue;if(static_cast<int>(model.determinism)>static_cast<int>(request.weakest_acceptable_determinism))continue;if(std::ranges::find(model.tasks,request.task)==model.tasks.end())continue;for(const auto& support:model.backends){const auto backend_rank=rank(request.backend_preference,support.backend);const auto precision_rank=rank(request.precision_preference,support.precision);if(backend_rank==std::numeric_limits<std::size_t>::max()||precision_rank==std::numeric_limits<std::size_t>::max())continue;const auto score=std::tuple{backend_rank,precision_rank,std::string_view(model.id),std::string_view(model.version)};if(best==nullptr||score<best_score){best=&model;best_score=score;}}}
  return best;
}

std::string ModelRegistry::canonical_manifest()const{std::ostringstream out;out<<"{\"models\":[";std::size_t index=0;for(const auto&[key,model]:models_){static_cast<void>(key);if(index++)out<<',';out<<canonicalize_model_descriptor(model);}out<<"]}";return out.str();}
} // namespace gspl::sprites
