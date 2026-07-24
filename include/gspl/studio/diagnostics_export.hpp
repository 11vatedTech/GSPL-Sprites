#pragma once

#include <string>
#include <memory>
#include <vector>

namespace gspl::studio {

struct DiagnosticsBundle {
    std::string timestamp;
    std::string log_content;
    std::string system_info;
    std::string workspace_summary;
    std::vector<std::string> included_files;
    std::string target_path;
};

class DiagnosticsExport {
public:
    DiagnosticsExport();
    ~DiagnosticsExport();

    DiagnosticsExport(const DiagnosticsExport&) = delete;
    DiagnosticsExport& operator=(const DiagnosticsExport&) = delete;

    bool export_to_file(const std::string& path);
    [[nodiscard]] auto collect_system_info() const -> std::string;
    [[nodiscard]] auto collect_logs() const -> std::string;

    void set_workspace_path(const std::string& path);
    void set_include_logs(bool include);
    void set_include_system_info(bool include);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::studio
