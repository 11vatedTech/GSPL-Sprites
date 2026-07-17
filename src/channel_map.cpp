#include "gspl_sprites/channel_map.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

namespace gspl::sprites {
namespace {
bool stable_id(std::string_view value){return!value.empty()&&value.size()<=256&&std::ranges::all_of(value,[](unsigned char c){return std::isalnum(c)!=0||c=='.'||c=='_'||c=='-';});}
std::string_view kind_text(ChannelMapKind kind){switch(kind){case ChannelMapKind::material_id:return"material_id";case ChannelMapKind::tangent_normal:return"tangent_normal";case ChannelMapKind::depth:return"depth";case ChannelMapKind::emissive:return"emissive";case ChannelMapKind::effects:return"effects";case ChannelMapKind::collision:return"collision";}throw std::logic_error("unreachable channel-map kind");}
}

ValidationResult validate_channel_map(const ChannelMap& map,const FrameSource& target){
  ValidationResult result;const auto add=[&](std::string code,std::string message){result.diagnostics.push_back({std::move(code),std::move(message)});};if(!stable_id(map.id)||!stable_id(map.target_frame_id)||map.target_frame_id!=target.id)add("SPRITE_CHANNEL_ID_INVALID","channel-map identity or target is invalid");if(!map.image.invariant()||!target.image.invariant()||map.image.width!=target.image.width||map.image.height!=target.image.height)add("SPRITE_CHANNEL_DIMENSIONS_INVALID","channel map and target frame must have equal valid dimensions");const bool color_map=map.kind==ChannelMapKind::emissive;if(map.image.color_space!=(color_map?ColorSpace::srgb:ColorSpace::data))add("SPRITE_CHANNEL_COLOR_SEMANTICS_INVALID",color_map?"emissive maps require explicit sRGB":"non-color channel maps require explicit data semantics");if(!map.image.invariant())return result;
  bool alpha_opaque=true,grayscale=true,binary=true,material_encoding=true,normals_unit=true;for(std::size_t offset=0;offset<map.image.pixels.size();offset+=4){const auto r=map.image.pixels[offset],g=map.image.pixels[offset+1],b=map.image.pixels[offset+2],a=map.image.pixels[offset+3];alpha_opaque=alpha_opaque&&a==255;grayscale=grayscale&&r==g&&g==b;binary=binary&&(r==0||r==255);material_encoding=material_encoding&&g==0&&b==0;const auto x=2*static_cast<int>(r)-255,y=2*static_cast<int>(g)-255,z=2*static_cast<int>(b)-255;const auto length=x*x+y*y+z*z;normals_unit=normals_unit&&length>=52'020&&length<=78'030&&b>=128;}
  if(!alpha_opaque)add("SPRITE_CHANNEL_ALPHA_INVALID","channel maps require fully opaque storage alpha");if((map.kind==ChannelMapKind::depth||map.kind==ChannelMapKind::effects||map.kind==ChannelMapKind::collision)&&!grayscale)add("SPRITE_CHANNEL_GRAYSCALE_REQUIRED","scalar channel map must use equal RGB channels");if(map.kind==ChannelMapKind::collision&&!binary)add("SPRITE_CHANNEL_BINARY_REQUIRED","collision mask must contain only 0 or 255");if(map.kind==ChannelMapKind::material_id&&!material_encoding)add("SPRITE_CHANNEL_MATERIAL_ENCODING_INVALID","material IDs must be stored in R with G and B zero");if(map.kind==ChannelMapKind::tangent_normal&&!normals_unit)add("SPRITE_CHANNEL_NORMAL_INVALID","tangent normals must face outward and have near-unit length");return result;
}

std::string canonicalize_channel_maps(std::span<const ChannelMap> maps){if(maps.size()>24576)throw std::invalid_argument("channel-map count exceeds limit");std::vector<const ChannelMap*> ordered;ordered.reserve(maps.size());std::set<std::pair<std::string,ChannelMapKind>> keys;for(const auto&map:maps){if(!stable_id(map.id)||!stable_id(map.target_frame_id)||!map.image.invariant()||!keys.emplace(map.target_frame_id,map.kind).second)throw std::invalid_argument("channel-map set is malformed or duplicate");ordered.push_back(&map);}std::ranges::sort(ordered,[](const auto*left,const auto*right){return std::tuple{left->target_frame_id,left->kind,left->id}<std::tuple{right->target_frame_id,right->kind,right->id};});std::ostringstream output;output<<"{\"maps\":[";for(std::size_t i=0;i<ordered.size();++i){if(i)output<<',';const auto&map=*ordered[i];output<<"{\"height\":"<<map.image.height<<",\"id\":\""<<map.id<<"\",\"kind\":\""<<kind_text(map.kind)<<"\",\"targetFrameId\":\""<<map.target_frame_id<<"\",\"width\":"<<map.image.width<<'}';}output<<"],\"schema\":\"gspl.sprite-channel-maps/0.1\"}";return output.str();}
} // namespace gspl::sprites
