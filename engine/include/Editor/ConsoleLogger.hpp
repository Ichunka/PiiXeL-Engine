#pragma once

#ifdef BUILD_WITH_EDITOR

#include <string>
#include <vector>
#include <mutex>
#include <raylib.h>

namespace PiiXeL {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error
};

enum class LogSource {
    Engine,
    Game
};

struct LogEntry {
    std::string message;
    LogLevel level;
    LogSource source;
    float timestamp;

    LogEntry(const std::string& msg, LogLevel lvl, LogSource src, float time)
        : message(msg), level(lvl), source(src), timestamp(time) {}
};

class ConsoleLogger {
public:
    static ConsoleLogger& Instance();

    void AddLog(const std::string& message, LogLevel level, LogSource source);
    void Clear();

    const std::vector<LogEntry>& GetLogs() const { return m_Logs; }

    // Callbacks pour Raylib
    static void RaylibLogCallback(int logLevel, const char* text, va_list args);

private:
    ConsoleLogger() = default;
    ~ConsoleLogger() = default;
    ConsoleLogger(const ConsoleLogger&) = delete;
    ConsoleLogger& operator=(const ConsoleLogger&) = delete;

    std::vector<LogEntry> m_Logs;
    std::mutex m_Mutex;

    static LogLevel ConvertRaylibLogLevel(int raylibLevel);
};

// Helper macros for easy logging
#define LOG_ENGINE_TRACE(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Trace, LogSource::Engine)
#define LOG_ENGINE_DEBUG(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Debug, LogSource::Engine)
#define LOG_ENGINE_INFO(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Info, LogSource::Engine)
#define LOG_ENGINE_WARNING(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Warning, LogSource::Engine)
#define LOG_ENGINE_ERROR(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Error, LogSource::Engine)

#define LOG_GAME_TRACE(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Trace, LogSource::Game)
#define LOG_GAME_DEBUG(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Debug, LogSource::Game)
#define LOG_GAME_INFO(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Info, LogSource::Game)
#define LOG_GAME_WARNING(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Warning, LogSource::Game)
#define LOG_GAME_ERROR(msg) ConsoleLogger::Instance().AddLog(msg, LogLevel::Error, LogSource::Game)

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
