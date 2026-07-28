#pragma once
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

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

// ── Canonical bounded JSON reader ─────────────────────────

struct BoundedJsonConfig {
    std::size_t max_input_bytes{64 * 1024 * 1024};
    std::size_t max_nesting_depth{64};
    std::size_t max_object_members{10'000};
    std::size_t max_array_length{100'000};
    std::size_t max_string_length{64 * 1024};
};

class BoundedJsonReader {
public:
    explicit BoundedJsonReader(std::string_view src, BoundedJsonConfig cfg = {});

    void skip_ws();
    bool expect(char c);
    bool has_more() const;
    std::size_t depth() const { return depth_; }

    std::string read_string();
    std::int64_t read_int64(std::int64_t fallback = 0);
    std::uint64_t read_uint64(std::uint64_t fallback = 0);
    double read_double(double fallback = 0.0);
    bool read_bool();
    bool read_null();

    char peek() const;
    void skip_value();

    std::string read_typed_value();

    bool has_error() const { return has_error_; }
    std::string const& error_message() const { return error_; }

private:
    std::string_view src_;
    std::size_t pos_{};
    std::size_t depth_{};
    BoundedJsonConfig cfg_;
    bool has_error_{false};
    std::string error_;

    void set_error(std::string msg);
    void enter_object();
    void leave_object();
    void enter_array();
    void leave_array();
    std::string read_raw_string();
};

// ── GeneValue type tags ───────────────────────────────────

enum class GeneValueTag : std::uint32_t {
    string_val = 0, bool_val = 1, int64_val = 2,
    uint64_val = 3, double_val = 4, string_list_val = 5
};

std::string gene_value_to_json(GeneValue const& value);
GeneValue json_to_gene_value(std::string_view json, GeneValueTag tag);
GeneValueTag gene_value_variant_index(GeneValue const& value);

} // namespace gspl
