#include "gspl_sprites/core.hpp"
#include "gspl_sprites/domain.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
std::string trim(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

std::vector<std::string> split(std::string_view input, char delimiter) {
  std::vector<std::string> result;
  std::size_t begin = 0;
  while (begin <= input.size()) {
    const auto end = input.find(delimiter, begin);
    result.emplace_back(input.substr(begin, end == std::string_view::npos ? input.size() - begin : end - begin));
    if (end == std::string_view::npos) break;
    begin = end + 1;
  }
  return result;
}

std::uint32_t parse_u32(std::string_view value, std::string_view field) {
  std::uint32_t result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{} || ptr != value.data() + value.size()) throw std::runtime_error("invalid unsigned integer for " + std::string(field));
  return result;
}

std::uint64_t parse_u64(std::string_view value, std::string_view field) {
  std::uint64_t result{};
  const auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result);
  if (ec != std::errc{} || ptr != value.data() + value.size()) throw std::runtime_error("invalid unsigned integer for " + std::string(field));
  return result;
}

RightsClass parse_rights(std::string_view value) {
  static const std::map<std::string_view, RightsClass> values{
    {"ORIGINAL_USER_CREATION", RightsClass::original_user_creation}, {"USER_OWNED_REFERENCE", RightsClass::user_owned},
    {"LICENSED_REFERENCE", RightsClass::licensed}, {"PUBLIC_DOMAIN", RightsClass::public_domain},
    {"PERMISSIVELY_LICENSED", RightsClass::permissive}, {"RESEARCH_ONLY_REFERENCE", RightsClass::research_only},
    {"RESTRICTED_REFERENCE", RightsClass::restricted}, {"UNKNOWN_RIGHTS", RightsClass::unknown}, {"PROHIBITED", RightsClass::prohibited}};
  const auto found = values.find(value);
  if (found == values.end()) throw std::runtime_error("unknown rights classification");
  return found->second;
}

std::string rights_text(RightsClass value) {
  switch (value) {
    case RightsClass::original_user_creation: return "ORIGINAL_USER_CREATION";
    case RightsClass::user_owned: return "USER_OWNED_REFERENCE";
    case RightsClass::licensed: return "LICENSED_REFERENCE";
    case RightsClass::public_domain: return "PUBLIC_DOMAIN";
    case RightsClass::permissive: return "PERMISSIVELY_LICENSED";
    case RightsClass::research_only: return "RESEARCH_ONLY_REFERENCE";
    case RightsClass::restricted: return "RESTRICTED_REFERENCE";
    case RightsClass::unknown: return "UNKNOWN_RIGHTS";
    case RightsClass::prohibited: return "PROHIBITED";
  }
  throw std::logic_error("unreachable rights classification");
}

std::string escape_json(std::string_view value) {
  std::string out;
  for (const unsigned char c : value) {
    switch (c) { case '\\': out += "\\\\"; break; case '"': out += "\\\""; break; case '\n': out += "\\n"; break; case '\r': out += "\\r"; break; case '\t': out += "\\t"; break; default: if (c < 0x20) throw std::runtime_error("control character is not canonicalizable"); else out += static_cast<char>(c); }
  }
  return out;
}

