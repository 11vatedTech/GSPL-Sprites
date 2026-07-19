#include "gspl_sprites/sprite2d.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
void require_image(const ImageRgba8& image) {
  if (!image.invariant() || image.color_space == ColorSpace::unknown) throw std::invalid_argument("2D pass requires a valid image with explicit color space");
  if (image.alpha_mode == AlphaMode::premultiplied) throw std::invalid_argument("2D pass requires straight or opaque alpha");
}

std::uint32_t rgba_at(const ImageRgba8& image, std::size_t offset) {
  return (static_cast<std::uint32_t>(image.pixels[offset]) << 24) |
         (static_cast<std::uint32_t>(image.pixels[offset + 1]) << 16) |
         (static_cast<std::uint32_t>(image.pixels[offset + 2]) << 8) |
         image.pixels[offset + 3];
}

std::uint64_t distance(std::uint32_t left, std::uint32_t right) {
  std::uint64_t result = 0;
  for (unsigned shift : {24U, 16U, 8U, 0U}) {
    const auto a = static_cast<int>((left >> shift) & 0xffU); const auto b = static_cast<int>((right >> shift) & 0xffU);
    result += static_cast<std::uint64_t>((a - b) * (a - b));
  }
  return result;
}

std::string escape_json(std::string_view value) {
  std::string output;
  for (const unsigned char character : value) {
    if (character == '"' || character == '\\') { output.push_back('\\'); output.push_back(static_cast<char>(character)); }
    else if (character < 0x20) throw std::invalid_argument("atlas frame id contains a control character");
    else output.push_back(static_cast<char>(character));
  }
  return output;
}
} // namespace

TrimmedFrame trim_transparent(const FrameSource& source, std::uint8_t alpha_threshold) {
  require_image(source.image); if (source.id.empty() || source.duration_ticks == 0) throw std::invalid_argument("trim source frame is invalid");
  std::uint32_t left=source.image.width,top=source.image.height,right=0,bottom=0;bool found=false;
  for(std::uint32_t y=0;y<source.image.height;++y)for(std::uint32_t x=0;x<source.image.width;++x){const auto alpha=source.image.pixels[(static_cast<std::size_t>(y)*source.image.width+x)*4ULL+3];if(alpha>alpha_threshold){left=std::min(left,x);top=std::min(top,y);right=std::max(right,x);bottom=std::max(bottom,y);found=true;}}
  if(!found)throw std::invalid_argument("cannot trim a frame with no foreground pixels");
  const auto width=right-left+1;const auto height=bottom-top+1;ImageRgba8 image{width,height,source.image.color_space,source.image.alpha_mode,std::vector<std::uint8_t>(static_cast<std::size_t>(width)*height*4ULL)};
  for(std::uint32_t y=0;y<height;++y){const auto begin=(static_cast<std::size_t>(top+y)*source.image.width+left)*4ULL;const auto target=static_cast<std::size_t>(y)*width*4ULL;std::ranges::copy_n(source.image.pixels.begin()+static_cast<std::ptrdiff_t>(begin),static_cast<std::size_t>(width)*4ULL,image.pixels.begin()+static_cast<std::ptrdiff_t>(target));}
  return{{source.id,std::move(image),source.pivot_x-static_cast<std::int32_t>(left),source.pivot_y-static_cast<std::int32_t>(top),source.duration_ticks},left,top,source.image.width,source.image.height};
}

