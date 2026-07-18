#include "gspl_sprites/authoring.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace gspl::sprites {
namespace {
constexpr std::size_t max_bytes = 1024 * 1024;
constexpr std::size_t max_lines = 16'384;
constexpr std::size_t max_line_bytes = 16 * 1024;

std::vector<std::string_view> split(std::string_view value, char delimiter) {
  std::vector<std::string_view> result;
  std::size_t start = 0;
  for (;;) {
    const auto end = value.find(delimiter, start);
    result.push_back(value.substr(start, end == std::string_view::npos
                                             ? value.size() - start
                                             : end - start));
    if (end == std::string_view::npos)
      break;
    start = end + 1;
  }
  return result;
}

bool safe(unsigned char value) {
  return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
         (value >= '0' && value <= '9') || value == '.' || value == '_' ||
         value == '-';
}

bool valid_utf8(std::string_view value) {
  for (std::size_t index = 0; index < value.size();) {
    const auto first = static_cast<unsigned char>(value[index]);
    std::size_t length = 1;
    std::uint32_t codepoint = first;
    if ((first & 0xe0U) == 0xc0U) {
      length = 2;
      codepoint = first & 0x1fU;
    } else if ((first & 0xf0U) == 0xe0U) {
      length = 3;
      codepoint = first & 0x0fU;
    } else if ((first & 0xf8U) == 0xf0U) {
      length = 4;
      codepoint = first & 0x07U;
    } else if (first >= 0x80U) {
      return false;
    }
    if (index + length > value.size())
      return false;
    for (std::size_t offset = 1; offset < length; ++offset) {
      const auto continuation =
          static_cast<unsigned char>(value[index + offset]);
      if ((continuation & 0xc0U) != 0x80U)
        return false;
      codepoint = (codepoint << 6U) | (continuation & 0x3fU);
    }
    if ((length == 2 && codepoint < 0x80U) ||
        (length == 3 && codepoint < 0x800U) ||
        (length == 4 && codepoint < 0x10000U) ||
        (codepoint >= 0xd800U && codepoint <= 0xdfffU) || codepoint > 0x10ffffU)
      return false;
    index += length;
  }
  return true;
}

std::string encode(std::string_view value) {
  constexpr char hex[] = "0123456789ABCDEF";
  std::string result;
  for (const unsigned char character : value) {
    if (safe(character)) {
      result += static_cast<char>(character);
    } else {
      result += '%';
      result += hex[character >> 4U];
      result += hex[character & 0x0fU];
    }
  }
  return result;
}

std::uint8_t nibble(char value) {
  if (value >= '0' && value <= '9')
    return static_cast<std::uint8_t>(value - '0');
  if (value >= 'A' && value <= 'F')
    return static_cast<std::uint8_t>(value - 'A' + 10);
  throw std::runtime_error("authoring source contains a non-canonical escape");
}

std::string decode(std::string_view value) {
  std::string result;
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] != '%') {
      if (!safe(static_cast<unsigned char>(value[index])))
        throw std::runtime_error("authoring source contains an unescaped byte");
      result += value[index];
      continue;
    }
    if (index + 2 >= value.size())
      throw std::runtime_error("authoring source contains a truncated escape");
    const auto byte = static_cast<unsigned char>(
        (nibble(value[index + 1]) << 4U) | nibble(value[index + 2]));
    if (safe(byte))
      throw std::runtime_error(
          "authoring source contains an unnecessary escape");
    result += static_cast<char>(byte);
    index += 2;
  }
  if (!valid_utf8(result))
    throw std::runtime_error("authoring source value is not valid UTF-8");
  return result;
}

template <class Integer>
Integer integer(std::string_view value, std::string_view label) {
  Integer result{};
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), result);
  if (value.empty() || error != std::errc{} ||
      end != value.data() + value.size())
    throw std::runtime_error("invalid authoring integer: " +
                             std::string(label));
  return result;
}

bool boolean(std::string_view value) {
  if (value == "0")
    return false;
  if (value == "1")
    return true;
  throw std::runtime_error("invalid authoring boolean");
}

