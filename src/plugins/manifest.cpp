#include "gspl/plugin/manifest.hpp"
#include "gspl/plugin/plugin_api.h"
#include <fstream>
#include <sstream>
#include <regex>
#include <cctype>

namespace gspl::plugin {
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

    std::string parse_string_value() {
        return parse_string();
    }

    bool parse_bool_value() {
        if (src_.substr(pos_, 4) == "true") { pos_ += 4; return true; }
        if (src_.substr(pos_, 5) == "false") { pos_ += 5; return false; }
        return false;
    }

    double parse_number_value() {
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
        std::string num_str(src_.substr(pos_, end - pos_));
        pos_ = end;
        return std::stod(num_str);
    }

    std::string parse_value_as_string() {
        char c = peek();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool_value() ? "true" : "false";
        if (c == '-' || (c >= '0' && c <= '9')) return std::to_string(parse_number_value());
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
            std::string id, version;
            for (auto& [k, v] : obj) {
                if (k == "id") id = v;
                else if (k == "version_constraint" || k == "version") version = v;
            }
            items.emplace_back(std::move(id), std::move(version));
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

auto PluginManifest::load(const std::string& path) -> std::optional<PluginManifest> {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (content.empty()) return std::nullopt;

    SimpleJsonParser parser(content);
    auto members = parser.parse_object();

    PluginManifest mf;
    for (auto& [key, value] : members) {
        if (key == "id") mf.id = value;
        else if (key == "version") mf.version = value;
        else if (key == "name") mf.name = value;
        else if (key == "description") mf.description = value;
        else if (key == "author") mf.author = value;
        else if (key == "license") mf.license = value;
        else if (key == "api_version") {
            try { mf.api_version = static_cast<uint32_t>(std::stoul(value)); }
            catch (...) {}
        }
    }

    if (mf.id.empty() || mf.version.empty()) return std::nullopt;
    return mf;
}

bool PluginManifest::validate() const {
    if (id.empty()) return false;
    if (!is_valid_semver(version)) return false;
    if (api_version == 0 || api_version > GSPL_PLUGIN_API_VERSION) return false;
    for (auto const& dep : dependencies) {
        if (dep.id.empty()) return false;
    }
    return true;
}

std::string PluginManifest::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"id\": \"" << id << "\",\n";
    oss << "  \"version\": \"" << version << "\",\n";
    oss << "  \"name\": \"" << name << "\",\n";
    oss << "  \"description\": \"" << description << "\",\n";
    oss << "  \"author\": \"" << author << "\",\n";
    oss << "  \"license\": \"" << license << "\",\n";
    oss << "  \"api_version\": " << api_version;
    if (!dependencies.empty()) {
        oss << ",\n  \"dependencies\": [\n";
        for (size_t i = 0; i < dependencies.size(); ++i) {
            if (i > 0) oss << ",\n";
            oss << "    { \"id\": \"" << dependencies[i].id
                << "\", \"version_constraint\": \"" << dependencies[i].version_constraint << "\" }";
        }
        oss << "\n  ]";
    }
    if (!capabilities.empty()) {
        oss << ",\n  \"capabilities\": [\n";
        for (size_t i = 0; i < capabilities.size(); ++i) {
            if (i > 0) oss << ",\n";
            oss << "    \"" << capabilities[i] << "\"";
        }
        oss << "\n  ]";
    }
    oss << "\n}\n";
    return oss.str();
}

} // namespace gspl::plugin
