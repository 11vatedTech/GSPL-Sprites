#pragma once

#include <string>
#include <memory>
#include <functional>

namespace gspl::studio {

struct CrashReport {
    std::string timestamp;
    std::string exception_type;
    std::string message;
    std::string stack_trace;
    std::string version;
    std::string log_snapshot;
};

using CrashCallback = std::function<void(const CrashReport&)>;

class CrashReporter {
public:
    CrashReporter();
    ~CrashReporter();

    CrashReporter(const CrashReporter&) = delete;
    CrashReporter& operator=(const CrashReporter&) = delete;

    void set_crash_callback(CrashCallback cb);
    void set_dump_directory(const std::string& dir);

    bool generate_report(const std::string& exception_type,
                         const std::string& message,
                         const std::string& stack_trace);
    [[nodiscard]] auto last_report() const -> const CrashReport&;
    [[nodiscard]] bool has_pending_report() const;
    void clear_pending();

    bool save_report(const CrashReport& report, const std::string& path = "") const;
    [[nodiscard]] auto load_report(const std::string& path) const -> CrashReport;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
