#include "gspl/studio/diagnostics_export.hpp"
#include "gspl/studio/logger.hpp"
#include <ctime>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace gspl::studio {
namespace fs = std::filesystem;

struct DiagnosticsExport::Impl {
    std::string workspace_path_;
    bool include_logs_{true};
    bool include_system_info_{true};
};

DiagnosticsExport::DiagnosticsExport() : impl_(std::make_unique<Impl>()) {}
DiagnosticsExport::~DiagnosticsExport() = default;

bool DiagnosticsExport::export_to_file(const std::string& path) {
    std::ofstream out(path);
    if (!out) return false;

    out << "=== GSPL Diagnostics Bundle ===\n";
    out << "Timestamp: " << collect_system_info().substr(0, 30) << "\n\n";

    if (impl_->include_logs_) {
        out << "--- Logs ---\n" << collect_logs() << "\n";
    }
    if (impl_->include_system_info_) {
        out << "--- System Info ---\n" << collect_system_info() << "\n";
    }
    if (!impl_->workspace_path_.empty()) {
        out << "--- Workspace ---\n" << impl_->workspace_path_ << "\n";
        size_t file_count = 0;
        if (fs::exists(impl_->workspace_path_)) {
            for (auto& f : fs::recursive_directory_iterator(impl_->workspace_path_)) {
                if (f.is_regular_file()) ++file_count;
            }
        }
        out << "Files: " << file_count << "\n";
    }
    return true;
}

auto DiagnosticsExport::collect_system_info() const -> std::string {
    std::ostringstream oss;
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &tt);
    oss << "Generated: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
#ifdef _WIN32
    oss << "Platform: Windows\n";
#elif __linux__
    oss << "Platform: Linux\n";
#elif __APPLE__
    oss << "Platform: macOS\n";
#else
    oss << "Platform: Unknown\n";
#endif
    oss << "Version: GSPL Sprites 0.1.0\n";
    return oss.str();
}

auto DiagnosticsExport::collect_logs() const -> std::string {
    std::ostringstream oss;
    oss << "(log capture — see application log file)\n";
    return oss.str();
}

void DiagnosticsExport::set_workspace_path(const std::string& path) {
    impl_->workspace_path_ = path;
}

void DiagnosticsExport::set_include_logs(bool include) { impl_->include_logs_ = include; }
void DiagnosticsExport::set_include_system_info(bool include) { impl_->include_system_info_ = include; }

} // namespace gspl::studio
