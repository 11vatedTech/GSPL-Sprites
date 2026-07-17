#include "gspl_sprites/core.hpp"
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
  try {
    if (argc != 4 || std::string_view(argv[1]) != "build") { std::cerr << "usage: gspl-sprites build <seed.sprite> <output-directory>\n"; return 2; }
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) { std::cerr << "cannot open input seed\n"; return 2; }
    const std::string source{std::istreambuf_iterator<char>(input), {}};
    const auto seed = gspl::sprites::parse_seed(source);
    const auto result = gspl::sprites::validate(seed);
    if (!result.ok()) { for (const auto& d : result.diagnostics) std::cerr << d.code << ": " << d.message << '\n'; return 1; }
    gspl::sprites::build_package(seed, argv[3]);
    std::cout << "built " << seed.stable_id << " identity=" << gspl::sprites::sha256(gspl::sprites::canonicalize(seed)) << '\n';
    return 0;
  } catch (const std::exception& error) { std::cerr << "GSPL_SPRITES_FATAL: " << error.what() << '\n'; return 1; }
}

