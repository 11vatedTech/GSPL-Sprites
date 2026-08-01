#include "gspl/json.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

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
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char hex[8];
                std::snprintf(hex, sizeof(hex), "\\u%04x",
                              static_cast<unsigned>(static_cast<unsigned char>(c)));
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
        throw std::invalid_argument(
            "json_append_key_double: non-finite double value for key '" +
            std::string(key) + "' — NaN and Infinity are not valid JSON");
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", value);
    ss << "  \"" << json_escape(key) << "\": " << buf;
}

void json_append_key_bool(std::ostringstream& ss, std::string_view key,
                          bool value, bool& first) {
    if (!first) ss << ",\n";
    first = false;
    ss << "  \"" << json_escape(key) << "\": " << (value ? "true" : "false");
}

// ===================================================================
// BoundedJsonReader — constructor
// ===================================================================

BoundedJsonReader::BoundedJsonReader(std::string_view src, BoundedJsonConfig cfg)
    : src_(src), cfg_(cfg) {
    if (src_.size() > cfg_.max_input_bytes) {
        set_error("input exceeds max_input_bytes (" +
                  std::to_string(cfg_.max_input_bytes) + ")");
    }
}

JsonSourcePosition BoundedJsonReader::source_position() const {
    return {pos_, line_, col_};
}

void BoundedJsonReader::set_error(std::string msg) {
    if (!has_error_) {
        has_error_ = true;
        error_ = std::move(msg);
    }
}

void BoundedJsonReader::count_token() {
    ++token_count_;
    if (cfg_.max_tokens > 0 && token_count_ > cfg_.max_tokens) {
        set_error("JSON token budget exhausted (" +
                  std::to_string(cfg_.max_tokens) + " max, " +
                  std::to_string(token_count_) + " consumed)");
    }
}

