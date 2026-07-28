#include "gspl/json.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <charconv>
#include <cstdlib>
#include <sstream>

static_assert(std::variant_size_v<gspl::GeneValue> == 6,
              "GeneValue variant size must match GeneValueTag enum count");

namespace gspl {

// ===================================================================
// JSON writer helpers
// ===================================================================

std::string json_escape(std::string_view sv) {
    std::ostringstream out;
    for (auto c : sv) {
        switch (c) {
        case '\"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char hex[8];
                std::snprintf(hex, sizeof(hex), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                out << hex;
            } else {
                out << c;
            }
        }
    }
    return out.str();
}

void json_append_key_string(std::ostringstream& ss, std::string_view key,
                            std::string_view value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": \"" << json_escape(value) << "\"";
}

void json_append_key_int(std::ostringstream& ss, std::string_view key,
                         std::int64_t value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << value;
}

void json_append_key_uint(std::ostringstream& ss, std::string_view key,
                          std::uint64_t value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << value;
}

void json_append_key_double(std::ostringstream& ss, std::string_view key,
                            double value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    if (!std::isfinite(value)) {
        ss << "  \"" << json_escape(key) << "\": null";
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.17g", value);
        ss << "  \"" << json_escape(key) << "\": " << buf;
    }
}

void json_append_key_bool(std::ostringstream& ss, std::string_view key,
                          bool value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << (value ? "true" : "false");
}

// ===================================================================
// BoundedJsonReader
// ===================================================================

BoundedJsonReader::BoundedJsonReader(std::string_view src, BoundedJsonConfig cfg)
    : src_(src), cfg_(cfg) {
    if (src_.size() > cfg_.max_input_bytes) {
        set_error("input exceeds max_input_bytes (" +
                  std::to_string(cfg_.max_input_bytes) + ")");
    }
}

void BoundedJsonReader::set_error(std::string msg) {
    if (!has_error_) {
        has_error_ = true;
        error_ = std::move(msg);
    }
}

void BoundedJsonReader::enter_object() {
    ++depth_;
    if (depth_ > cfg_.max_nesting_depth) {
        set_error("nesting depth exceeds max_nesting_depth (" +
                  std::to_string(cfg_.max_nesting_depth) + ")");
    }
}

void BoundedJsonReader::leave_object() {
    if (depth_ > 0) --depth_;
}

void BoundedJsonReader::enter_array() {
    ++depth_;
    if (depth_ > cfg_.max_nesting_depth) {
        set_error("nesting depth exceeds max_nesting_depth (" +
                  std::to_string(cfg_.max_nesting_depth) + ")");
    }
}

void BoundedJsonReader::leave_array() {
    if (depth_ > 0) --depth_;
}

void BoundedJsonReader::skip_ws() {
    while (pos_ < src_.size() && !has_error_) {
        char c = src_[pos_];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            ++pos_;
        } else {
            break;
        }
    }
}

bool BoundedJsonReader::expect(char c) {
    skip_ws();
    if (has_error_ || pos_ >= src_.size() || src_[pos_] != c) return false;
    ++pos_;
    return true;
}

bool BoundedJsonReader::has_more() const {
    auto p = pos_;
    while (p < src_.size()) {
        char c = src_[p];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') { ++p; continue; }
        return c != '}' && c != ']';
    }
    return false;
}

char BoundedJsonReader::peek() const {
    auto p = pos_;
    while (p < src_.size()) {
        char c = src_[p];
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') { ++p; continue; }
        return c;
    }
    return '\0';
}

std::string BoundedJsonReader::read_raw_string() {
    // Assumes pos_ is at the opening quote
    if (pos_ >= src_.size() || src_[pos_] != '\"') return {};
    ++pos_; // skip opening quote
    std::string out;
    while (pos_ < src_.size() && !has_error_) {
        char c = src_[pos_];
        if (c == '\"') {
            ++pos_; // skip closing quote
            return out;
        }
        if (out.size() >= cfg_.max_string_length) {
            set_error("string exceeds max_string_length (" +
                      std::to_string(cfg_.max_string_length) + ")");
            return {};
        }
        if (c == '\\' && pos_ + 1 < src_.size()) {
            ++pos_;
            char esc = src_[pos_];
            switch (esc) {
            case '\"': out += '\"'; break;
            case '\\': out += '\\'; break;
            case '/':  out += '/';  break;
            case 'b':  out += '\b'; break;
            case 'f':  out += '\f'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            case 'u': {
                if (pos_ + 4 < src_.size()) {
                    unsigned cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        ++pos_;
                        char hc = src_[pos_];
                        cp <<= 4;
                        if (hc >= '0' && hc <= '9') cp |= static_cast<unsigned>(hc - '0');
                        else if (hc >= 'a' && hc <= 'f') cp |= static_cast<unsigned>(hc - 'a' + 10);
                        else if (hc >= 'A' && hc <= 'F') cp |= static_cast<unsigned>(hc - 'A' + 10);
                        else { set_error("invalid \\u escape sequence"); return {}; }
                    }
                    if (cp < 0x80) {
                        out += static_cast<char>(cp);
                    } else if (cp < 0x800) {
                        out += static_cast<char>(0xC0 | (cp >> 6));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    } else {
                        out += static_cast<char>(0xE0 | (cp >> 12));
                        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                        out += static_cast<char>(0x80 | (cp & 0x3F));
                    }
                }
                break;
            }
            default:
                set_error("invalid escape character in JSON string");
                return {};
            }
        } else if (static_cast<unsigned char>(c) < 0x20) {
            set_error("unescaped control character in JSON string");
            return {};
        } else {
            out += c;
        }
        ++pos_;
    }
    set_error("unterminated JSON string");
    return {};
}