constexpr std::array<std::uint32_t, 64> k{
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
constexpr std::uint32_t rotr(std::uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
}

SpriteSeed parse_seed(std::string_view source) {
  constexpr std::size_t max_source_bytes = 1U << 20;
  if (source.size() > max_source_bytes) throw std::runtime_error("source exceeds 1 MiB limit");
  SpriteSeed seed;
  std::istringstream stream{std::string(source)};
  std::string line;
  std::size_t line_number = 0;
  std::map<std::string, bool> seen;
  while (std::getline(stream, line)) {
    ++line_number;
    if (line_number > 4096) throw std::runtime_error("source exceeds 4096-line limit");
    if (line.size() > 8192) throw std::runtime_error("line exceeds 8192-byte limit");
    line = trim(line);
    if (line.empty() || line.starts_with('#')) continue;
    const auto equals = line.find('=');
    if (equals == std::string::npos) throw std::runtime_error("line " + std::to_string(line_number) + ": expected key=value");
    const auto key = trim(line.substr(0, equals));
    const auto value = trim(line.substr(equals + 1));
    if (key != "ability" && !seen.emplace(key, true).second) throw std::runtime_error("line " + std::to_string(line_number) + ": duplicate field " + key);
    if (key == "schema") seed.schema = value;
    else if (key == "id") seed.stable_id = value;
    else if (key == "name") seed.name = value;
    else if (key == "classification") seed.classification = value;
    else if (key == "rights") seed.rights = parse_rights(value);
    else if (key == "entropy_root") seed.entropy_root = parse_u64(value, key);
    else if (key == "primary_color") seed.primary_color = value;
    else if (key == "accent_color") seed.accent_color = value;
    else if (key == "ability") {
      if (seed.abilities.size() >= 64) throw std::runtime_error("ability count exceeds 64");
      const auto parts = split(value, '|');
      if (parts.size() != 5) throw std::runtime_error("line " + std::to_string(line_number) + ": ability requires id|effect|cost|cooldown|active");
      seed.abilities.push_back({parts[0], parts[1], parse_u32(parts[2], "ability.cost"), parse_u32(parts[3], "ability.cooldown"), parse_u32(parts[4], "ability.active")});
    } else throw std::runtime_error("line " + std::to_string(line_number) + ": unknown field " + key);
  }
  return seed;
}

ValidationResult validate(const SpriteSeed& seed) {
  ValidationResult result;
  auto require = [&](bool condition, std::string code, std::string message) { if (!condition) result.diagnostics.push_back({std::move(code), std::move(message)}); };
  require(seed.schema == "gspl.sprite-seed/0.1", "SPRITE_SCHEMA_UNSUPPORTED", "schema must be gspl.sprite-seed/0.1");
  require(!seed.stable_id.empty() && seed.stable_id.size() <= 128, "SPRITE_ID_INVALID", "stable id must contain 1..128 bytes");
  require(!seed.name.empty() && seed.name.size() <= 128, "SPRITE_NAME_INVALID", "name must contain 1..128 bytes");
  require(!seed.classification.empty(), "SPRITE_CLASSIFICATION_REQUIRED", "classification is required");
  require(seed.rights != RightsClass::unknown && seed.rights != RightsClass::prohibited && seed.rights != RightsClass::research_only && seed.rights != RightsClass::restricted, "SPRITE_RIGHTS_EXPORT_DENIED", "rights classification does not permit production export");
  const auto color_ok = [](const std::string& c) { return c.size() == 7 && c[0] == '#' && std::all_of(c.begin() + 1, c.end(), [](unsigned char x){ return std::isxdigit(x) != 0; }); };
  require(color_ok(seed.primary_color) && color_ok(seed.accent_color), "SPRITE_COLOR_INVALID", "colors must use #RRGGBB");
  require(!seed.abilities.empty() && seed.abilities.size() <= 64, "SPRITE_ABILITIES_INVALID", "entity requires 1..64 abilities");
  for (const auto& ability : seed.abilities) {
    require(!ability.id.empty() && !ability.effect.empty(), "SPRITE_ABILITY_INVALID", "ability id and semantic effect are required");
    require(ability.cost <= 100 && ability.active_ticks > 0 && ability.active_ticks <= 600 && ability.cooldown_ticks <= 36000, "SPRITE_ABILITY_BOUNDS", "ability cost or timing exceeds bounds");
  }
  return result;
}

std::string canonicalize(const SpriteSeed& seed) {
  std::vector<AbilitySeed> abilities = seed.abilities;
  std::ranges::sort(abilities, {}, &AbilitySeed::id);
  std::ostringstream out;
  out << "{\"abilities\":[";
  for (std::size_t i = 0; i < abilities.size(); ++i) {
    if (i) out << ',';
    const auto& a = abilities[i];
    out << "{\"activeTicks\":" << a.active_ticks << ",\"cooldownTicks\":" << a.cooldown_ticks << ",\"cost\":" << a.cost << ",\"effect\":\"" << escape_json(a.effect) << "\",\"id\":\"" << escape_json(a.id) << "\"}";
  }
  out << "],\"classification\":\"" << escape_json(seed.classification) << "\",\"colors\":{\"accent\":\"" << seed.accent_color << "\",\"primary\":\"" << seed.primary_color << "\"},\"entropyRoot\":" << seed.entropy_root << ",\"id\":\"" << escape_json(seed.stable_id) << "\",\"name\":\"" << escape_json(seed.name) << "\",\"rights\":\"" << rights_text(seed.rights) << "\",\"schema\":\"" << seed.schema << "\"}";
  return out.str();
}

std::string sha256(std::string_view input) {
  std::vector<std::uint8_t> data(input.begin(), input.end());
  const auto bit_length = static_cast<std::uint64_t>(data.size()) * 8;
  data.push_back(0x80);
  while ((data.size() % 64) != 56) data.push_back(0);
  for (int i = 7; i >= 0; --i) data.push_back(static_cast<std::uint8_t>(bit_length >> (i * 8)));
  std::array<std::uint32_t, 8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  for (std::size_t offset = 0; offset < data.size(); offset += 64) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) w[i] = (std::uint32_t(data[offset+i*4])<<24)|(std::uint32_t(data[offset+i*4+1])<<16)|(std::uint32_t(data[offset+i*4+2])<<8)|data[offset+i*4+3];
    for (std::size_t i = 16; i < 64; ++i) { const auto s0=rotr(w[i-15],7)^rotr(w[i-15],18)^(w[i-15]>>3); const auto s1=rotr(w[i-2],17)^rotr(w[i-2],19)^(w[i-2]>>10); w[i]=w[i-16]+s0+w[i-7]+s1; }
    auto [a,b,c,d,e,f,g,hh]=h;
    for (std::size_t i=0;i<64;++i){const auto s1=rotr(e,6)^rotr(e,11)^rotr(e,25);const auto ch=(e&f)^((~e)&g);const auto t1=hh+s1+ch+k[i]+w[i];const auto s0=rotr(a,2)^rotr(a,13)^rotr(a,22);const auto maj=(a&b)^(a&c)^(b&c);const auto t2=s0+maj;hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;}
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
  }
  std::ostringstream out; out << std::hex << std::setfill('0'); for (const auto value : h) out << std::setw(8) << value; return out.str();
}

