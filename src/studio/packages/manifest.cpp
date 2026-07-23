#include "gspl/package/manifest.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace gspl::package {
namespace {

std::string trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\n' || s.front() == '\r'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\n' || s.back() == '\r'))
        s.remove_suffix(1);
    return std::string(s);
}

std::string strip_jsonc_comments(std::string_view src) {
    std::string out;
    out.reserve(src.size());
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '/' && i + 1 < src.size() && src[i + 1] == '/') {
            i += 2;
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }
        if (src[i] == '/' && i + 1 < src.size() && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < src.size() && !(src[i] == '*' && src[i + 1] == '/')) ++i;
            if (i < src.size()) i += 1;
            continue;
        }
        out += src[i];
    }
    return out;
}

class SimpleJsonParser {
    std::string_view src_;
    size_t pos_{};

    void skip_ws() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t' || src_[pos_] == '\n' || src_[pos_] == '\r'))
            ++pos_;
    }

    char peek() { skip_ws(); return pos_ < src_.size() ? src_[pos_] : '\0'; }
    char advance() { skip_ws(); return pos_ < src_.size() ? src_[pos_++] : '\0'; }

    std::string parse_string() {
        if (advance() != '"') return {};
        std::string s;
        while (pos_ < src_.size()) {
            char c = src_[pos_++];
            if (c == '"') return s;
            if (c == '\\' && pos_ < src_.size()) {
                char esc = src_[pos_++];
                switch (esc) {
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    case '/': s += '/'; break;
                    case 'n': s += '\n'; break;
                    case 'r': s += '\r'; break;
                    case 't': s += '\t'; break;
                    case 'u': {
                        if (pos_ + 4 <= src_.size()) {
                            s += "\\u";
                            s += src_.substr(pos_ - 1, 4);
                            pos_ += 4;
                        }
                        break;
                    }
                    default: s += esc;
                }
            } else {
                s += c;
            }
        }
        return s;
    }

public:
    explicit SimpleJsonParser(std::string_view src) : src_(strip_jsonc_comments(src)) {}

    std::string parse_value_as_string() {
        char c = peek();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') {
            if (src_.substr(pos_, 4) == "true") { pos_ += 4; return "true"; }
            if (src_.substr(pos_, 5) == "false") { pos_ += 5; return "false"; }
            return {};
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            size_t end = pos_;
            if (end < src_.size() && src_[end] == '-') ++end;
            while (end < src_.size() && std::isdigit(static_cast<unsigned char>(src_[end]))) ++end;
            if (end < src_.size() && src_[end] == '.') {
                ++end;
                while (end < src_.size() && std::isdigit(static_cast<unsigned char>(src_[end]))) ++end;
            }
            if (end < src_.size() && (src_[end] == 'e' || src_[end] == 'E')) {
                ++end;
                if (end < src_.size() && (src_[end] == '+' || src_[end] == '-')) ++end;
                while (end < src_.size() && std::isdigit(static_cast<unsigned char>(src_[end]))) ++end;
            }
            std::string num(src_.substr(pos_, end - pos_));
            pos_ = end;
            return num;
        }
        if (c == 'n') { pos_ += 4; return "null"; }
        return {};
    }

    bool parse_member(std::string& key, std::string& value) {
        if (peek() != '"') return false;
        key = parse_string();
        if (advance() != ':') return false;
        value = parse_value_as_string();
        return true;
    }

    std::vector<std::pair<std::string, std::string>> parse_object() {
        std::vector<std::pair<std::string, std::string>> members;
        if (advance() != '{') return members;
        if (peek() == '}') { advance(); return members; }
        for (;;) {
            std::string key, value;
            if (!parse_member(key, value)) break;
            members.emplace_back(std::move(key), std::move(value));
            if (peek() == ',') { advance(); continue; }
            break;
        }
        if (peek() == '}') advance();
        return members;
    }

    std::vector<std::string> parse_string_array() {
        std::vector<std::string> items;
        if (advance() != '[') return items;
        if (peek() == ']') { advance(); return items; }
        for (;;) {
            items.push_back(parse_string());
            if (peek() == ',') { advance(); continue; }
            break;
        }
        if (peek() == ']') advance();
        return items;
    }

    std::vector<std::pair<std::string, std::string>> parse_object_array() {
        std::vector<std::pair<std::string, std::string>> items;
        if (advance() != '[') return items;
        if (peek() == ']') { advance(); return items; }
        for (;;) {
            auto obj = parse_object();
            std::string pkg_id, version;
            for (auto& [k, v] : obj) {
                if (k == "package_id") pkg_id = v;
                else if (k == "version_constraint" || k == "version") version = v;
            }
            items.emplace_back(std::move(pkg_id), std::move(version));
            if (peek() == ',') { advance(); continue; }
            break;
        }
        if (peek() == ']') advance();
        return items;
    }

    std::vector<std::string> parse_mixed_array() {
        std::vector<std::string> items;
        if (advance() != '[') return items;
        if (peek() == ']') { advance(); return items; }
        for (;;) {
            if (peek() == '"') items.push_back(parse_string());
            else if (peek() == '{') {
                auto& item = items.emplace_back();
                auto obj = parse_object();
                for (auto& [k, v] : obj) {
                    if (k == "package_id") { item = v; break; }
                }
            } else {
                items.push_back(parse_value_as_string());
            }
            if (peek() == ',') { advance(); continue; }
            break;
        }
        if (peek() == ']') advance();
        return items;
    }
};