std::string read_bounded(const std::filesystem::path &path) {
  if (!std::filesystem::is_regular_file(path) ||
      std::filesystem::is_symlink(std::filesystem::symlink_status(path)))
    throw std::runtime_error("authoring project path is absent or unsafe");
  const auto size = std::filesystem::file_size(path);
  if (size > max_bytes)
    throw std::runtime_error("authoring project exceeds byte limit");
  std::string result(static_cast<std::size_t>(size), '\0');
  std::ifstream input(path, std::ios::binary);
  input.read(result.data(), static_cast<std::streamsize>(result.size()));
  if (!input && !result.empty())
    throw std::runtime_error("failed reading complete authoring project");
  return result;
}
} // namespace

std::string serialize_authoring_project(const AuthoringProject &project) {
  const auto validation = validate_authoring_project(project);
  if (!validation.ok())
    throw std::invalid_argument(validation.diagnostics.front().message);
  auto fields = project.fields;
  auto abilities = project.abilities;
  auto variants = project.variants;
  std::ranges::sort(fields, {}, &AuthoringField::path);
  std::ranges::sort(abilities, [](const auto &left, const auto &right) {
    return left.value.id < right.value.id;
  });
  std::ranges::sort(variants, {}, &AuthoringVariant::id);
  std::ostringstream output;
  output << "schema=gspl.sprite-authoring/0.1\nid=" << encode(project.id)
         << "\nintent=" << encode(project.intent)
         << "\nrevision=" << project.revision << "\nparent="
         << (project.parent_revision_identity
                 ? *project.parent_revision_identity
                 : "-")
         << '\n';
  for (const auto &field : fields) {
    output << "field=" << field.path << '|' << (field.locked ? '1' : '0')
           << '|';
    if (field.selected)
      output << *field.selected;
    else
      output << '-';
    output << '|';
    for (std::size_t index = 0; index < field.alternatives.size(); ++index) {
      if (index)
        output << ';';
      output << encode(field.alternatives[index]);
    }
    output << '\n';
  }
  for (const auto &ability : abilities)
    output << "ability=" << ability.value.id << '|'
           << encode(ability.value.effect) << '|' << ability.value.cost << '|'
           << ability.value.cooldown_ticks << '|' << ability.value.active_ticks
           << '|' << (ability.enabled ? '1' : '0') << '|'
           << (ability.locked ? '1' : '0') << '\n';
  for (auto &variant : variants) {
    output << "variant=" << variant.id << '\n';
    std::ranges::sort(variant.field_overrides);
    std::ranges::sort(variant.disabled_abilities);
    for (const auto &[path, value] : variant.field_overrides)
      output << "variant_field=" << variant.id << '|' << path << '|'
             << encode(value) << '\n';
    for (const auto &id : variant.disabled_abilities)
      output << "variant_disable=" << variant.id << '|' << id << '\n';
  }
  const auto result = output.str();
  if (result.size() > max_bytes)
    throw std::length_error("serialized authoring project exceeds byte limit");
  return result;
}