void BoundedJsonReader::advance_pos() {
    if (pos_ >= src_.size()) return;
    if (src_[pos_] == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    ++pos_;
}

// ── Enter / leave containers (shared depth counter) ──────

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

// ── Per-container stack helper ─────────────────────────────────

ContainerFrame* BoundedJsonReader::current_frame() noexcept {
    return container_stack_.empty() ? nullptr : &container_stack_.back();
}

// ── Governed container APIs ────────────────────────────────

bool BoundedJsonReader::begin_object(std::string_view path) {
    if (!require('{', path)) return false;
    enter_object();
    ContainerFrame frame;
    frame.kind = ContainerFrame::object;
    frame.path = path;
    container_stack_.push_back(std::move(frame));
    return true;
}

bool BoundedJsonReader::end_object(std::string_view path) {
    // Verify frame kind before popping
    if (container_stack_.empty()) {
        set_error(std::string(path) + ": end_object called with empty container stack");
        return false;
    }
    auto& frame = container_stack_.back();
    if (frame.kind != ContainerFrame::object) {
        set_error(std::string(path) + ": end_object called but current frame is not an object (" + frame.path + ")");
        return false;
    }
    container_stack_.pop_back();
    leave_object();
    return require('}', path);
}

bool BoundedJsonReader::begin_array(std::string_view path) {
    if (!require('[', path)) return false;
    enter_array();
    ContainerFrame frame;
    frame.kind = ContainerFrame::array;
    frame.path = path;
    container_stack_.push_back(std::move(frame));
    return true;
}

bool BoundedJsonReader::end_array(std::string_view path) {
    // Verify frame kind before popping
    if (container_stack_.empty()) {
        set_error(std::string(path) + ": end_array called with empty container stack");
        return false;
    }
    auto& frame = container_stack_.back();
    if (frame.kind != ContainerFrame::array) {
        set_error(std::string(path) + ": end_array called but current frame is not an array (" + frame.path + ")");
        return false;
    }
    container_stack_.pop_back();
    leave_array();
    return require(']', path);
}

bool BoundedJsonReader::record_object_member(std::string_view path) {
    auto* frame = current_frame();
    if (!frame) {
        set_error(std::string(path) + ": record_object_member called outside governed container");
        return false;
    }
    ++frame->item_count;
    if (frame->item_count > cfg_.max_object_members) {
        set_error(std::string(path) + ": object member count (" +
                  std::to_string(frame->item_count) + ") exceeds max_object_members (" +
                  std::to_string(cfg_.max_object_members) + ")");
        return false;
    }
    return true;
}

bool BoundedJsonReader::record_array_element(std::string_view path) {
    auto* frame = current_frame();
    if (!frame) {
        set_error(std::string(path) + ": record_array_element called outside governed container");
        return false;
    }
    ++frame->item_count;
    if (frame->item_count > cfg_.max_array_length) {
        set_error(std::string(path) + ": array element count (" +
                  std::to_string(frame->item_count) + ") exceeds max_array_length (" +
                  std::to_string(cfg_.max_array_length) + ")");
        return false;
    }
    return true;
}

bool BoundedJsonReader::next_object_member(std::string_view path) {
    auto* frame = current_frame();
    std::string frame_path = frame ? frame->path : std::string(path);
    skip_ws();
    if (has_error_) return false;
    if (pos_ >= src_.size()) {
        set_error(frame_path + ": unterminated object");
        return false;
    }
    if (src_[pos_] == '}') {
        if (frame) frame->expect_separator = false;
        return false;
    }
    if (src_[pos_] != ',') {
        set_error(frame_path + ": expected ',' or '}' after object member at line " +
                  std::to_string(line_) + " column " + std::to_string(col_));
        return false;
    }
    advance_pos(); // consume comma
    count_token();  // comma separator token
    if (frame) frame->expect_separator = false;
    skip_ws();
    if (has_error_) return false;
    if (pos_ < src_.size() && src_[pos_] == '}') {
        set_error(frame_path + ": trailing comma in object at line " +
                  std::to_string(line_) + " column " + std::to_string(col_));
        return false;
    }
    return true;
}

bool BoundedJsonReader::next_array_element(std::string_view path) {
    auto* frame = current_frame();
    std::string frame_path = frame ? frame->path : std::string(path);
    skip_ws();
    if (has_error_) return false;
    if (pos_ >= src_.size()) {
        set_error(frame_path + ": unterminated array");
        return false;
    }
    if (src_[pos_] == ']') {
        if (frame) frame->expect_separator = false;
        return false;
    }
    if (src_[pos_] != ',') {
        set_error(frame_path + ": expected ',' or ']' after array element at line " +
                  std::to_string(line_) + " column " + std::to_string(col_));
        return false;
    }
    advance_pos(); // consume comma
    count_token();  // comma separator token
    if (frame) frame->expect_separator = false;
    skip_ws();
    if (has_error_) return false;
    if (pos_ < src_.size() && src_[pos_] == ']') {
        set_error(frame_path + ": trailing comma in array at line " +
                  std::to_string(line_) + " column " + std::to_string(col_));
        return false;
    }
    return true;
}

// ── Typed uint32 read ─────────────────────────────────────

JsonReadResult<std::uint32_t> BoundedJsonReader::read_uint32_result() {
    JsonReadResult<std::uint32_t> result;
    auto int_res = read_int64_result();
    if (!int_res.ok()) {
        result.diagnostics = std::move(int_res.diagnostics);
        return result;
    }
    auto val = *int_res.value;
    if (val < 0) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "expected uint32, got negative value", {});
        return result;
    }
    if (static_cast<std::uint64_t>(val) > static_cast<std::uint64_t>(UINT32_MAX)) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "uint32 overflow: " + std::to_string(val), {});
        return result;
    }
    result.value = static_cast<std::uint32_t>(val);
    return result;
}