bool is_valid_semver(std::string_view v) {
    static const std::regex semver_re(R"(^(\d+)\.(\d+)\.(\d+)(-[a-zA-Z0-9.]+)?(\+[a-zA-Z0-9.]+)?$)");
    return std::regex_match(v.begin(), v.end(), semver_re);
}

} // anonymous namespace

std::string PackageManifest::id() const {
    return publisher + "/" + package_name;
}

bool PackageManifest::validate() const {
    if (publisher.empty() || package_name.empty()) return false;
    if (!is_valid_semver(version)) return false;
    if (gspl_version.empty()) return false;
    if (!dependencies.empty()) {
        for (auto const& dep : dependencies) {
            if (dep.package_id.empty()) return false;
            if (dep.package_id.find('/') == std::string_view::npos) return false;
        }
    }
    return true;
}

bool PackageManifest::has_signature() const {
    return !signature.empty();
}

auto PackageManifest::load(const std::string& path) -> std::optional<PackageManifest> {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (content.empty()) return std::nullopt;

    SimpleJsonParser parser(content);
    auto members = parser.parse_object();

    PackageManifest mf;
    for (auto& [key, value] : members) {
        if (key == "publisher") mf.publisher = value;
        else if (key == "package_name" || key == "name") mf.package_name = value;
        else if (key == "version") mf.version = value;
        else if (key == "display_name") mf.display_name = value;
        else if (key == "description") mf.description = value;
        else if (key == "license") mf.license = value;
        else if (key == "gspl_version") mf.gspl_version = value;
        else if (key == "description_mime_type") mf.description_mime_type = value;
        else if (key == "repository") mf.repository = value;
        else if (key == "homepage") mf.homepage = value;
        else if (key == "signature") mf.signature = value;
    }

    if (mf.publisher.empty() || mf.package_name.empty()) return std::nullopt;

    // Parse array fields using position tracking on the stripped content
    {
        std::string clean = strip_jsonc_comments(content);
        auto find_key = [&](const std::string& target) -> std::string {
            size_t start = 0;
            while (true) {
                auto key_pos = clean.find("\"" + target + "\"", start);
                if (key_pos == std::string::npos) return {};
                auto colon = clean.find(':', key_pos);
                if (colon == std::string::npos) return {};
                auto val_start = clean.find_first_not_of(" \t\n\r", colon + 1);
                if (val_start == std::string::npos) return {};
                if (clean[val_start] == '[') {
                    // Array value - extract raw
                    int depth = 0;
                    size_t arr_end = val_start;
                    for (; arr_end < clean.size(); ++arr_end) {
                        if (clean[arr_end] == '[') ++depth;
                        if (clean[arr_end] == ']') { --depth; if (depth == 0) break; }
                    }
                    return clean.substr(val_start, arr_end - val_start + 1);
                }
                start = colon + 1;
            }
        };

        auto parse_array_of_strings = [](const std::string& arr_str) -> std::vector<std::string> {
            std::vector<std::string> result;
            if (arr_str.empty() || arr_str.front() != '[') return result;
            size_t i = 1;
            while (i < arr_str.size() && arr_str[i] != ']') {
                while (i < arr_str.size() && (arr_str[i] == ' ' || arr_str[i] == '\t' || arr_str[i] == '\n' || arr_str[i] == '\r' || arr_str[i] == ','))
                    ++i;
                if (i >= arr_str.size() || arr_str[i] == ']') break;
                if (arr_str[i] == '"') {
                    ++i;
                    std::string s;
                    while (i < arr_str.size() && arr_str[i] != '"') {
                        if (arr_str[i] == '\\' && i + 1 < arr_str.size()) {
                            s += arr_str[i + 1];
                            i += 2;
                        } else {
                            s += arr_str[i++];
                        }
                    }
                    if (i < arr_str.size()) ++i;
                    result.push_back(s);
                }
            }
            return result;
        };

        auto parse_deps = [](const std::string& arr_str) -> std::vector<PackageDependency> {
            std::vector<PackageDependency> result;
            if (arr_str.empty() || arr_str.front() != '[') return result;
            size_t i = 1;
            while (i < arr_str.size() && arr_str[i] != ']') {
                while (i < arr_str.size() && (arr_str[i] == ' ' || arr_str[i] == '\t' || arr_str[i] == '\n' || arr_str[i] == '\r' || arr_str[i] == ','))
                    ++i;
                if (i >= arr_str.size() || arr_str[i] == ']') break;
                if (arr_str[i] == '{') {
                    ++i;
                    PackageDependency dep;
                    while (i < arr_str.size() && arr_str[i] != '}') {
                        while (i < arr_str.size() && (arr_str[i] == ' ' || arr_str[i] == '\t' || arr_str[i] == '\n' || arr_str[i] == '\r'))
                            ++i;
                        if (i >= arr_str.size() || arr_str[i] == '}') break;
                        auto key_start = arr_str.find('"', i);
                        if (key_start == std::string::npos || key_start >= arr_str.size()) break;
                        auto key_end = arr_str.find('"', key_start + 1);
                        if (key_end == std::string::npos) break;
                        std::string k = arr_str.substr(key_start + 1, key_end - key_start - 1);
                        i = key_end + 1;
                        auto colon = arr_str.find(':', i);
                        if (colon == std::string::npos) break;
                        i = colon + 1;
                        while (i < arr_str.size() && (arr_str[i] == ' ' || arr_str[i] == '\t' || arr_str[i] == '\n' || arr_str[i] == '\r'))
                            ++i;
                        if (i < arr_str.size() && arr_str[i] == '"') {
                            ++i;
                            std::string val;
                            while (i < arr_str.size() && arr_str[i] != '"') {
                                if (arr_str[i] == '\\' && i + 1 < arr_str.size()) {
                                    val += arr_str[i + 1];
                                    i += 2;
                                } else {
                                    val += arr_str[i++];
                                }
                            }
                            if (i < arr_str.size()) ++i;
                            if (k == "package_id") dep.package_id = val;
                            else if (k == "version_constraint" || k == "version") dep.version_constraint = val;
                        }
                        while (i < arr_str.size() && arr_str[i] != ',' && arr_str[i] != '}') ++i;
                    }
                    if (i < arr_str.size()) ++i;
                    result.push_back(std::move(dep));
                }
            }
            return result;
        };

        auto authors_str = find_key("authors");
        if (!authors_str.empty()) mf.authors = parse_array_of_strings(authors_str);

        auto deps_str = find_key("dependencies");
        if (!deps_str.empty()) mf.dependencies = parse_deps(deps_str);

        auto tags_str = find_key("tags");
        if (!tags_str.empty()) mf.tags = parse_array_of_strings(tags_str);
    }

    return mf;
}

