#include "gspl_sprites/core.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace gspl::sprites;
namespace { void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main() {
  try {
    check(sha256("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", "SHA-256 empty vector failed");
    check(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", "SHA-256 abc vector failed");
    const std::string text = "schema=gspl.sprite-seed/0.1\nid=original.test\nname=Test\nclassification=fictional\nrights=ORIGINAL_USER_CREATION\nentropy_root=7\nprimary_color=#112233\naccent_color=#AABBCC\nability=arc|electric.projectile|20|4|2\n";
    const auto seed = parse_seed(text);
    check(validate(seed).ok(), "valid seed rejected");
    check(canonicalize(seed) == canonicalize(parse_seed(text)), "canonicalization not deterministic");
    const auto ir = compile(seed); check(ir.seed_identity.size() == 64, "identity is not SHA-256");
    RuntimeEntity runtime; check(activate(runtime, seed.abilities[0]), "ability did not activate"); check(runtime.energy == 80, "ability cost not applied"); check(!activate(runtime, seed.abilities[0]), "concurrent ability activation allowed"); tick(runtime); tick(runtime); tick(runtime); check(runtime.state == RuntimeState::idle, "runtime failed to recover");
    auto prohibited = seed; prohibited.rights = RightsClass::prohibited; check(!validate(prohibited).ok(), "prohibited rights exported");
    bool duplicate_rejected = false; try { (void)parse_seed(text + "name=Again\n"); } catch (const std::runtime_error&) { duplicate_rejected = true; } check(duplicate_rejected, "duplicate field accepted");
    bool oversized_rejected = false; try { (void)parse_seed(std::string((1U << 20) + 1, 'x')); } catch (const std::runtime_error&) { oversized_rejected = true; } check(oversized_rejected, "oversized source accepted");
    const auto output = std::filesystem::temp_directory_path() / "gspl-sprites-core-test"; std::filesystem::remove_all(output); std::filesystem::remove_all(output.string() + ".staging"); build_package(seed, output); check(std::filesystem::exists(output / "manifest.json"), "manifest missing"); check(std::filesystem::exists(output / "assets" / "entity.svg"), "projection missing"); check(std::filesystem::exists(output / "asset-graph.json"), "asset graph missing"); check(std::filesystem::exists(output / "provenance.json"), "provenance missing"); check(std::filesystem::exists(output / "rights.json"), "rights decision missing");
    bool overwrite_rejected = false; try { build_package(seed, output); } catch (const std::runtime_error&) { overwrite_rejected = true; } check(overwrite_rejected, "existing package overwritten"); std::filesystem::remove_all(output);
    std::cout << "all gspl sprites core tests passed\n"; return 0;
  } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