JsonReadResult<std::int32_t> BoundedJsonReader::read_int32_result() {
    JsonReadResult<std::int32_t> result;
    auto int_res = read_int64_result();
    if (!int_res.ok()) {
        result.diagnostics = std::move(int_res.diagnostics);
        return result;
    }
    auto val = *int_res.value;
    if (val < static_cast<std::int64_t>(INT32_MIN)) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "int32 underflow", {});
        return result;
    }
    if (val > static_cast<std::int64_t>(INT32_MAX)) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "int32 overflow", {});
        return result;
    }
    result.value = static_cast<std::int32_t>(val);
    return result;
}

// ── Whitespace ────────────────────────────────────────────

void BoundedJsonReader::skip_ws() {
    while (pos_ < src_.size() && !has_error_) {
        char c = src_[pos_];
        if (c == ' ' || c == '\r' || c == '\t') {
            ++col_;
            ++pos_;
        } else if (c == '\n') {
            ++line_;
            col_ = 1;
            ++pos_;
        } else {
            break;
        }
    }
}

// ── Navigation ────────────────────────────────────────────

bool BoundedJsonReader::require(char expected, std::string_view path) {
    skip_ws();
    if (has_error_) return false;
    if (pos_ >= src_.size()) {
        set_error(std::string(path) + ": expected '" + expected + "', got end-of-input at line " +
                  std::to_string(line_) + " column " + std::to_string(col_));
        return false;
    }
    if (src_[pos_] != expected) {
        char buf[32];
        if (static_cast<unsigned char>(src_[pos_]) >= 0x20 &&
            static_cast<unsigned char>(src_[pos_]) <= 0x7E) {
            std::snprintf(buf, sizeof(buf), "'%c'", src_[pos_]);
        } else {
            std::snprintf(buf, sizeof(buf), "0x%02x",
                          static_cast<unsigned>(static_cast<unsigned char>(src_[pos_])));
        }
        set_error(std::string(path) + ": expected '" + expected + "', got " + buf +
                  " at line " + std::to_string(line_) + " column " + std::to_string(col_));
        return false;
    }
    advance_pos();
    count_token();  // structural token: { } [ ] :
    return true;
}

bool BoundedJsonReader::consume_if(char token) {
    skip_ws();
    if (has_error_ || pos_ >= src_.size() || src_[pos_] != token) return false;
    advance_pos();
    count_token();
    return true;
}

bool BoundedJsonReader::expect(char c) {
    skip_ws();
    if (has_error_ || pos_ >= src_.size() || src_[pos_] != c) return false;
    advance_pos();
    count_token();
    return true;
}

