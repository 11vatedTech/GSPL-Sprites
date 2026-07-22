#include "gspl/legacy.hpp"
#include "gspl/cache.hpp"
#include "gspl/provider.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace gspl {

LegacySpriteParser::LegacySpriteParser(SourceBuffer const& source) : source_(source) {}

bool is_legacy_sprite_format(std::string_view content) {
    auto trimmed = content;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t' ||
                                trimmed.front() == '\n' || trimmed.front() == '\r')) {
        trimmed.remove_prefix(1);
    }
    return trimmed.starts_with("sprite ") || trimmed.starts_with("SPRITE ") ||
           trimmed.starts_with("entity ") || trimmed.starts_with("seed ");
}

DiagnosticResult LegacySpriteParser::parse() {
    auto content = source_.content();
    std::istringstream stream{std::string(content)};
    std::string line;
    std::size_t line_num = 0;

    result_.stable_id = source_.identifier();
    result_.rights = "ORIGINAL_USER_CREATION";

    while (std::getline(stream, line)) {
        ++line_num;
        auto trim_line = line;
        while (!trim_line.empty() && (trim_line.front() == ' ' || trim_line.front() == '\t'))
            trim_line.erase(trim_line.begin());
        if (trim_line.empty() || trim_line[0] == '#') continue;

        if (trim_line.starts_with("stable_id:")) {
            auto val = trim_line.substr(10);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            result_.stable_id = val;
        } else if (trim_line.starts_with("name:")) {
            auto val = trim_line.substr(5);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            result_.name = val;
        } else if (trim_line.starts_with("classification:")) {
            auto val = trim_line.substr(15);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            result_.classification = val;
        } else if (trim_line.starts_with("color:")) {
            auto val = trim_line.substr(6);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            result_.primary_color = val;
        } else if (trim_line.starts_with("rights:")) {
            auto val = trim_line.substr(6);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            result_.rights = val;
        } else if (trim_line.starts_with("form:")) {
            auto val = trim_line.substr(5);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            CanonicalForm cf;
            cf.id = val;
            result_.forms.push_back(cf);
        } else if (trim_line.starts_with("ability:")) {
            auto val = trim_line.substr(8);
            while (!val.empty() && (val.front() == ' ' || val.front() == '"')) val.erase(val.begin());
            while (!val.empty() && (val.back() == ' ' || val.back() == '"' || val.back() == ';')) val.pop_back();
            result_.abilities.push_back({val, "generic.effect", 5, 10, 4});
        }
    }
    return diags_;
}

CanonicalEntity LegacySpriteParser::to_canonical() const {
    return result_;
}

std::vector<std::pair<std::size_t, std::size_t>> LegacySpriteParser::source_mappings() const {
    return {};
}

MigrateReport migrate_file(std::filesystem::path const& input_path, MigrateOptions const& opts) {
    MigrateReport report;
    try {
        if (!std::filesystem::exists(input_path)) {
            report.errors.push_back("Input file does not exist: " + input_path.string());
            return report;
        }
        auto ext = input_path.extension().string();
        if (ext != ".sprite" && ext != ".txt") {
            report.files_skipped++;
            report.warnings.push_back("Skipping non-legacy extension: " + input_path.string());
            return report;
        }
        std::ifstream f(input_path, std::ios::binary);
        if (!f) {
            report.errors.push_back("Cannot open input file: " + input_path.string());
            return report;
        }
        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (!is_legacy_sprite_format(content)) {
            report.files_skipped++;
            return report;
        }
        report.files_processed++;

        auto buf = SourceBuffer::from_file(input_path);
        LegacySpriteParser parser(buf);
        auto parse_result = parser.parse();
        for (auto const& d : parse_result.diagnostics) {
            report.warnings.push_back(d.message);
        }
        if (!parse_result.ok()) {
            report.nonconvertible_constructs += parse_result.diagnostics.size();
        }
        auto canonical = parser.to_canonical();
        if (opts.output_dir.empty()) {
            report.files_converted++;
            return report;
        }
        std::filesystem::create_directories(opts.output_dir);
        auto out_path = opts.output_dir / (input_path.stem().string() + ".gspl");
        if (std::filesystem::exists(out_path) && !opts.overwrite) {
            report.warnings.push_back("Output exists and overwrite not set: " + out_path.string());
            report.files_skipped++;
            return report;
        }
        {
            std::ofstream of(out_path);
            if (!of) {
                report.errors.push_back("Cannot write output file: " + out_path.string());
                return report;
            }
            of << "module " << input_path.stem().string() << ";\n";
            of << "entity " << input_path.stem().string() << " {\n";
            of << "  rights " << canonical.rights << ";\n";
            if (!canonical.stable_id.empty()) {
                of << "  gene identity { stable_id: \"" << canonical.stable_id << "\"; }\n";
            }
            if (!canonical.classification.empty()) {
                of << "  gene classification { taxonomy: \"" << canonical.classification << "\"; }\n";
            }
            for (auto const& form : canonical.forms) {
                of << "  form " << form.id << " {}\n";
            }
            of << "  morphology {}\n";
            of << "}\n";
        }
        if (opts.verify_output) {
            try {
                auto verify_buf = SourceBuffer::from_file(out_path);
                (void)verify_buf;
            } catch (...) {
                report.warnings.push_back("Verification compile warning for: " + out_path.string());
            }
        }
        report.files_converted++;
    } catch (std::exception const& e) {
        report.errors.push_back(e.what());
    }
    return report;
}

MigrateReport migrate_directory(std::filesystem::path const& input_dir, MigrateOptions const& opts) {
    MigrateReport report;
    try {
        if (!std::filesystem::exists(input_dir) || !std::filesystem::is_directory(input_dir)) {
            report.errors.push_back("Input directory does not exist: " + input_dir.string());
            return report;
        }
        for (auto const& entry : std::filesystem::recursive_directory_iterator(input_dir)) {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            if (ext != ".sprite" && ext != ".txt") continue;
            auto file_report = migrate_file(entry.path(), opts);
            report.files_processed += file_report.files_processed;
            report.files_converted += file_report.files_converted;
            report.files_skipped += file_report.files_skipped;
            report.nonconvertible_constructs += file_report.nonconvertible_constructs;
            report.warnings.insert(report.warnings.end(), file_report.warnings.begin(), file_report.warnings.end());
            report.errors.insert(report.errors.end(), file_report.errors.begin(), file_report.errors.end());
        }
    } catch (std::exception const& e) {
        report.errors.push_back(e.what());
    }
    return report;
}

} // namespace gspl