SpriteIr compile(const SpriteSeed& seed) {
  const auto validation = validate(seed);
  if (!validation.ok()) throw std::runtime_error(validation.diagnostics.front().code + ": " + validation.diagnostics.front().message);
  const GeneRegistry registry;
  const auto genes = genes_from_seed(seed);
  const auto gene_validation = registry.validate(genes);
  if (!gene_validation.ok()) throw std::runtime_error(gene_validation.diagnostics.front().code + ": " + gene_validation.diagnostics.front().message);
  return {sha256(canonicalize(seed)), seed.stable_id, seed.abilities, seed.primary_color, seed.accent_color};
}

bool activate(RuntimeEntity& entity, const AbilitySeed& ability) {
  if (entity.state != RuntimeState::idle || entity.cooldown_ticks != 0 || entity.energy < ability.cost) return false;
  entity.energy -= ability.cost; entity.state = RuntimeState::attacking; entity.remaining_ticks = ability.active_ticks; entity.cooldown_ticks = ability.cooldown_ticks; return true;
}

void tick(RuntimeEntity& entity) noexcept {
  if (entity.cooldown_ticks > 0) --entity.cooldown_ticks;
  if (entity.remaining_ticks > 0 && --entity.remaining_ticks == 0) entity.state = RuntimeState::recovering;
  else if (entity.state == RuntimeState::recovering) entity.state = RuntimeState::idle;
}