bool BoundedJsonReader::has_more() const {
    if (has_error_) return false;
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

// ── String reading with Unicode support ───────────────────

static unsigned decode_hex4(std::string_view s, std::size_t pos) {
    unsigned cp = 0;
    for (int i = 0; i < 4; ++i) {
        if (pos + i >= s.size()) return 0;
        char hc = s[pos + i];
        cp <<= 4;
        if (hc >= '0' && hc <= '9') cp |= static_cast<unsigned>(hc - '0');
        else if (hc >= 'a' && hc <= 'f') cp |= static_cast<unsigned>(hc - 'a' + 10);
        else if (hc >= 'A' && hc <= 'F') cp |= static_cast<unsigned>(hc - 'A' + 10);
        else return 0xFFFFFFFF;
    }
    return cp;
}

static void encode_utf8(unsigned cp, std::string& out) {
    if (cp <= 0x7F) {
        out += static_cast<char>(cp);
    } else if (cp <= 0x7FF) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp <= 0x10FFFF) {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

std::string BoundedJsonReader::read_raw_string() {
    if (pos_ >= src_.size() || src_[pos_] != '"') return {};
    advance_pos(); // skip opening quote
    count_token();  // string token
    std::string out;
    while (pos_ < src_.size() && !has_error_) {
        char c = src_[pos_];
        if (c == '"') {
            advance_pos(); // skip closing quote
            return out;
        }
        if (out.size() >= cfg_.max_string_length) {
            set_error("string exceeds max_string_length (" +
                      std::to_string(cfg_.max_string_length) + ")");
            return {};
        }
        if (c == '\\' && pos_ + 1 < src_.size()) {
            advance_pos(); // skip backslash
            char esc = src_[pos_];
            switch (esc) {
            case '"': out += '"'; advance_pos(); break;
            case '\\': out += '\\'; advance_pos(); break;
            case '/': out += '/'; advance_pos(); break;
            case 'b': out += '\b'; advance_pos(); break;
            case 'f': out += '\f'; advance_pos(); break;
            case 'n': out += '\n'; advance_pos(); break;
            case 'r': out += '\r'; advance_pos(); break;
            case 't': out += '\t'; advance_pos(); break;
            case 'u': {
                if (pos_ + 4 >= src_.size()) {
                    set_error("truncated \\u escape sequence");
                    return {};
                }
                unsigned cp = decode_hex4(src_, pos_ + 1);
                if (cp == 0xFFFFFFFF) {
                    set_error("invalid \\u escape sequence");
                    return {};
                }
                // Advance past the 4 hex digits (consume 'u' + 3 hex = 4 chars, then 1 more for 4th hex digit)
                for (int i = 0; i < 4; ++i) advance_pos();
                advance_pos(); // past last hex digit

                // Surrogate pair support
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    // High surrogate — expect \uXXXX low surrogate
                    if (pos_ + 6 < src_.size() && src_[pos_] == '\\' &&
                        src_[pos_ + 1] == 'u') {
                        advance_pos(); // past '\\'
                        advance_pos(); // past 'u'
                        unsigned lo = decode_hex4(src_, pos_);
                        if (lo >= 0xDC00 && lo <= 0xDFFF) {
                            for (int i = 0; i < 4; ++i) advance_pos();
                            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                            encode_utf8(cp, out);
                        } else {
                            set_error("isolated high surrogate without valid low surrogate");
                            return {};
                        }
                    } else {
                        set_error("isolated high surrogate pair element");
                        return {};
                    }
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    set_error("isolated low surrogate pair element");
                    return {};
                } else {
                    encode_utf8(cp, out);
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
            // Verify valid UTF-8 lead byte or continuation
            unsigned char uc = static_cast<unsigned char>(c);
            if (uc >= 0x80) {
                // Validate UTF-8 sequence
                int seq_len = 0;
                if ((uc & 0xE0) == 0xC0) seq_len = 2;
                else if ((uc & 0xF0) == 0xE0) seq_len = 3;
                else if ((uc & 0xF8) == 0xF0) seq_len = 4;
                else { set_error("invalid UTF-8 lead byte"); return {}; }

                if (pos_ + seq_len > src_.size()) {
                    set_error("truncated UTF-8 sequence"); return {};
                }
                for (int i = 1; i < seq_len; ++i) {
                    if ((static_cast<unsigned char>(src_[pos_ + i]) & 0xC0) != 0x80) {
                        set_error("invalid UTF-8 continuation byte"); return {};
                    }
                }
                // Copy entire sequence
                for (int i = 0; i < seq_len; ++i) {
                    out += src_[pos_];
                    advance_pos();
                }
                continue; // skip the advance_pos() at end of loop
            }
            out += c;
            advance_pos();
            continue;
        }
    }
    set_error("unterminated JSON string");
    return {};
}

// ── Legacy fallback-returning reads ────────────────────────

std::string BoundedJsonReader::read_string() {
    skip_ws();
    return read_raw_string();
}

std::int64_t BoundedJsonReader::read_int64(std::int64_t fallback) {
    auto res = read_int64_result();
    return res.ok() ? *res.value : fallback;
}

std::uint64_t BoundedJsonReader::read_uint64(std::uint64_t fallback) {
    auto res = read_uint64_result();
    return res.ok() ? *res.value : fallback;
}

double BoundedJsonReader::read_double(double fallback) {
    auto res = read_double_result();
    return res.ok() ? *res.value : fallback;
}

bool BoundedJsonReader::read_bool() {
    auto res = read_bool_result();
    return res.ok() ? *res.value : false;
}

bool BoundedJsonReader::read_null() {
    auto res = read_null_result();
    return res.ok();
}

// ── Fail-closed reads ─────────────────────────────────────

JsonReadResult<std::string> BoundedJsonReader::read_string_result() {
    JsonReadResult<std::string> result;
    skip_ws();
    if (has_error_) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_UNTERMINATED_STRING,
                                     error_, {});
        return result;
    }
    auto s = read_raw_string();
    if (has_error_) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_INVALID_UTF8,
                                     error_, {});
        return result;
    }
    result.value = std::move(s);
    return result;
}

