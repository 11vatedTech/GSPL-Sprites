#include "gspl_sprites/image.hpp"

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

    const AnimationClip clip{"idle",{"idle.0","idle.1"},{2,3},{{"blink",4}},true}; check(validate_animation(clip, atlas.placements).ok(), "valid animation rejected");
    auto bad = clip; bad.frame_ids[1]="missing"; bad.events[0].tick=5; check(!validate_animation(bad, atlas.placements).ok(), "invalid animation accepted");
    std::cout << "all gspl sprites image tests passed\n"; return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