FrameSource scale_nearest(const FrameSource& source, std::uint32_t integer_scale, const ImageLimits& limits) {
  require_image(source.image);if(integer_scale==0||integer_scale>64)throw std::invalid_argument("nearest-neighbor scale must be 1..64");
  const auto width=static_cast<std::uint64_t>(source.image.width)*integer_scale;const auto height=static_cast<std::uint64_t>(source.image.height)*integer_scale;const auto pixels=width*height;
  if(width>limits.max_width||height>limits.max_height||pixels>limits.max_pixels||pixels>limits.max_decoded_bytes/4ULL||pixels>std::numeric_limits<std::size_t>::max()/4ULL)throw std::invalid_argument("scaled frame exceeds image limits");
  ImageRgba8 output{static_cast<std::uint32_t>(width),static_cast<std::uint32_t>(height),source.image.color_space,source.image.alpha_mode,std::vector<std::uint8_t>(static_cast<std::size_t>(pixels)*4ULL)};
  for(std::uint32_t y=0;y<output.height;++y)for(std::uint32_t x=0;x<output.width;++x){const auto input=(static_cast<std::size_t>(y/integer_scale)*source.image.width+x/integer_scale)*4ULL;const auto target=(static_cast<std::size_t>(y)*output.width+x)*4ULL;std::ranges::copy_n(source.image.pixels.begin()+static_cast<std::ptrdiff_t>(input),4,output.pixels.begin()+static_cast<std::ptrdiff_t>(target));}
  const auto scaled_pivot_x=static_cast<std::int64_t>(source.pivot_x)*integer_scale;const auto scaled_pivot_y=static_cast<std::int64_t>(source.pivot_y)*integer_scale;if(scaled_pivot_x<std::numeric_limits<std::int32_t>::min()||scaled_pivot_x>std::numeric_limits<std::int32_t>::max()||scaled_pivot_y<std::numeric_limits<std::int32_t>::min()||scaled_pivot_y>std::numeric_limits<std::int32_t>::max())throw std::invalid_argument("scaled pivot exceeds integer range");
  return{source.id,std::move(output),static_cast<std::int32_t>(scaled_pivot_x),static_cast<std::int32_t>(scaled_pivot_y),source.duration_ticks};
}

std::vector<PaletteEntry> extract_palette(const ImageRgba8& image, std::uint32_t maximum_entries) {
  require_image(image);if(maximum_entries==0||maximum_entries>65536)throw std::invalid_argument("palette entry limit must be 1..65536");std::map<std::uint32_t,std::uint64_t> counts;
  for(std::size_t offset=0;offset<image.pixels.size();offset+=4){const auto color=rgba_at(image,offset);if((color&0xffU)!=0)++counts[color];}
  std::vector<PaletteEntry> result;result.reserve(std::min<std::size_t>(counts.size(),maximum_entries));for(const auto&[color,count]:counts)result.push_back({color,count});std::ranges::sort(result,[](const auto&left,const auto&right){return left.occurrences!=right.occurrences?left.occurrences>right.occurrences:left.rgba<right.rgba;});if(result.size()>maximum_entries)result.resize(maximum_entries);return result;
}

ImageRgba8 remap_palette(const ImageRgba8& image, std::span<const std::uint32_t> palette) {
  require_image(image);if(palette.empty()||palette.size()>256)throw std::invalid_argument("pixel palette must contain 1..256 entries");std::set<std::uint32_t> unique(palette.begin(),palette.end());if(unique.size()!=palette.size())throw std::invalid_argument("pixel palette contains duplicates");ImageRgba8 output=image;
  for(std::size_t offset=0;offset<output.pixels.size();offset+=4){const auto source=rgba_at(output,offset);if((source&0xffU)==0){output.pixels[offset]=0;output.pixels[offset+1]=0;output.pixels[offset+2]=0;continue;}const auto chosen=*std::ranges::min_element(palette,[&](auto left,auto right){const auto ld=distance(source,left);const auto rd=distance(source,right);return ld!=rd?ld<rd:left<right;});output.pixels[offset]=static_cast<std::uint8_t>(chosen>>24);output.pixels[offset+1]=static_cast<std::uint8_t>(chosen>>16);output.pixels[offset+2]=static_cast<std::uint8_t>(chosen>>8);output.pixels[offset+3]=static_cast<std::uint8_t>(chosen);}
  return output;
}

ImageRgba8 alpha_mask(const ImageRgba8& image, std::uint8_t threshold) {
  require_image(image);ImageRgba8 output{image.width,image.height,ColorSpace::srgb,AlphaMode::opaque,std::vector<std::uint8_t>(image.pixels.size())};for(std::size_t offset=0;offset<image.pixels.size();offset+=4){const auto value=image.pixels[offset+3]>threshold?std::uint8_t{255}:std::uint8_t{0};output.pixels[offset]=value;output.pixels[offset+1]=value;output.pixels[offset+2]=value;output.pixels[offset+3]=255;}return output;
}