JsonReadResult<std::int64_t> BoundedJsonReader::read_int64_result() {
    JsonReadResult<std::int64_t> result;
    skip_ws();
    if (has_error_) return result;

    if (pos_ >= src_.size()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "expected integer, got end of input", {});
        return result;
    }

    // Check for negative sign followed by unsigned token
    char first = src_[pos_];
    std::string num;
    if (first == '-') { num += first; advance_pos(); }
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
        num += src_[pos_]; advance_pos();
    }
    if (num.empty() || num == "-") {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "invalid integer token", {});
        return result;
    }
    // Reject leading zero: "0" is valid, "01", "-01" etc are not
    if (num.size() > 1 && num[0] == '0') {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "leading zeros not allowed in JSON: " + num, {});
        return result;
    }
    if (num.size() > 2 && num[0] == '-' && num[1] == '0') {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "leading zeros not allowed in JSON: " + num, {});
        return result;
    }
    std::int64_t v{};
    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), v);
    if (ec != std::errc{}) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "integer overflow", {});
        return result;
    }
    count_token();  // number token
    result.value = v;
    return result;
}

JsonReadResult<std::uint64_t> BoundedJsonReader::read_uint64_result() {
    JsonReadResult<std::uint64_t> result;
    skip_ws();
    if (has_error_) return result;

    if (pos_ >= src_.size()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "expected unsigned integer, got end of input", {});
        return result;
    }

    // Reject negative values
    if (src_[pos_] == '-') {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "expected unsigned integer, got negative", {});
        return result;
    }

    std::string num;
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
        num += src_[pos_]; advance_pos();
    }
    if (num.empty()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "invalid unsigned integer token", {});
        return result;
    }
    // Reject leading zero
    if (num.size() > 1 && num[0] == '0') {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "leading zeros not allowed in JSON: " + num, {});
        return result;
    }
    std::uint64_t v{};
    auto [ptr, ec] = std::from_chars(num.data(), num.data() + num.size(), v);
    if (ec != std::errc{}) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "unsigned integer overflow", {});
        return result;
    }
    count_token();  // number token
    result.value = v;
    return result;
}

