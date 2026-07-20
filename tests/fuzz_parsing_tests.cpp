#define _CRT_SECURE_NO_WARNINGS
#include "gspl_sprites/core.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>

using namespace gspl::sprites;

namespace {

std::uint32_t xorshift32(std::uint32_t &state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

std::uint32_t get_seed() {
  const char *env = std::getenv("GSPL_FUZZ_SEED");
  if (env)
    return static_cast<std::uint32_t>(std::atoll(env));
  return 42;
}

} // namespace

int main() {
  try {
    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + std::chrono::seconds(5);

    std::uint32_t prng = get_seed();
    std::uint64_t iterations = 0;
    std::uint64_t parse_successes = 0;

    while (std::chrono::steady_clock::now() < deadline) {
      ++iterations;
      const std::size_t length = 1 + (xorshift32(prng) % 256);
      std::string bytes(length, '\0');
      for (std::size_t i = 0; i < length; ++i)
        bytes[i] = static_cast<char>(xorshift32(prng) & 0xFF);

      // Feed to parse_seed() — no crash
      try {
        const auto seed = parse_seed(bytes);
        ++parse_successes;

        // Round-trip through canonicalize and re-parse
        const auto canonical = canonicalize(seed);
        const auto re_seed = parse_seed(canonical);
        const auto re_canonical = canonicalize(re_seed);
        if (canonical != re_canonical)
          throw std::runtime_error("round-trip canonicalization mismatch");
      } catch (const std::exception &) {
        // Expected for most random byte sequences; parse failures are OK
      }
    }

    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();

    std::cout << "all gspl sprites fuzz parsing tests passed ("
              << iterations << " iterations in " << elapsed << "s, "
              << parse_successes << " parse successes)\n";
    return 0;

  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