ImageRgba8 outline_mask(const ImageRgba8& image, std::uint8_t threshold) {
  require_image(image);ImageRgba8 output{image.width,image.height,ColorSpace::srgb,AlphaMode::opaque,std::vector<std::uint8_t>(image.pixels.size(),0)};for(std::size_t offset=3;offset<output.pixels.size();offset+=4)output.pixels[offset]=255;
  const auto occupied=[&](std::int64_t x,std::int64_t y){return x>=0&&y>=0&&x<image.width&&y<image.height&&image.pixels[(static_cast<std::size_t>(y)*image.width+static_cast<std::size_t>(x))*4ULL+3]>threshold;};
  for(std::uint32_t y=0;y<image.height;++y)for(std::uint32_t x=0;x<image.width;++x){if(occupied(x,y))continue;if(occupied(static_cast<std::int64_t>(x)-1,y)||occupied(static_cast<std::int64_t>(x)+1,y)||occupied(x,static_cast<std::int64_t>(y)-1)||occupied(x,static_cast<std::int64_t>(y)+1)){const auto offset=(static_cast<std::size_t>(y)*image.width+x)*4ULL;output.pixels[offset]=255;output.pixels[offset+1]=255;output.pixels[offset+2]=255;}}
  return output;
}

ValidationResult validate_pixel_art(const ImageRgba8& image, const PixelArtPolicy& policy) {
  ValidationResult result;const auto add=[&](std::string code,std::string message){result.diagnostics.push_back({std::move(code),std::move(message)});};if(!image.invariant()||image.color_space==ColorSpace::unknown)add("SPRITE_PIXEL_IMAGE_INVALID","pixel-art image must be valid and tagged");if(policy.grid_width==0||policy.grid_height==0||policy.maximum_colors==0||policy.maximum_colors>256)add("SPRITE_PIXEL_POLICY_INVALID","pixel-art grid and palette policy is invalid");if(!result.ok())return result;if(image.width%policy.grid_width!=0||image.height%policy.grid_height!=0)add("SPRITE_PIXEL_GRID_MISALIGNED","image dimensions are not aligned to the declared grid");std::set<std::uint32_t> colors;bool fractional_alpha=false;for(std::size_t offset=0;offset<image.pixels.size();offset+=4){const auto alpha=image.pixels[offset+3];fractional_alpha=fractional_alpha||(policy.binary_alpha&&alpha!=0&&alpha!=255);if(alpha!=0&&colors.size()<=policy.maximum_colors)colors.insert(rgba_at(image,offset));}if(fractional_alpha)add("SPRITE_PIXEL_ALPHA_NONBINARY","pixel art contains unintended fractional alpha");if(colors.size()>policy.maximum_colors)add("SPRITE_PIXEL_PALETTE_EXCEEDED","pixel art exceeds its fixed palette budget");return result;
}

std::string canonicalize_atlas_metadata(const AtlasResult& atlas) {
  require_image(atlas.image);auto placements=atlas.placements;std::ranges::sort(placements,{},&AtlasPlacement::frame_id);std::set<std::string> ids;std::ostringstream out;out<<"{\"frames\":[";for(std::size_t i=0;i<placements.size();++i){const auto&p=placements[i];if(p.frame_id.empty()||!ids.insert(p.frame_id).second||p.width==0||p.height==0||p.x>atlas.image.width||p.y>atlas.image.height||p.width>atlas.image.width-p.x||p.height>atlas.image.height-p.y||p.duration_ticks==0)throw std::invalid_argument("atlas placement is invalid");if(i)out<<',';out<<"{\"durationTicks\":"<<p.duration_ticks<<",\"height\":"<<p.height<<",\"id\":\""<<escape_json(p.frame_id)<<"\",\"pivotX\":"<<p.pivot_x<<",\"pivotY\":"<<p.pivot_y<<",\"width\":"<<p.width<<",\"x\":"<<p.x<<",\"y\":"<<p.y<<'}';}out<<"],\"height\":"<<atlas.image.height<<",\"schema\":\"gspl.sprite-atlas/0.1\",\"width\":"<<atlas.image.width<<'}';return out.str();
}