JsonReadResult<double> BoundedJsonReader::read_double_result() {
    JsonReadResult<double> result;
    skip_ws();
    if (has_error_) return result;

    if (pos_ >= src_.size()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "expected number, got end of input", {});
        return result;
    }

    // Check for NaN / Infinity tokens
    if (pos_ + 3 <= src_.size() &&
        (src_.substr(pos_, 3) == "NaN" || (pos_ + 8 <= src_.size() &&
         (src_.substr(pos_, 8) == "Infinity" || src_.substr(pos_, 8) == "-Infinit")))) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "NaN and Infinity are not valid JSON numbers", {});
        return result;
    }

    std::string num;
    if (pos_ < src_.size() && src_[pos_] == '-') { num += src_[pos_]; advance_pos(); }
    // Integer part
    bool saw_digit = false;
    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
        num += src_[pos_]; advance_pos();
        saw_digit = true;
    }
    if (!saw_digit) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "expected digit in number", {});
        return result;
    }
    // Fraction
    if (pos_ < src_.size() && src_[pos_] == '.') {
        num += '.'; advance_pos();
        bool saw_frac = false;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            num += src_[pos_]; advance_pos();
            saw_frac = true;
        }
        if (!saw_frac) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                         "incomplete fraction: expected digit after '.'", {});
            return result;
        }
    }
    // Exponent
    if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
        num += src_[pos_]; advance_pos();
        if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
            num += src_[pos_]; advance_pos();
        }
        bool saw_exp = false;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            num += src_[pos_]; advance_pos();
            saw_exp = true;
        }
        if (!saw_exp) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                         "incomplete exponent: expected digit after 'e'", {});
            return result;
        }
    }
    if (num.empty() || num == "-") {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "invalid number token", {});
        return result;
    }
    char* end = nullptr;
    double v = std::strtod(num.c_str(), &end);
    if (end == num.c_str() || end != num.c_str() + num.size()) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "failed to parse number: " + num, {});
        return result;
    }
    if (!std::isfinite(v)) {
        result.diagnostics.add_error(DiagnosticCode::GSPL_LEX_NUMERIC_OVERFLOW,
                                     "number overflow or NaN", {});
        return result;
    }
    count_token();  // number token
    result.value = v;
    return result;
}

JsonReadResult<bool> BoundedJsonReader::read_bool_result() {
    JsonReadResult<bool> result;
    skip_ws();
    if (has_error_) return result;

    if (pos_ + 4 <= src_.size() && src_.substr(pos_, 4) == "true") {
        for (int i = 0; i < 4; ++i) advance_pos();
        count_token();  // boolean token
        result.value = true;
        return result;
    }
    if (pos_ + 5 <= src_.size() && src_.substr(pos_, 5) == "false") {
        for (int i = 0; i < 5; ++i) advance_pos();
        count_token();  // boolean token
        result.value = false;
        return result;
    }
    result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                 "expected true or false", {});
    return result;
}

JsonReadResult<bool> BoundedJsonReader::read_null_result() {
    JsonReadResult<bool> result;
    skip_ws();
    if (pos_ + 4 <= src_.size() && src_.substr(pos_, 4) == "null") {
        for (int i = 0; i < 4; ++i) advance_pos();
        count_token();  // null token
        result.value = true;
        return result;
    }
    result.diagnostics.add_error(DiagnosticCode::GSPL_TYPE_MISMATCH,
                                 "expected null", {});
    return result;
}

// ── Value skipping (recursive, unified mixed-nesting depth) ─

