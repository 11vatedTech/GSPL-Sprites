#pragma once
#include "gspl/diagnostics.hpp"
#include "gspl/source.hpp"
#include "gspl/semantics.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace gspl {

struct MigrateOptions {
    bool dry_run{false};
    bool preserve_comments{true};
    bool verify_output{true};
    bool overwrite{false};
    std::filesystem::path output_dir;
};

struct MigrateReport {
    std::size_t files_processed{};
    std::size_t files_converted{};
    std::size_t files_skipped{};
    std::size_t nonconvertible_constructs{};
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    bool success() const { return errors.empty(); }
};

class LegacySpriteParser {
public:
    explicit LegacySpriteParser(SourceBuffer const& source);
    DiagnosticResult parse();
    CanonicalEntity to_canonical() const;
    std::vector<std::pair<std::size_t, std::size_t>> source_mappings() const;
private:
    SourceBuffer const& source_;
    DiagnosticResult diags_;
    CanonicalEntity result_;
};

MigrateReport migrate_file(std::filesystem::path const& input_path, MigrateOptions const& opts);
MigrateReport migrate_directory(std::filesystem::path const& input_dir, MigrateOptions const& opts);
bool is_legacy_sprite_format(std::string_view content);

} // namespace gspl