std::string BoundedJsonReader::read_string() {
    skip_ws();
    return read_raw_string();
}

std::int64_t BoundedJsonReader::read_int64(std::int64_t fallback) {
    skip_ws();
    std::string num;
    if (pos_ < src_.size() && src_[pos_] == '-') { num += src_[pos_]; ++pos_; }
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
        num += src_[pos_]; ++pos_;
    }
    if (num.empty() || num == "-") return fallback;
    std::int64_t v{};
    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), v);
    return (ec != std::errc{}) ? fallback : v;
}

std::uint64_t BoundedJsonReader::read_uint64(std::uint64_t fallback) {
    skip_ws();
    std::string num;
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
        num += src_[pos_]; ++pos_;
    }
    if (num.empty()) return fallback;
    std::uint64_t v{};
    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), v);
    return (ec != std::errc{}) ? fallback : v;
}

double BoundedJsonReader::read_double(double fallback) {
    skip_ws();
    std::string num;
    if (pos_ < src_.size() && src_[pos_] == '-') { num += src_[pos_]; ++pos_; }
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
        num += src_[pos_]; ++pos_;
    }
    if (pos_ < src_.size() && src_[pos_] == '.') {
        num += '.'; ++pos_;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            num += src_[pos_]; ++pos_;
        }
    }
    if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
        num += src_[pos_]; ++pos_;
        if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
            num += src_[pos_]; ++pos_;
        }
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            num += src_[pos_]; ++pos_;
        }
    }
    if (num.empty() || num == "-") return fallback;
    char* end = nullptr;
    double v = std::strtod(num.c_str(), &end);
    if (end == num.c_str()) return fallback;
    if (!std::isfinite(v)) {
        set_error("NaN or infinity not allowed in bounded JSON reader");
        return fallback;
    }
    return v;
}

bool BoundedJsonReader::read_bool() {
    skip_ws();
    if (pos_ + 4 <= src_.size() && src_.substr(pos_, 4) == "true") {
        pos_ += 4; return true;
    }
    if (pos_ + 5 <= src_.size() && src_.substr(pos_, 5) == "false") {
        pos_ += 5; return false;
    }
    return false;
}

bool BoundedJsonReader::read_null() {
    skip_ws();
    if (pos_ + 4 <= src_.size() && src_.substr(pos_, 4) == "null") {
        pos_ += 4; return true;
    }
    return false;
}

void BoundedJsonReader::skip_value() {
    skip_ws();
    if (has_error_ || pos_ >= src_.size()) return;
    char c = src_[pos_];
    if (c == '\"') {
        read_raw_string();
    } else if (c == '{') {
        enter_object();
        ++pos_;
        std::size_t member_count = 0;
        int obj_depth = 1;
        while (pos_ < src_.size() && obj_depth > 0 && !has_error_) {
            if (src_[pos_] == '{') {
                ++obj_depth;
                if (obj_depth > static_cast<int>(cfg_.max_nesting_depth)) {
                    set_error("nesting depth (" + std::to_string(obj_depth) +
                              ") exceeds max_nesting_depth (" + std::to_string(cfg_.max_nesting_depth) + ")");
                }
            }
            else if (src_[pos_] == '}') --obj_depth;
            else if (src_[pos_] == '\"') {
                read_raw_string();
                // Only count members at the top level of this object
                skip_ws();
                if (obj_depth == 1 && pos_ < src_.size() && src_[pos_] == ':') {
                    ++member_count;
                    ++pos_;
                }
                continue; // skip the ++pos_ at the bottom
            }
            else if (src_[pos_] == ',' && member_count > 0) {
                // separator between members
            }
            ++pos_;
        }
        leave_object();
        if (pos_ < src_.size()) ++pos_; // skip past '}'
        if (member_count > cfg_.max_object_members) {
            set_error("object member count (" + std::to_string(member_count) +
                      ") exceeds max_object_members (" + std::to_string(cfg_.max_object_members) + ")");
        }
    } else if (c == '[') {
        enter_array();
        ++pos_;
        std::size_t elem_count = 0;
        int arr_depth = 1;
        bool in_element = false;
        while (pos_ < src_.size() && arr_depth > 0 && !has_error_) {
            char ac = src_[pos_];
            if (ac == '[') {
                ++arr_depth;
                if (arr_depth > static_cast<int>(cfg_.max_nesting_depth)) {
                    set_error("nesting depth (" + std::to_string(arr_depth) +
                              ") exceeds max_nesting_depth (" + std::to_string(cfg_.max_nesting_depth) + ")");
                }
                in_element = true;
            }
            else if (ac == ']') {
                --arr_depth;
                if (arr_depth == 0 && in_element) {
                    ++elem_count;
                    in_element = false;  // prevent double-count in post-loop
                }
            }
            else if (ac == '\"') { read_raw_string(); if (arr_depth == 1) in_element = true; continue; }
            else if (ac == ',' && arr_depth == 1) { if (in_element) ++elem_count; in_element = false; }
            else if (ac != ' ' && ac != '\n' && ac != '\r' && ac != '\t' && arr_depth == 1) { in_element = true; }
            ++pos_;
        }
        // Count the last element if we didn't see a trailing comma
        if (in_element && arr_depth == 0) ++elem_count;
        leave_array();
        if (pos_ < src_.size()) ++pos_; // skip past ']'
        if (elem_count > cfg_.max_array_length) {
            set_error("array element count (" + std::to_string(elem_count) +
                      ") exceeds max_array_length (" + std::to_string(cfg_.max_array_length) + ")");
        }
    } else if (c == 't' || c == 'f') {
        read_bool();
    } else if (c == 'n') {
        read_null();
    } else {
        // number — consume until delimiter
        while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
               src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') ++pos_;
    }
}

