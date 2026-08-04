#pragma once
#include "gspl/genes.hpp"
#include "gspl/diagnostics.hpp"
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace gspl {

// ── Canonical bounded JSON writer ─────────────────────────

std::string json_escape(std::string_view sv);
void json_append_key_string(std::ostringstream& ss, std::string_view key,
                            std::string_view value, bool& first);
void json_append_key_int(std::ostringstream& ss, std::string_view key,
                         std::int64_t value, bool& first);
void json_append_key_uint(std::ostringstream& ss, std::string_view key,
                          std::uint64_t value, bool& first);
void json_append_key_double(std::ostringstream& ss, std::string_view key,
                            double value, bool& first);
void json_append_key_bool(std::ostringstream& ss, std::string_view key,
                          bool value, bool& first);

// ── JSON source position ─────────────────────────────────

struct JsonSourcePosition {
    std::size_t byte_offset{};
    std::size_t line{1};
    std::size_t column{1};
};

// ── Fail-closed read result ──────────────────────────────

template <typename T>
struct JsonReadResult {
    std::optional<T> value;
    DiagnosticResult diagnostics;

    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && diagnostics.ok();
    }
};

// ── Bounded JSON error codes ───────────────────────────────

enum class BoundedJsonErrorCode {
    none,
    malformed,
    token_limit,
    nesting_limit,
    object_member_limit,
    array_length_limit,
    string_length_limit,
    input_limit,
    unexpected_eof,
    trailing_content,
    type_mismatch
};

// ── Canonical bounded JSON reader ─────────────────────────

struct BoundedJsonConfig {
    std::size_t max_input_bytes{64 * 1024 * 1024};
    std::size_t max_nesting_depth{64};
    std::size_t max_object_members{10'000};
    std::size_t max_array_length{100'000};
    std::size_t max_string_length{64 * 1024};
    std::size_t max_tokens{0};  // 0 = unlimited
};

// Per-container tracking frame — nested-safe
struct ContainerFrame {
    enum Kind { object, array };
    Kind kind;
    std::string path;
    std::size_t item_count{0};
    bool expect_separator{false};  // false=expect value/member, true=expect separator or end
};

class BoundedJsonReader {
public:
    explicit BoundedJsonReader(std::string_view src, BoundedJsonConfig cfg = {});

    // Position / navigation
    void skip_ws();

    // Fail-closed mandatory token consumption — sets error state on mismatch
    bool require(char expected, std::string_view path);

    // Optional token probe — no error on mismatch, advances if match
    bool consume_if(char token);

    // Prefer require() or consume_if() instead
    bool expect(char c);

    bool has_more() const;
    std::size_t depth() const { return depth_; }
    JsonSourcePosition source_position() const;
    std::size_t byte_offset() const { return pos_; }
    std::size_t position() const { return pos_; }
    char peek() const;
    std::string_view source() const { return src_; }

    // Legacy fallback-returning reads (deprecated — use Result variants)
    std::string read_string();
    std::int64_t read_int64(std::int64_t fallback = 0);
    std::uint64_t read_uint64(std::uint64_t fallback = 0);
    double read_double(double fallback = 0.0);
    bool read_bool();
    bool read_null();

    // Fail-closed reads
    JsonReadResult<std::string> read_string_result();
    JsonReadResult<std::int64_t> read_int64_result();
    JsonReadResult<std::uint64_t> read_uint64_result();
    JsonReadResult<double> read_double_result();
    JsonReadResult<bool> read_bool_result();
    JsonReadResult<bool> read_null_result();

    // Value skipping and raw fragment capture
    void skip_value();
    std::string read_typed_value();

    // Error state
    bool has_error() const { return has_error_; }
    std::string const& error_message() const { return error_; }
    BoundedJsonErrorCode error_code() const { return error_code_; }

    // Token budget accounting
    std::size_t token_count() const { return token_count_; }

    // Container entry (increments unified nesting depth)
    void enter_object();
    void leave_object();
    void enter_array();
    void leave_array();

    // Governed container parsing — enforces bounds, separators, trailing commas
    bool begin_object(std::string_view path);   // require '{' + enter + reset member count
    bool end_object(std::string_view path);     // require '}' + leave
    bool begin_array(std::string_view path);    // require '[' + enter + reset element count
    bool end_array(std::string_view path);      // require ']' + leave
    bool record_object_member(std::string_view path);  // count + enforce limit
    bool record_array_element(std::string_view path);  // count + enforce limit
    // Returns true if comma consumed (more items), false if at closing delimiter.
    // Sets error on trailing comma or missing comma.
    bool next_object_member(std::string_view path);
    bool next_array_element(std::string_view path);

    // Typed uint32 read with checked narrowing
    JsonReadResult<std::uint32_t> read_uint32_result();
    JsonReadResult<std::int32_t> read_int32_result();

private:
    std::string_view src_;
    std::size_t pos_{};
    std::size_t depth_{};
    std::vector<ContainerFrame> container_stack_;
    std::size_t line_{1};
    std::size_t col_{1};
    BoundedJsonConfig cfg_;
    bool has_error_{false};
    std::string error_;
    BoundedJsonErrorCode error_code_{BoundedJsonErrorCode::none};
    std::size_t token_count_{};

    ContainerFrame* current_frame() noexcept;
    void set_error(std::string msg, BoundedJsonErrorCode code = BoundedJsonErrorCode::malformed);
    void count_token();   // increments token_count_, checks cfg_.max_tokens
    std::string read_raw_string();
    void advance_pos();  // increments pos_, updates line_/col_
};

// ── GeneValue type tags ───────────────────────────────────

enum class GeneValueTag : std::uint32_t {
    string_val = 0, bool_val = 1, int64_val = 2,
    uint64_val = 3, double_val = 4, string_list_val = 5
};

std::string gene_value_to_json(GeneValue const& value);
GeneValue json_to_gene_value(std::string_view json, GeneValueTag tag);
GeneValueTag gene_value_variant_index(GeneValue const& value);

// Fail-closed GeneValue deserialization
struct GeneValueDeserializeResult {
    std::optional<GeneValue> value;
    DiagnosticResult diagnostics;
    [[nodiscard]] bool ok() const noexcept {
        return value.has_value() && diagnostics.ok();
    }
};
GeneValueDeserializeResult json_to_gene_value_result(std::string_view json, GeneValueTag tag);

} // namespace gspl