void BoundedJsonReader::skip_value() {
    skip_ws();
    if (has_error_ || pos_ >= src_.size()) return;
    char c = src_[pos_];

    if (c == '"') {
        read_raw_string();
        return;
    }

    if (c == '{') {
        enter_object();
        advance_pos(); // skip '{'
        count_token();  // {
        std::size_t member_count = 0;
        bool expect_comma = false;
        bool saw_closing = false;

        for (;;) {
            skip_ws();
            if (pos_ >= src_.size() || has_error_) break;
            if (src_[pos_] == '}') {
                advance_pos();
                count_token();  // }
                leave_object();
                saw_closing = true;
                break;
            }
            if (expect_comma) {
                if (src_[pos_] == ',') {
                    advance_pos();
                    count_token();  // ,
                    expect_comma = false;
                    continue;
                } else {
                    set_error("expected ',' or '}' after object member");
                    break;
                }
            }
            if (src_[pos_] != '"') {
                set_error("expected string key in JSON object");
                break;
            }
            read_raw_string(); // key (counted internally)
            if (has_error_) break;
            skip_ws();
            if (pos_ >= src_.size() || src_[pos_] != ':') {
                set_error("expected ':' after object key");
                break;
            }
            advance_pos(); // skip ':'
            count_token();  // :
            ++member_count;
            if (member_count > cfg_.max_object_members) {
                set_error("object member count (" + std::to_string(member_count) +
                          ") exceeds max_object_members (" +
                          std::to_string(cfg_.max_object_members) + ")");
            }
            skip_value(); // recursively skip the value
            if (has_error_) break;
            expect_comma = true;
        }
        if (!saw_closing && pos_ >= src_.size() && !has_error_) {
            set_error("unterminated JSON object");
        }
        if (!saw_closing) {
            leave_object();  // balance enter_object() for all non-closing exits
        }
        return;
    }

    if (c == '[') {
        enter_array();
        advance_pos(); // skip '['
        count_token();  // [
        std::size_t elem_count = 0;
        bool expect_comma = false;
        bool saw_closing = false;

        for (;;) {
            skip_ws();
            if (pos_ >= src_.size() || has_error_) break;
            if (src_[pos_] == ']') {
                advance_pos();
                count_token();  // ]
                leave_array();
                saw_closing = true;
                break;
            }
            if (expect_comma) {
                if (src_[pos_] == ',') {
                    advance_pos();
                    count_token();  // ,
                    expect_comma = false;
                    continue;
                } else {
                    set_error("expected ',' or ']' after array element");
                    break;
                }
            }
            ++elem_count;
            if (elem_count > cfg_.max_array_length) {
                set_error("array element count (" + std::to_string(elem_count) +
                          ") exceeds max_array_length (" +
                          std::to_string(cfg_.max_array_length) + ")");
            }
            skip_value(); // recursively skip the element
            if (has_error_) break;
            expect_comma = true;
        }
        if (!saw_closing && pos_ >= src_.size() && !has_error_) {
            set_error("unterminated JSON array");
        }
        if (!saw_closing) {
            leave_array();  // balance enter_array() for all non-closing exits
        }
        return;
    }

    // Scalar: true, false, null, or number
    if (c == 't' || c == 'f') {
        read_bool();  // counted internally
    } else if (c == 'n') {
        read_null();  // counted internally
    } else {
        // number — consume until delimiter (count as 1 token)
        bool consumed = false;
        while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
               src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') {
            advance_pos();
            consumed = true;
        }
        if (consumed) count_token();
    }
}

// ── Typed value fragment capture ───────────────────────────