std::string render_svg(const SpriteIr& ir) {
  std::ostringstream out;
  out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"256\" height=\"256\" viewBox=\"0 0 256 256\" role=\"img\" aria-label=\"" << ir.entity_id << "\">"
      << "<g fill=\"" << ir.primary_color << "\" stroke=\"" << ir.accent_color << "\" stroke-width=\"8\" stroke-linejoin=\"round\">"
      << "<path d=\"M52 122 28 62l62 34c25-18 55-18 78 0l60-34-22 64c12 18 10 49-7 67-21 23-112 23-137 0-17-18-20-48-10-71Z\"/>"
      << "<path fill=\"none\" d=\"m106 137 18 17 27-33-10 31 20 5-35 39 11-31-25-5Z\"/>"
      << "<circle cx=\"91\" cy=\"128\" r=\"7\"/><circle cx=\"169\" cy=\"128\" r=\"7\"/></g></svg>";
  return out.str();
}

void build_package(const SpriteSeed& seed, const std::filesystem::path& output) {
  const auto ir = compile(seed); const auto canonical = canonicalize(seed); const auto svg = render_svg(ir);
  const auto rights = evaluate_rights(seed.rights, AssetUsage::commercial_export);
  if (!rights.allowed) throw std::runtime_error(rights.code + ": " + rights.explanation);
  if (output.empty()) throw std::invalid_argument("output path must not be empty");
  if (std::filesystem::exists(output)) throw std::runtime_error("output already exists; refusing to overwrite: " + output.string());
  auto staging = output; staging += ".staging";
  if (std::filesystem::exists(staging)) throw std::runtime_error("staging path already exists: " + staging.string());
  std::filesystem::create_directories(staging / "assets");
  auto write = [](const auto& path, std::string_view bytes) { std::ofstream file(path, std::ios::binary); if (!file) throw std::runtime_error("cannot write " + path.string()); file.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); if (!file) throw std::runtime_error("failed writing " + path.string()); };
  try {
    const ProvenanceRecord seed_provenance{"prov-seed-" + ir.seed_identity, ProvenanceActor::user, "user", "author-seed/1", {}, ir.seed_identity};
    AssetGraph graph;
    const auto seed_artifact = graph.add("application/vnd.gspl.sprite-seed+json", canonical, {}, "canonicalize-seed/1", seed_provenance.id, "portable", ArtifactValidation::valid);
    const ProvenanceRecord svg_provenance{"prov-svg-" + sha256(svg), ProvenanceActor::compiler, "gspl-sprites/0.1.0", "render-svg/1", {seed_artifact}, sha256(svg)};
    (void)graph.add("image/svg+xml", svg, {seed_artifact}, "render-svg/1", svg_provenance.id, "svg", ArtifactValidation::valid);
    write(staging / "seed.canonical.json", canonical); write(staging / "assets" / "entity.svg", svg);
    write(staging / "asset-graph.json", graph.canonical_manifest());
    write(staging / "provenance.json", "{\"records\":[" + canonical_provenance(seed_provenance) + "," + canonical_provenance(svg_provenance) + "]}");
    write(staging / "rights.json", "{\"classification\":\"" + rights_text(seed.rights) + "\",\"commercialExport\":true,\"decisionCode\":\"" + rights.code + "\"}");
    std::ostringstream manifest; manifest << "{\"entityId\":\"" << ir.entity_id << "\",\"format\":\"gspl.sprite-package/0.1\",\"seedIdentity\":\"" << ir.seed_identity << "\",\"artifacts\":[{\"path\":\"assets/entity.svg\",\"sha256\":\"" << sha256(svg) << "\"}],\"assetGraph\":\"asset-graph.json\",\"provenance\":\"provenance.json\",\"rights\":\"rights.json\"}";
    write(staging / "manifest.json", manifest.str());
    std::filesystem::rename(staging, output);
  } catch (...) {
    std::error_code ignored; std::filesystem::remove_all(staging, ignored); throw;
  }
}
} // namespace gspl::sprites
