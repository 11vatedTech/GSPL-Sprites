#include "gspl/json.hpp"
#include "gspl/genes.hpp"
#include <cctype>
#include <cstdio>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <sstream>

static_assert(std::variant_size_v<GeneValue> == 6,
              "GeneValue variant size must match GeneValueTag enum");

namespace gspl {

std::string json_escape(std::string_view sv) {
    std::ostringstream out;
    for (auto c : sv) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\': out << "\\\\"; break;
        case '\n': out << "\n"; break;
        case '\r': out << "\r"; break;
        case '\t': out << "\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                out << "\u00" << "0123456789abcdef"[(c>>4)&0xf]
                    << "0123456789abcdef"[c&0xf];
            } else out << c;
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
    ss << "  \"" << json_escape(key) << "\": " << value;
}

void json_append_key_bool(std::ostringstream& ss, std::string_view key,
                          bool value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << (value ? "true" : "false");
}

BoundedJsonReader::BoundedJsonReader(std::string_view src, BoundedJsonConfig cfg)
    : src_(src), cfg_(cfg) {
    if (src_.size() > cfg_.max_input_bytes)
        set_error("input exceeds max_input_bytes");
}

void BoundedJsonReader::set_error(std::string msg) {
    has_error_ = true;
    error_ = std::move(msg);
}

void BoundedJsonReader::skip_ws() {
    while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\n' ||
           src_[pos_] == '\r' || src_[pos_] == '\t')) ++pos_;
}

bool BoundedJsonReader::expect(char c) {
    skip_ws();
    if (pos_ >= src_.size() || src_[pos_] != c) return false;
    ++pos_;
    return true;
}

bool BoundedJsonReader::has_more() const {
    auto p = pos_;
    while (p < src_.size() && (src_[p] == ' ' || src_[p] == '\n' ||
           src_[p] == '\r' || src_[p] == '\t')) ++p;
    return p < src_.size() && src_[p] != '}' && src_[p] != ']';
}

char BoundedJsonReader::peek() const {
    auto p = pos_;
    while (p < src_.size() && (src_[p] == ' ' || src_[p] == '\n' ||
           src_[p] == '\r' || src_[p] == '\t')) ++p;
    return p < src_.size() ? src_[p] : '\0';
}

void BoundedJsonReader::enter_object() {
    if (++depth_ > cfg_.max_nesting_depth)
        set_error("max_nesting_depth exceeded");
}

void BoundedJsonReader::leave_object() {
    if (depth_ > 0) --depth_;
}

void BoundedJsonReader::enter_array() {
    if (++depth_ > cfg_.max_nesting_depth)
        set_error("max_nesting_depth exceeded");
}

void BoundedJsonReader::leave_array() {
    if (depth_ > 0) --depth_;
}

std::string BoundedJsonReader::read_raw_string() {
    if (pos_ >= src_.size() || src_[pos_] != '"') return {};
    ++pos_;
    std::string out;
    while (pos_ < src_.size() && src_[pos_] != '"') {
        if (out.size() >= cfg_.max_string_length) {
            set_error("max_string_length exceeded");
            return out;
        }
        if (src_[pos_] == '\' && pos_ + 1 < src_.size()) {
            ++pos_;
            switch (src_[pos_]) {
            case '"': out += '"'; break;
            case '\': out += '\'; break;
            case '/':  out += '/'; break;
            case 'n':  out += '\n'; break;
            case 'r':  out += '\r'; break;
            case 't':  out += '\t'; break;
            case 'u':
                if (pos_ + 4 < src_.size()) {
                    auto hex = src_.substr(pos_ + 1, 4);
                    unsigned cp = 0;
                    for (auto hc : hex) {
                        cp <<= 4;
                        if (hc >= '0' && hc <= '9') cp |= (hc - '0');
                        else if (hc >= 'a' && hc <= 'f') cp |= (hc - 'a' + 10);
                        else if (hc >= 'A' && hc <= 'F') cp |= (hc - 'A' + 10);
                    }
                    out += (cp < 0x80) ? static_cast<char>(cp) : '?';
                    pos_ += 4;
                    continue;
                }
                break;
            default: out += src_[pos_]; break;
            }
        } else {
            out += src_[pos_];
        }
        ++pos_;
    }
    if (pos_ < src_.size()) ++pos_;
    return out;
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
    return (end == num.c_str()) ? fallback : v;
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
    if (c == '"') {
        read_raw_string();
    } else if (c == '{') {
        enter_object();
        ++pos_;
        int obj_depth = 1;
        while (pos_ < src_.size() && obj_depth > 0 && !has_error_) {
            if (src_[pos_] == '{') ++obj_depth;
            else if (src_[pos_] == '}') --obj_depth;
            else if (src_[pos_] == '"') read_raw_string();
            else ++pos_;
        }
        leave_object();
        if (pos_ < src_.size()) ++pos_;
    } else if (c == '[') {
        enter_array();
        ++pos_;
        int arr_depth = 1;
        while (pos_ < src_.size() && arr_depth > 0 && !has_error_) {
            if (src_[pos_] == '[') ++arr_depth;
            else if (src_[pos_] == ']') --arr_depth;
            else if (src_[pos_] == '"') read_raw_string();
            else ++pos_;
        }
        leave_array();
        if (pos_ < src_.size()) ++pos_;
    } else if (c == 't' || c == 'f') {
        read_bool();
    } else if (c == 'n') {
        read_null();
    } else {
        while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
               src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') ++pos_;
    }
}

std::string BoundedJsonReader::read_typed_value() {
    skip_ws();
    if (has_error_ || pos_ >= src_.size()) return {};
    char c = src_[pos_];
    if (c == '"') return read_raw_string();
    if (c == '{' || c == '[') {
        auto start = pos_;
        skip_value();
        return std::string(src_.substr(start, pos_ - start));
    }
    std::string scalar;
    while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
           src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') {
        scalar += src_[pos_];
        ++pos_;
    }
    return scalar;
}

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
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            std::ostringstream ss;
            ss << '[';
            for (std::size_t i = 0; i < v.size(); ++i) {
                if (i > 0) ss << ',';
                ss << '"' << json_escape(v[i]) << '"';
            }
            ss << ']';
            return ss.str();
        }
        return "null";
    }, value);
}

GeneValue json_to_gene_value(std::string_view json, GeneValueTag tag) {
    switch (tag) {
    case GeneValueTag::string_val:
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
