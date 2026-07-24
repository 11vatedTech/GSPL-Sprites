#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::studio {

enum class DiffLineType : std::uint8_t {
    Context,
    Added,
    Removed,
    Header,
    HunkHeader,
    NoNewline,
};

struct DiffLine {
    DiffLineType type = DiffLineType::Context;
    std::string content;
    int old_line_no = -1;
    int new_line_no = -1;
};

struct DiffHunk {
    int old_start = 0;
    int old_count = 0;
    int new_start = 0;
    int new_count = 0;
    std::string header;
    std::vector<DiffLine> lines;
};

struct DiffFile {
    std::string old_path;
    std::string new_path;
    std::string status;  // M, A, D, R, etc.
    std::vector<DiffHunk> hunks;

    bool is_gspl() const;
    std::string filename() const;
};

class DiffParser {
public:
    // Parse unified diff output into structured format
    static std::vector<DiffFile> parse(std::string_view unified_diff);

    // Filter to only GSPL files (.gspl extension)
    static std::vector<DiffFile> filter_gspl(std::vector<DiffFile> const& files);

    // Get summary statistics
    struct DiffStats {
        int files_changed = 0;
        int insertions = 0;
        int deletions = 0;
        int gspl_files = 0;
    };
    static DiffStats stats(std::vector<DiffFile> const& files);
};

} // namespace gspl::studio
