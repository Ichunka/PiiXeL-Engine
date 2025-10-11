#ifdef BUILD_WITH_EDITOR

#include "Editor/ConsoleLogger.hpp"
#include <raylib.h>
#include <cstdio>
#include <cstdarg>

namespace PiiXeL {

ConsoleLogger& ConsoleLogger::Instance() {
    static ConsoleLogger instance;
    return instance;
}

void ConsoleLogger::AddLog(const std::string& message, LogLevel level, LogSource source) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    float timestamp = 0.0f;
    if (IsWindowReady()) {
        timestamp = static_cast<float>(GetTime());
    }
    m_Logs.emplace_back(message, level, source, timestamp);

    if (m_Logs.size() > 10000) {
        m_Logs.erase(m_Logs.begin(), m_Logs.begin() + 1000);
    }
}

void ConsoleLogger::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Logs.clear();
}

LogLevel ConsoleLogger::ConvertRaylibLogLevel(int raylibLevel) {
    switch (raylibLevel) {
        case LOG_TRACE:   return LogLevel::Trace;
        case LOG_DEBUG:   return LogLevel::Debug;
        case LOG_INFO:    return LogLevel::Info;
        case LOG_WARNING: return LogLevel::Warning;
        case LOG_ERROR:   return LogLevel::Error;
        default:          return LogLevel::Info;
    }
}

void ConsoleLogger::RaylibLogCallback(int logLevel, const char* text, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    std::string message(buffer);
    LogLevel level = ConvertRaylibLogLevel(logLevel);

    LogSource source = LogSource::Engine;
    if (message.find("[GAME]") != std::string::npos ||
        message.find("[Game]") != std::string::npos ||
        message.find("[SCRIPT]") != std::string::npos) {
        source = LogSource::Game;
        size_t pos = message.find("]");
        if (pos != std::string::npos) {
            message = message.substr(pos + 2);
        }
    }

    ConsoleLogger::Instance().AddLog(message, level, source);

    #ifdef _DEBUG
    printf("[%s] %s\n", source == LogSource::Engine ? "ENGINE" : "GAME", buffer);
    #endif
}

} // namespace PiiXeL

#endif // BUILD_WITH_EDITOR