SpriteSheetArtifacts compile_sprite_sheet(std::span<const FrameSource> frames, const SpriteSheetOptions& options) {
  if(frames.empty()||frames.size()>4096||options.maximum_width==0||options.maximum_height==0||options.maximum_width>16384||options.maximum_height>16384||options.padding>64)throw std::invalid_argument("sprite-sheet options or frame count are invalid");
  std::vector<FrameSource> prepared;prepared.reserve(frames.size());
  for(const auto&frame:frames){if(options.trim_frames)prepared.push_back(trim_transparent(frame,options.alpha_threshold).frame);else{require_image(frame.image);prepared.push_back(frame);}}
  auto atlas=pack_atlas(prepared,options.maximum_width,options.maximum_height,options.padding);auto metadata=canonicalize_atlas_metadata(atlas);auto alpha=alpha_mask(atlas.image,options.alpha_threshold);auto outline=outline_mask(atlas.image,options.alpha_threshold);return{std::move(atlas),std::move(alpha),std::move(outline),std::move(metadata)};
}

std::vector<TemporalTransitionMetrics> analyze_temporal_stability(std::span<const FrameSource> frames,std::uint8_t threshold,std::uint64_t maximum_pixels){
  if(frames.size()<2||maximum_pixels==0||maximum_pixels>std::numeric_limits<std::uint64_t>::max()/1'000'000ULL)throw std::invalid_argument("temporal analysis requires at least two frames and a bounded positive comparison limit");
  for(const auto&frame:frames){require_image(frame.image);if(frame.id.empty())throw std::invalid_argument("temporal frame identity is empty");}
  std::vector<TemporalTransitionMetrics> result;result.reserve(frames.size()-1);
  for(std::size_t index=1;index<frames.size();++index){const auto&left_frame=frames[index-1];const auto&right_frame=frames[index];const auto left_x=std::min(-static_cast<std::int64_t>(left_frame.pivot_x),-static_cast<std::int64_t>(right_frame.pivot_x));const auto top_y=std::min(-static_cast<std::int64_t>(left_frame.pivot_y),-static_cast<std::int64_t>(right_frame.pivot_y));const auto right_x=std::max(static_cast<std::int64_t>(left_frame.image.width)-left_frame.pivot_x,static_cast<std::int64_t>(right_frame.image.width)-right_frame.pivot_x);const auto bottom_y=std::max(static_cast<std::int64_t>(left_frame.image.height)-left_frame.pivot_y,static_cast<std::int64_t>(right_frame.image.height)-right_frame.pivot_y);if(right_x<=left_x||bottom_y<=top_y)throw std::invalid_argument("temporal comparison bounds are invalid");const auto width=static_cast<std::uint64_t>(right_x-left_x);const auto height=static_cast<std::uint64_t>(bottom_y-top_y);if(width>maximum_pixels||height>maximum_pixels/width)throw std::invalid_argument("temporal comparison exceeds pixel limit");TemporalTransitionMetrics metrics{left_frame.id,right_frame.id,width*height};
    const auto sample=[&](const FrameSource&frame,std::int64_t x,std::int64_t y){const auto local_x=x+frame.pivot_x;const auto local_y=y+frame.pivot_y;if(local_x<0||local_y<0||local_x>=static_cast<std::int64_t>(frame.image.width)||local_y>=static_cast<std::int64_t>(frame.image.height))return std::uint32_t{};const auto offset=(static_cast<std::size_t>(local_y)*frame.image.width+static_cast<std::size_t>(local_x))*4ULL;const auto packed=rgba_at(frame.image,offset);return(packed&0xffU)>threshold?packed:std::uint32_t{};};
    for(auto y=top_y;y<bottom_y;++y)for(auto x=left_x;x<right_x;++x){const auto left=sample(left_frame,x,y);const auto right=sample(right_frame,x,y);if(left!=right)++metrics.changed_pixels;const bool left_occupied=left!=0,right_occupied=right!=0;if(left_occupied||right_occupied)++metrics.silhouette_union_pixels;if(left_occupied&&right_occupied)++metrics.silhouette_intersection_pixels;}
    metrics.changed_per_million=static_cast<std::uint32_t>((metrics.changed_pixels*1'000'000ULL+metrics.compared_pixels/2)/metrics.compared_pixels);metrics.silhouette_iou_per_million=metrics.silhouette_union_pixels==0?1'000'000U:static_cast<std::uint32_t>((metrics.silhouette_intersection_pixels*1'000'000ULL+metrics.silhouette_union_pixels/2)/metrics.silhouette_union_pixels);result.push_back(std::move(metrics));
  }return result;
}
} // namespace gspl::sprites
