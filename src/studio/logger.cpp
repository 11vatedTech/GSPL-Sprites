#include "gspl/studio/logger.hpp"
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <ctime>
#include <iomanip>

namespace gspl::studio {

struct Logger::Impl {
    LogLevel min_level{LogLevel::Info};
    LogCallback callback;
    std::mutex mutex;
};

Logger& Logger::instance() {
    static Logger logger;
    if (!logger.impl_) logger.impl_ = std::make_unique<Impl>();
    return logger;
}

void Logger::set_level(LogLevel min_level) { impl_->min_level = min_level; }
LogLevel Logger::level() const { return impl_->min_level; }

void Logger::set_callback(LogCallback cb) { impl_->callback = cb; }

void Logger::log(LogLevel level, std::string_view component, std::string_view message,
                 std::string_view file, int line) {
    if (level < impl_->min_level) return;
    
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.component = component;
    entry.message = message;
    entry.file = file;
    entry.line = line;
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->callback) {
        impl_->callback(entry);
    }
    
    // Default stderr output
    auto tt = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm tm;
    localtime_s(&tm, &tt);
    std::cerr << std::put_time(&tm, "%H:%M:%S") << " [" << level_name(level) << "] "
              << "(" << component << ") " << message << std::endl;
}

void Logger::trace(std::string_view component, std::string_view message) { log(LogLevel::Trace, component, message); }
void Logger::debug(std::string_view component, std::string_view message) { log(LogLevel::Debug, component, message); }
void Logger::info(std::string_view component, std::string_view message)  { log(LogLevel::Info, component, message); }
void Logger::warn(std::string_view component, std::string_view message)  { log(LogLevel::Warning, component, message); }
void Logger::error(std::string_view component, std::string_view message) { log(LogLevel::Error, component, message); }
void Logger::fatal(std::string_view component, std::string_view message) { log(LogLevel::Fatal, component, message); }

std::string Logger::level_name(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "UNKNOWN";
}

LogLevel Logger::level_from_name(std::string_view name) {
    static const std::unordered_map<std::string, LogLevel> map = {
        {"TRACE", LogLevel::Trace}, {"DEBUG", LogLevel::Debug},
        {"INFO", LogLevel::Info}, {"WARN", LogLevel::Warning},
        {"ERROR", LogLevel::Error}, {"FATAL", LogLevel::Fatal}
    };
    auto it = map.find(std::string(name));
    return it != map.end() ? it->second : LogLevel::Info;
}

} // namespace gspl::studio
