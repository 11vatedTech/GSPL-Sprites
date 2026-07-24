#include "gspl/studio/crash_reporter.hpp"
#include <ctime>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace gspl::studio {
namespace fs = std::filesystem;

struct CrashReporter::Impl {
    CrashCallback callback_;
    std::string dump_dir_;
    CrashReport last_report_;
    bool has_pending_{false};
};

CrashReporter::CrashReporter() : impl_(std::make_unique<Impl>()) {}
CrashReporter::~CrashReporter() = default;

void CrashReporter::set_crash_callback(CrashCallback cb) { impl_->callback_ = std::move(cb); }

void CrashReporter::set_dump_directory(const std::string& dir) {
    impl_->dump_dir_ = dir;
    fs::create_directories(dir);
}

bool CrashReporter::generate_report(const std::string& exception_type,
                                    const std::string& message,
                                    const std::string& stack_trace) {
    CrashReport report;
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &tt);
    std::ostringstream ts;
    ts << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    report.timestamp = ts.str();
    report.exception_type = exception_type;
    report.message = message;
    report.stack_trace = stack_trace;
    report.version = "0.1.0";

    impl_->last_report_ = report;
    impl_->has_pending_ = true;

    if (!impl_->dump_dir_.empty()) {
        save_report(report, impl_->dump_dir_ + "/crash_" + ts.str() + ".txt");
    }
    if (impl_->callback_) {
        impl_->callback_(report);
    }
    return true;
}

auto CrashReporter::last_report() const -> const CrashReport& {
    return impl_->last_report_;
}

bool CrashReporter::has_pending_report() const { return impl_->has_pending_; }
void CrashReporter::clear_pending() { impl_->has_pending_ = false; }

bool CrashReporter::save_report(const CrashReport& report, const std::string& path) const {
    auto out_path = path.empty() ? "crash_report.txt" : path;
    std::ofstream f(out_path);
    if (!f) return false;
    f << "Crash Report\n";
    f << "============\n";
    f << "Timestamp: " << report.timestamp << "\n";
    f << "Version: " << report.version << "\n";
    f << "Type: " << report.exception_type << "\n";
    f << "Message: " << report.message << "\n";
    if (!report.stack_trace.empty()) {
        f << "Stack Trace:\n" << report.stack_trace << "\n";
    }
    return true;
}

auto CrashReporter::load_report(const std::string& path) const -> CrashReport {
    CrashReport report;
    std::ifstream f(path);
    if (!f) return report;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find("Timestamp: ") == 0) report.timestamp = line.substr(11);
        else if (line.find("Version: ") == 0) report.version = line.substr(9);
        else if (line.find("Type: ") == 0) report.exception_type = line.substr(6);
        else if (line.find("Message: ") == 0) report.message = line.substr(9);
    }
    return report;
}

} // namespace gspl::studio
