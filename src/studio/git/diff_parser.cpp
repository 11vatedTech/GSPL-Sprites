#include "gspl/studio/diff_parser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace gspl::studio {

bool DiffFile::is_gspl() const {
    auto fn = filename();
    return fn.size() >= 5 &&
           (fn.substr(fn.size() - 5) == ".gspl" ||
            (fn.size() >= 6 && fn.substr(fn.size() - 6) == ".gsplj") ||
            fn == "gspl-project.jsonc");
}

std::string DiffFile::filename() const {
    auto pos = new_path.rfind('/');
    if (pos == std::string_view::npos)
        pos = new_path.rfind('\\');
    if (pos == std::string_view::npos)
        return new_path;
    return new_path.substr(pos + 1);
}

std::vector<DiffFile> DiffParser::parse(std::string_view unified_diff) {
    std::vector<DiffFile> result;
    DiffFile current;
    bool in_hunk = false;

    std::string input(unified_diff);
    std::istringstream stream(input);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        if (line.substr(0, 4) == "--- ") {
            // Previous file - save if we have content
            if (!current.old_path.empty() && !current.hunks.empty()) {
                result.push_back(std::move(current));
                current = DiffFile{};
                in_hunk = false;
            }
            current.old_path = line.substr(4);
            // Strip leading a/ prefix
            if (current.old_path.size() > 2 && current.old_path[0] == 'a' && current.old_path[1] == '/')
                current.old_path = current.old_path.substr(2);
            in_hunk = false;
        } else if (line.substr(0, 4) == "+++ ") {
            current.new_path = line.substr(4);
            if (current.new_path.size() > 2 && current.new_path[0] == 'b' && current.new_path[1] == '/')
                current.new_path = current.new_path.substr(2);
        } else if (line.substr(0, 2) == "@@" ) {
            // Hunk header: @@ -old_start,old_count +new_start,new_count @@
            DiffHunk hunk;
            hunk.header = line;

            // Parse: @@ -old_start,old_count +new_start,new_count @@
            auto at2 = line.find('@', 2);
            if (at2 != std::string_view::npos) {
                auto content = line.substr(2, at2 - 2);
                // Parse -old_start,old_count +new_start,new_count
                auto comma1 = content.find(',');
                auto plus = content.find('+');
                auto comma2 = content.find(',', plus);

                auto parse_num = [](std::string_view s) -> int {
                    int val = 0;
                    for (char c : s) {
                        if (c >= '0' && c <= '9') val = val * 10 + (c - '0');
                    }
                    return val;
                };

                std::string old_part(content.substr(1, (comma1 != std::string_view::npos) ? comma1 - 1 : plus - 2));
                hunk.old_start = parse_num(old_part);
                if (comma1 != std::string_view::npos) {
                    std::string old_cnt(content.substr(comma1 + 1, plus - comma1 - 2));
                    hunk.old_count = parse_num(old_cnt);
                } else {
                    hunk.old_count = 1;
                }

                std::string new_part(content.substr(plus + 1, (comma2 != std::string_view::npos) ? comma2 - plus - 1 : std::string_view::npos));
                hunk.new_start = parse_num(new_part);
                if (comma2 != std::string_view::npos) {
                    std::string new_cnt(content.substr(comma2 + 1));
                    hunk.new_count = parse_num(new_cnt);
                } else {
                    hunk.new_count = 1;
                }
            }

            current.hunks.push_back(std::move(hunk));
            in_hunk = true;
        } else if (in_hunk && !current.hunks.empty()) {
            auto& hunk = current.hunks.back();
            DiffLine dl;
            dl.content = line;

            if (line.size() >= 1) {
                char prefix = line[0];
                switch (prefix) {
                    case ' ': dl.type = DiffLineType::Context; break;
                    case '+': dl.type = DiffLineType::Added; break;
                    case '-': dl.type = DiffLineType::Removed; break;
                    default:  dl.type = DiffLineType::Header; break;
                }
            }

            // Track old/new line numbers
            auto& prev = hunk.lines.empty() ? dl : hunk.lines.back();
            if (dl.type == DiffLineType::Context || dl.type == DiffLineType::Removed) {
                dl.old_line_no = (prev.old_line_no >= 0) ? prev.old_line_no + 1 : hunk.old_start;
            }
            if (dl.type == DiffLineType::Context || dl.type == DiffLineType::Added) {
                dl.new_line_no = (prev.new_line_no >= 0) ? prev.new_line_no + 1 : hunk.new_start;
            }

            hunk.lines.push_back(std::move(dl));
        }
    }

    // Save last file
    if (!current.old_path.empty() && !current.hunks.empty()) {
        result.push_back(std::move(current));
    }

    return result;
}

std::vector<DiffFile> DiffParser::filter_gspl(std::vector<DiffFile> const& files) {
    std::vector<DiffFile> result;
    for (auto const& f : files) {
        if (f.is_gspl()) result.push_back(f);
    }
    return result;
}

DiffParser::DiffStats DiffParser::stats(std::vector<DiffFile> const& files) {
    DiffStats s;
    s.files_changed = static_cast<int>(files.size());
    s.gspl_files = 0;
    for (auto const& f : files) {
        if (f.is_gspl()) s.gspl_files++;
        for (auto const& h : f.hunks) {
            for (auto const& l : h.lines) {
                if (l.type == DiffLineType::Added) s.insertions++;
                if (l.type == DiffLineType::Removed) s.deletions++;
            }
        }
    }
    return s;
}

} // namespace gspl::studio