AuthoringProject parse_authoring_project(std::string_view source) {
  if (source.empty() || source.size() > max_bytes)
    throw std::runtime_error("authoring source is empty or exceeds byte limit");
  AuthoringProject project;
  project.schema.clear();
  std::map<std::string, std::size_t, std::less<>> variants;
  std::set<std::string> singleton;
  std::size_t cursor = 0;
  std::size_t line_number = 0;
  while (cursor < source.size()) {
    if (++line_number > max_lines)
      throw std::runtime_error("authoring source exceeds line limit");
    const auto end = source.find('\n', cursor);
    const auto line = source.substr(cursor, end == std::string_view::npos
                                                ? source.size() - cursor
                                                : end - cursor);
    cursor = end == std::string_view::npos ? source.size() : end + 1;
    if (line.empty())
      continue;
    if (line.size() > max_line_bytes || line.back() == '\r')
      throw std::runtime_error(
          "authoring source line is oversized or non-canonical");
    const auto separator = line.find('=');
    if (separator == std::string_view::npos)
      throw std::runtime_error("authoring source record lacks '='");
    const auto key = line.substr(0, separator);
    const auto value = line.substr(separator + 1);
    if (key == "schema" || key == "id" || key == "intent" ||
        key == "revision" || key == "parent") {
      if (!singleton.insert(std::string(key)).second)
        throw std::runtime_error("duplicate authoring singleton record");
      if (key == "schema")
        project.schema = std::string(value);
      else if (key == "id")
        project.id = decode(value);
      else if (key == "intent")
        project.intent = decode(value);
      else if (key == "revision")
        project.revision = integer<std::uint64_t>(value, key);
      else if (value != "-")
        project.parent_revision_identity = std::string(value);
    } else if (key == "field") {
      const auto parts = split(value, '|');
      if (parts.size() != 4)
        throw std::runtime_error("authoring field requires four parts");
      AuthoringField field{std::string(parts[0]), {}, {}, boolean(parts[1])};
      if (parts[2] != "-")
        field.selected = integer<std::uint32_t>(parts[2], "selection");
      for (const auto alternative : split(parts[3], ';'))
        field.alternatives.push_back(decode(alternative));
      project.fields.push_back(std::move(field));
    } else if (key == "ability") {
      const auto parts = split(value, '|');
      if (parts.size() != 7)
        throw std::runtime_error("authoring ability requires seven parts");
      project.abilities.push_back(
          {{std::string(parts[0]), decode(parts[1]),
            integer<std::uint32_t>(parts[2], "ability cost"),
            integer<std::uint32_t>(parts[3], "ability cooldown"),
            integer<std::uint32_t>(parts[4], "ability active ticks")},
           boolean(parts[5]),
           boolean(parts[6])});
    } else if (key == "variant") {
      project.variants.push_back({std::string(value), {}, {}});
      if (!variants
               .emplace(project.variants.back().id, project.variants.size() - 1)
               .second)
        throw std::runtime_error("duplicate authoring variant record");
    } else if (key == "variant_field" || key == "variant_disable") {
      const auto parts = split(value, '|');
      if (parts.size() != (key == "variant_field" ? 3U : 2U))
        throw std::runtime_error("authoring variant record has wrong arity");
      const auto found = variants.find(parts[0]);
      if (found == variants.end())
        throw std::runtime_error("authoring variant must precede its records");
      auto &variant = project.variants[found->second];
      if (key == "variant_field")
        variant.field_overrides.emplace_back(parts[1], decode(parts[2]));
      else
        variant.disabled_abilities.emplace_back(parts[1]);
    } else {
      throw std::runtime_error("unknown authoring source record: " +
                               std::string(key));
    }
  }
  constexpr std::array singleton_keys{"schema", "id", "intent", "revision",
                                      "parent"};
  for (const auto key : singleton_keys)
    if (!singleton.contains(key))
      throw std::runtime_error("authoring source singleton is absent: " +
                               std::string(key));
  const auto validation = validate_authoring_project(project);
  if (!validation.ok())
    throw std::runtime_error(validation.diagnostics.front().code + ": " +
                             validation.diagnostics.front().message);
  if (serialize_authoring_project(project) != source)
    throw std::runtime_error("authoring source is not canonical");
  return project;
}

void save_authoring_project(const AuthoringProject &project,
                            const std::filesystem::path &path) {
  if (path.empty() || std::filesystem::exists(path))
    throw std::runtime_error(
        "authoring destination is empty or already exists");
  auto staging = path;
  staging += ".staging";
  if (std::filesystem::exists(staging))
    throw std::runtime_error("authoring staging path already exists");
  const auto source = serialize_authoring_project(project);
  try {
    std::ofstream output(staging, std::ios::binary | std::ios::trunc);
    output.write(source.data(), static_cast<std::streamsize>(source.size()));
    if (!output)
      throw std::runtime_error("failed writing authoring staging file");
    output.close();
    static_cast<void>(load_authoring_project(staging));
    std::filesystem::rename(staging, path);
  } catch (...) {
    std::error_code ignored;
    std::filesystem::remove(staging, ignored);
    throw;
  }
}

AuthoringProject load_authoring_project(const std::filesystem::path &path) {
  return parse_authoring_project(read_bounded(path));
}

} // namespace gspl::sprites
