#include "gspl_sprites/image.hpp"

#include <spng.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>

namespace gspl::sprites {
namespace {
using Context = std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)>;
void require(int result, std::string_view operation) { if(result!=0)throw std::runtime_error(std::string(operation)+": "+spng_strerror(result)); }
std::uint32_t big_endian_u32(std::span<const std::byte> bytes,std::size_t offset){return(std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset]))<<24)|(std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset+1]))<<16)|(std::uint32_t(std::to_integer<std::uint8_t>(bytes[offset+2]))<<8)|std::to_integer<std::uint8_t>(bytes[offset+3]);}
void preflight_png(std::span<const std::byte> encoded,const ImageLimits& limits){
  static constexpr std::array signature{std::byte{0x89},std::byte{'P'},std::byte{'N'},std::byte{'G'},std::byte{'\r'},std::byte{'\n'},std::byte{0x1a},std::byte{'\n'}};
  if(encoded.size()<signature.size()||!std::equal(signature.begin(),signature.end(),encoded.begin()))throw std::runtime_error("invalid PNG signature");
  std::size_t cursor=signature.size();std::uint32_t chunks=0;bool ended=false;
  while(cursor<encoded.size()){
    if(encoded.size()-cursor<12)throw std::runtime_error("truncated PNG chunk envelope");
    if(++chunks>limits.max_chunk_count)throw std::runtime_error("PNG chunk count exceeds limit");
    const auto length=big_endian_u32(encoded,cursor);const auto available=encoded.size()-cursor-12;if(length>available)throw std::runtime_error("truncated PNG chunk payload");
    const char t0=static_cast<char>(std::to_integer<unsigned char>(encoded[cursor+4]));const char t1=static_cast<char>(std::to_integer<unsigned char>(encoded[cursor+5]));const char t2=static_cast<char>(std::to_integer<unsigned char>(encoded[cursor+6]));const char t3=static_cast<char>(std::to_integer<unsigned char>(encoded[cursor+7]));cursor+=static_cast<std::size_t>(length)+12;
    if(t0=='I'&&t1=='E'&&t2=='N'&&t3=='D'){if(length!=0||cursor!=encoded.size())throw std::runtime_error("PNG IEND is malformed or not final");ended=true;break;}
  }
  if(!ended)throw std::runtime_error("PNG IEND chunk is missing");
}
}

ImageRgba8 decode_png(std::span<const std::byte> encoded,const ImageLimits& limits){
  if(encoded.empty()||encoded.size()>limits.max_input_bytes)throw std::runtime_error("PNG input is empty or exceeds byte limit");
  if(limits.max_chunk_count==0)throw std::invalid_argument("PNG chunk-count limit must be positive");
  preflight_png(encoded,limits);
  Context context(spng_ctx_new(0),&spng_ctx_free);if(!context)throw std::runtime_error("failed to allocate PNG decoder");
  require(spng_set_image_limits(context.get(),limits.max_width,limits.max_height),"set PNG image limits");
  const auto metadata_limit=static_cast<std::size_t>(std::min<std::uint64_t>(limits.max_metadata_bytes,std::numeric_limits<std::size_t>::max()));
  const auto chunk_limit=std::min<std::size_t>(metadata_limit,4U*1024U*1024U);
  require(spng_set_chunk_limits(context.get(),chunk_limit,metadata_limit),"set PNG chunk limits");
  require(spng_set_crc_action(context.get(),SPNG_CRC_ERROR,SPNG_CRC_DISCARD),"set PNG CRC policy");
  require(spng_set_png_buffer(context.get(),encoded.data(),encoded.size()),"set PNG input");
  spng_ihdr header{};require(spng_get_ihdr(context.get(),&header),"read PNG header");
  const auto pixels=static_cast<std::uint64_t>(header.width)*header.height;if(header.width==0||header.height==0||pixels>limits.max_pixels||pixels>limits.max_decoded_bytes/4ULL)throw std::runtime_error("decoded PNG dimensions exceed limits");
  std::size_t decoded_size{};require(spng_decoded_image_size(context.get(),SPNG_FMT_RGBA8,&decoded_size),"calculate PNG decoded size");if(decoded_size!=pixels*4ULL||decoded_size>limits.max_decoded_bytes)throw std::runtime_error("decoded PNG byte size is invalid");
  ImageRgba8 image{header.width,header.height,ColorSpace::unknown,AlphaMode::straight,std::vector<std::uint8_t>(decoded_size)};
  require(spng_decode_image(context.get(),image.pixels.data(),image.pixels.size(),SPNG_FMT_RGBA8,SPNG_DECODE_TRNS),"decode PNG pixels");
  std::uint8_t rendering_intent{};if(spng_get_srgb(context.get(),&rendering_intent)==0)image.color_space=ColorSpace::srgb;
  bool opaque=true;for(std::size_t index=3;index<image.pixels.size();index+=4)if(image.pixels[index]!=255){opaque=false;break;}image.alpha_mode=opaque?AlphaMode::opaque:AlphaMode::straight;
  return image;
}

std::vector<std::byte> encode_png(const ImageRgba8& image){
  if(!image.invariant())throw std::invalid_argument("invalid image for PNG encoding");
  if(image.color_space!=ColorSpace::srgb&&image.color_space!=ColorSpace::data)throw std::invalid_argument("PNG encoder requires explicit sRGB color or data-texture semantics");
  Context context(spng_ctx_new(SPNG_CTX_ENCODER),&spng_ctx_free);if(!context)throw std::runtime_error("failed to allocate PNG encoder");
  require(spng_set_option(context.get(),SPNG_ENCODE_TO_BUFFER,1),"enable PNG buffer encoding");
  spng_ihdr header{};header.width=image.width;header.height=image.height;header.bit_depth=8;header.color_type=SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;header.compression_method=0;header.filter_method=0;header.interlace_method=0;
  require(spng_set_ihdr(context.get(),&header),"set PNG header");if(image.color_space==ColorSpace::srgb)require(spng_set_srgb(context.get(),0),"set PNG sRGB intent");
  require(spng_encode_image(context.get(),image.pixels.data(),image.pixels.size(),SPNG_FMT_PNG,SPNG_ENCODE_FINALIZE),"encode PNG pixels");
  std::size_t size{};int error{};void* buffer=spng_get_png_buffer(context.get(),&size,&error);if(buffer==nullptr)require(error,"retrieve encoded PNG");std::unique_ptr<void,decltype(&std::free)> owned(buffer,&std::free);if(size==0)throw std::runtime_error("PNG encoder returned empty output");
  const auto* bytes=static_cast<const std::byte*>(buffer);return{bytes,bytes+size};
}
} // namespace gspl::sprites