std::string BoundedJsonReader::read_typed_value() {
    skip_ws();
    if (has_error_ || pos_ >= src_.size()) return {};
    char c = src_[pos_];
    if (c == '\"') {
        auto start = pos_;
        read_raw_string();
        return std::string(src_.substr(start, pos_ - start));
    }
    if (c == '{' || c == '[') {
        auto start = pos_;
        skip_value();
        return std::string(src_.substr(start, pos_ - start));
    }
    // scalar: number, true, false, null
    std::string scalar;
    while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
           src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') {
        scalar += src_[pos_];
        ++pos_;
    }
    return scalar;
}

// ===================================================================
// GeneValue type-tagged JSON serialization
// ===================================================================

GeneValueTag gene_value_variant_index(GeneValue const& value) {
    return static_cast<GeneValueTag>(value.index());
}

std::string gene_value_to_json(GeneValue const& value) {
    return std::visit([](auto const& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + json_escape(v) + "\"";
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, std::uint64_t>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, double>) {
            if (!std::isfinite(v)) return "null";
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.17g", v);
            return std::string(buf);
        } else {
            static_assert(std::is_same_v<T, std::vector<std::string>>,
                          "gene_value_to_json: missing if-constexpr branch");
            std::ostringstream ss;
            ss << '[';
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i > 0) ss << ',';
                ss << '"' << json_escape(v[i]) << '"';
            }
            ss << ']';
            return ss.str();
        }
    }, value);
}

GeneValue json_to_gene_value(std::string_view json, GeneValueTag tag) {
    switch (tag) {
    case GeneValueTag::string_val:
        // json should be a JSON string like "\"hello\"" — strip quotes
        if (json.size() >= 2 && json.front() == '\"' && json.back() == '\"') {
            // Simple unescape: pass through BoundedJsonReader
            BoundedJsonReader r(json);
            auto s = r.read_string();
            if (!s.empty() || json == "\"\"") return GeneValue{std::in_place_index<0>, s};
        }
        return GeneValue{std::in_place_index<0>, std::string(json)};
    case GeneValueTag::bool_val: {
        bool b = (json == "true");
        return GeneValue{std::in_place_index<1>, b};
    }
    case GeneValueTag::int64_val: {
        std::int64_t v{};
        auto [ptr, ec] = std::from_chars(json.data(), json.data() + json.size(), v);
        if (ec != std::errc{}) return GeneValue{std::in_place_index<0>, std::string(json)};
        return GeneValue{std::in_place_index<2>, v};
    }
    case GeneValueTag::uint64_val: {
        std::uint64_t v{};
        auto [ptr, ec] = std::from_chars(json.data(), json.data() + json.size(), v);
        if (ec != std::errc{}) return GeneValue{std::in_place_index<0>, std::string(json)};
        return GeneValue{std::in_place_index<3>, v};
    }
    case GeneValueTag::double_val: {
        std::string s(json);
        char* end = nullptr;
        double v = std::strtod(s.c_str(), &end);
        if (end == s.c_str() || !std::isfinite(v))
            return GeneValue{std::in_place_index<0>, std::string(json)};
        return GeneValue{std::in_place_index<4>, v};
    }
    case GeneValueTag::string_list_val: {
        std::vector<std::string> list;
        BoundedJsonReader r(json);
        if (r.expect('[')) {
            while (r.has_more()) {
                auto s = r.read_string();
                if (s.empty() && !r.has_more()) break;
                list.push_back(s);
                if (!r.expect(',')) break;
            }
            r.expect(']');
        }
        return GeneValue{std::in_place_index<5>, std::move(list)};
    }
    default:
        return GeneValue{std::in_place_index<0>, std::string(json)};
    }
}

} // namespace gspl
