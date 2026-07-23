#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <chrono>
#include <sstream>

namespace gspl::studio {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warning = 3,
    Error = 4,
    Fatal = 5
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level{LogLevel::Info};
    std::string component;
    std::string message;
    std::string file;
    int line{0};
};

class Logger {
public:
    using LogCallback = std::function<void(const LogEntry&)>;

    static Logger& instance();

    void set_level(LogLevel min_level);
    [[nodiscard]] LogLevel level() const;

    void set_callback(LogCallback cb);

    void log(LogLevel level, std::string_view component, std::string_view message,
             std::string_view file = "", int line = 0);

    void trace(std::string_view component, std::string_view message);
    void debug(std::string_view component, std::string_view message);
    void info(std::string_view component, std::string_view message);
    void warn(std::string_view component, std::string_view message);
    void error(std::string_view component, std::string_view message);
    void fatal(std::string_view component, std::string_view message);

    [[nodiscard]] static std::string level_name(LogLevel level);
    [[nodiscard]] static LogLevel level_from_name(std::string_view name);

private:
    Logger() = default;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Convenience macros
#define LOG_TRACE(comp, msg) gspl::studio::Logger::instance().trace(comp, msg)
#define LOG_DEBUG(comp, msg) gspl::studio::Logger::instance().debug(comp, msg)
#define LOG_INFO(comp, msg)  gspl::studio::Logger::instance().info(comp, msg)
#define LOG_WARN(comp, msg)  gspl::studio::Logger::instance().warn(comp, msg)
#define LOG_ERROR(comp, msg) gspl::studio::Logger::instance().error(comp, msg)
#define LOG_FATAL(comp, msg) gspl::studio::Logger::instance().fatal(comp, msg)

} // namespace gspl::studio
