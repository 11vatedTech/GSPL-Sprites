#include "gspl_sprites/image.hpp"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
  try {
    const std::array<std::byte, 17> ppm{std::byte{'P'},std::byte{'6'},std::byte{'\n'},std::byte{'2'},std::byte{' '},std::byte{'1'},std::byte{'\n'},std::byte{'2'},std::byte{'5'},std::byte{'5'},std::byte{'\n'},std::byte{255},std::byte{0},std::byte{0},std::byte{0},std::byte{255},std::byte{0}};
    const auto image = decode_ppm_p6(ppm); check(image.invariant() && image.width == 2 && image.height == 1, "PPM decode failed"); check(image.pixels[0] == 255 && image.pixels[3] == 255 && image.pixels[5] == 255, "PPM channels incorrect"); check(encode_ppm_p6(image) == std::vector<std::byte>(ppm.begin(), ppm.end()), "PPM roundtrip failed");
    bool truncated = false; try { (void)decode_ppm_p6(std::span<const std::byte>(ppm).first(16)); } catch (const std::runtime_error&) { truncated = true; } check(truncated, "truncated raster accepted");
    bool limited = false; try { auto limits = ImageLimits{}; limits.max_pixels = 1; (void)decode_ppm_p6(ppm, limits); } catch (const std::runtime_error&) { limited = true; } check(limited, "pixel limit ignored");

    ImageRgba8 red{2,2,ColorSpace::srgb,AlphaMode::opaque,std::vector<std::uint8_t>(16,255)}; red.pixels[1]=0; red.pixels[2]=0;
    ImageRgba8 blue{1,3,ColorSpace::srgb,AlphaMode::opaque,std::vector<std::uint8_t>(12,255)}; for(std::size_t i=0;i<12;i+=4){blue.pixels[i]=0;blue.pixels[i+1]=0;}
    const std::array frames{FrameSource{"idle.0",red,1,1,2},FrameSource{"idle.1",blue,0,2,3}};
    const auto atlas = pack_atlas(frames, 8, 8, 1); check(atlas.image.invariant() && atlas.placements.size() == 2, "atlas pack failed"); check(atlas.placements[0].frame_id == "idle.0", "placements are not canonical");
    const auto atlas2 = pack_atlas(frames, 8, 8, 1); check(atlas.image.pixels == atlas2.image.pixels && atlas.placements[0].x == atlas2.placements[0].x, "atlas packing is nondeterministic");
    const std::array reversed{frames[1],frames[0]}; const auto atlas3 = pack_atlas(reversed, 8, 8, 1); check(atlas.image.pixels == atlas3.image.pixels && atlas.placements[0].x == atlas3.placements[0].x && atlas.placements[1].y == atlas3.placements[1].y, "input order changed atlas output");
    bool overflow = false; try { (void)pack_atlas(frames, 2, 2, 1); } catch (const std::runtime_error&) { overflow = true; } check(overflow, "atlas overflow accepted");
    bool huge = false; try { (void)pack_atlas(frames, 65535, 65535, 1); } catch (const std::invalid_argument&) { huge = true; } check(huge, "hostile atlas dimensions accepted");

    const auto png=encode_png(red);const auto decoded_png=decode_png(png);check(decoded_png.width==red.width&&decoded_png.height==red.height&&decoded_png.pixels==red.pixels&&decoded_png.color_space==ColorSpace::srgb,"PNG roundtrip failed");
    bool png_limited=false;try{auto limits=ImageLimits{};limits.max_pixels=3;(void)decode_png(png,limits);}catch(const std::runtime_error&){png_limited=true;}check(png_limited,"PNG decoded-pixel limit ignored");
    bool chunk_limited=false;try{auto limits=ImageLimits{};limits.max_chunk_count=2;(void)decode_png(png,limits);}catch(const std::runtime_error&){chunk_limited=true;}check(chunk_limited,"PNG chunk-count limit ignored");
    bool png_truncated=false;try{(void)decode_png(std::span<const std::byte>(png).first(png.size()-1));}catch(const std::runtime_error&){png_truncated=true;}check(png_truncated,"truncated PNG accepted");
    bool bad_signature=false;try{auto invalid=png;invalid[0]=std::byte{0};(void)decode_png(invalid);}catch(const std::runtime_error&){bad_signature=true;}check(bad_signature,"invalid PNG signature accepted");
    bool trailing_data=false;try{auto invalid=png;invalid.push_back(std::byte{0});(void)decode_png(invalid);}catch(const std::runtime_error&){trailing_data=true;}check(trailing_data,"PNG trailing data accepted");
    bool corrupt_critical=false;try{auto invalid=png;const std::array idat_type{std::byte{'I'},std::byte{'D'},std::byte{'A'},std::byte{'T'}};const auto idat=std::search(invalid.begin(),invalid.end(),idat_type.begin(),idat_type.end());check(idat!=invalid.end(),"encoded PNG omitted IDAT chunk");*(idat+4)^=std::byte{1};(void)decode_png(invalid);}catch(const std::runtime_error&){corrupt_critical=true;}check(corrupt_critical,"corrupt critical PNG chunk accepted");
    auto untagged_png=png;const std::array signature{std::byte{'s'},std::byte{'R'},std::byte{'G'},std::byte{'B'}};const auto srgb=std::search(untagged_png.begin(),untagged_png.end(),signature.begin(),signature.end());check(srgb!=untagged_png.end(),"encoded PNG omitted sRGB chunk");const auto chunk_begin=srgb-4;untagged_png.erase(chunk_begin,chunk_begin+13);const auto untagged=decode_png(untagged_png);check(untagged.color_space==ColorSpace::unknown,"untagged PNG silently assumed a color space");bool untagged_atlas=false;try{const std::array untagged_frames{FrameSource{"untagged",untagged,0,0,1}};(void)pack_atlas(untagged_frames,8,8);}catch(const std::invalid_argument&){untagged_atlas=true;}check(untagged_atlas,"untagged image entered atlas");
    bool unknown_encode=false;try{auto unknown=red;unknown.color_space=ColorSpace::unknown;(void)encode_png(unknown);}catch(const std::invalid_argument&){unknown_encode=true;}check(unknown_encode,"untagged image encoded as sRGB");
    auto data_image=red;data_image.color_space=ColorSpace::data;const auto data_png=encode_png(data_image);const auto decoded_data=decode_png(data_png);check(decoded_data.color_space==ColorSpace::unknown&&decoded_data.pixels==data_image.pixels,"data-texture PNG acquired color semantics or changed pixels");

    const AnimationClip clip{"idle",{"idle.0","idle.1"},{2,3},{{"blink",4}},true}; check(validate_animation(clip, atlas.placements).ok(), "valid animation rejected");
    auto bad = clip; bad.frame_ids[1]="missing"; bad.events[0].tick=5; check(!validate_animation(bad, atlas.placements).ok(), "invalid animation accepted");
    std::cout << "all gspl sprites image tests passed\n"; return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