std::string PackageManifest::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"publisher\": \"" << publisher << "\",\n";
    oss << "  \"package_name\": \"" << package_name << "\",\n";
    oss << "  \"version\": \"" << version << "\",\n";
    oss << "  \"display_name\": \"" << display_name << "\",\n";
    oss << "  \"description\": \"" << description << "\",\n";
    oss << "  \"license\": \"" << license << "\",\n";
    oss << "  \"gspl_version\": \"" << gspl_version << "\",\n";
    oss << "  \"description_mime_type\": \"" << description_mime_type << "\"";

    if (!authors.empty()) {
        oss << ",\n  \"authors\": [";
        for (size_t i = 0; i < authors.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "\"" << authors[i] << "\"";
        }
        oss << "]";
    }

    if (!dependencies.empty()) {
        oss << ",\n  \"dependencies\": [\n";
        for (size_t i = 0; i < dependencies.size(); ++i) {
            if (i > 0) oss << ",\n";
            oss << "    { \"package_id\": \"" << dependencies[i].package_id
                << "\", \"version_constraint\": \"" << dependencies[i].version_constraint << "\" }";
        }
        oss << "\n  ]";
    }

    if (!tags.empty()) {
        oss << ",\n  \"tags\": [";
        for (size_t i = 0; i < tags.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "\"" << tags[i] << "\"";
        }
        oss << "]";
    }

    if (!repository.empty()) {
        oss << ",\n  \"repository\": \"" << repository << "\"";
    }
    if (!homepage.empty()) {
        oss << ",\n  \"homepage\": \"" << homepage << "\"";
    }
    if (!signature.empty()) {
        oss << ",\n  \"signature\": \"" << signature << "\"";
    }

    oss << "\n}\n";
    return oss.str();
}

} // namespace gspl::package