std::string BoundedJsonReader::read_typed_value() {
    skip_ws();
    if (has_error_ || pos_ >= src_.size()) return {};
    char c = src_[pos_];
    if (c == '"') {
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
    bool consumed = false;
    while (pos_ < src_.size() && src_[pos_] != ',' && src_[pos_] != '}' &&
           src_[pos_] != ']' && src_[pos_] != '\n' && src_[pos_] != '\r') {
        scalar += src_[pos_];
        advance_pos();
        consumed = true;
    }
    if (consumed) count_token();
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
            if (!std::isfinite(v)) {
                throw std::invalid_argument(
                    "gene_value_to_json: non-finite double value — "
                    "NaN and Infinity are not valid JSON");
            }
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
    auto result = json_to_gene_value_result(json, tag);
    if (result.ok()) return *result.value;
    // Legacy migration: only available through explicit migration adapter
    // Current-schema callers must use json_to_gene_value_result() directly
    throw std::invalid_argument(
        "json_to_gene_value: decoding failed for tag " +
        std::to_string(static_cast<std::uint32_t>(tag)) +
        " — use json_to_gene_value_result() for fail-closed access");
}

GeneValueDeserializeResult json_to_gene_value_result(std::string_view json,
                                                      GeneValueTag tag) {
    GeneValueDeserializeResult result;

    // Trim trailing whitespace — from_chars doesn't skip it
    while (!json.empty() && (json.back() == ' ' || json.back() == '\n' ||
           json.back() == '\r' || json.back() == '\t')) {
        json.remove_suffix(1);
    }

    switch (tag) {
    case GeneValueTag::string_val:
        if (json.size() >= 2 && json.front() == '"' && json.back() == '"') {
            BoundedJsonReader r(json);
            auto s = r.read_string_result();
            if (s.ok()) {
                // Verify complete consumption after closing quote
                r.skip_ws();
                if (r.position() < r.source().size()) {
                    result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                                 "trailing data after string value", {});
                    return result;
                }
                result.value = GeneValue{std::in_place_index<0>, *s.value};
                return result;
            }
            result.diagnostics = s.diagnostics;
            return result;
        }
        // Reject unquoted strings in current schema
        result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                     "string value must be JSON-quoted", {});
        return result;

    case GeneValueTag::bool_val: {
        if (json == "true") {
            result.value = GeneValue{std::in_place_index<1>, true};
            return result;
        }
        if (json == "false") {
            result.value = GeneValue{std::in_place_index<1>, false};
            return result;
        }
        result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                     "expected bool value, got: " + std::string(json), {});
        return result;
    }

    case GeneValueTag::int64_val: {
        std::int64_t v{};
        auto [ptr, ec] = std::from_chars(json.data(), json.data() + json.size(), v);
        if (ec != std::errc{}) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "int64 parse failure: " + std::string(json), {});
            return result;
        }
        // Require complete consumption — reject trailing data
        if (ptr != json.data() + json.size()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "trailing data after int64: " + std::string(json), {});
            return result;
        }
        result.value = GeneValue{std::in_place_index<2>, v};
        return result;
    }

    case GeneValueTag::uint64_val: {
        // Reject negative
        if (!json.empty() && json[0] == '-') {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "expected uint64, got negative", {});
            return result;
        }
        std::uint64_t v{};
        auto [ptr, ec] = std::from_chars(json.data(), json.data() + json.size(), v);
        if (ec != std::errc{}) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "uint64 parse failure: " + std::string(json), {});
            return result;
        }
        // Require complete consumption
        if (ptr != json.data() + json.size()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "trailing data after uint64: " + std::string(json), {});
            return result;
        }
        result.value = GeneValue{std::in_place_index<3>, v};
        return result;
    }

    case GeneValueTag::double_val: {
        std::string s(json);
        char* end = nullptr;
        double v = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "double parse failure: " + s, {});
            return result;
        }
        // Require complete consumption
        if (end != s.c_str() + s.size()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "trailing data after double: " + s, {});
            return result;
        }
        if (!std::isfinite(v)) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "NaN/infinity not allowed for double gene value", {});
            return result;
        }
        result.value = GeneValue{std::in_place_index<4>, v};
        return result;
    }

    case GeneValueTag::string_list_val: {
        std::vector<std::string> list;
        BoundedJsonReader r(json);
        if (!r.begin_array("string_list")) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "expected '[' for string_list", {});
            return result;
        }
        while (r.has_more() && !r.has_error()) {
            auto s = r.read_string_result();
            if (!s.ok()) {
                result.diagnostics = s.diagnostics;
                return result;
            }
            list.push_back(*s.value);
            r.record_array_element("string_list");
            if (!r.next_array_element("string_list")) break;
        }
        if (!r.end_array("string_list")) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "expected ']' to close string_list", {});
            return result;
        }
        // Verify complete consumption
        r.skip_ws();
        if (r.position() < r.source().size()) {
            result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_INVALID_VALUE,
                                         "trailing data after string_list", {});
            return result;
        }
        result.value = GeneValue{std::in_place_index<5>, std::move(list)};
        return result;
    }

    default:
        result.diagnostics.add_error(DiagnosticCode::GSPL_GENE_UNKNOWN,
                                     "unknown GeneValueTag: " +
                                     std::to_string(static_cast<std::uint32_t>(tag)), {});
        return result;
    }
}

} // namespace gspl
