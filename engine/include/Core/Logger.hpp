#pragma once

#include <cstdio>
#include <cstdarg>

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
    Runtime,
    Game
};

enum class LogCategory {
    ENGINE,
    ASSET,
    EDITOR,
    PHYSICS,
    RENDER,
    SCENE,
    SCRIPT,
    ANIMATION,
    BUILD,
    GAME,
    UNKNOWN
};

class Logger {
public:
    static bool IsCategoryEnabled(LogCategory category);
    static void SetCategoryEnabled(LogCategory category, bool enabled);
    static const char* GetCategoryName(LogCategory category);
    static void LogMessage(LogLevel level, LogCategory category, const char* format, ...);

private:
    static bool s_CategoryEnabled[12];
};

} // namespace PiiXeL

#if defined(NDEBUG) && !defined(BUILD_WITH_EDITOR)
    #define PX_LOG_INFO(category, ...) ((void)0)
    #define PX_LOG_WARNING(category, ...) ((void)0)
    #define PX_LOG_ERROR(category, ...) PiiXeL::Logger::LogMessage(PiiXeL::LogLevel::Error, PiiXeL::LogCategory::category, __VA_ARGS__)
#else
    #define PX_LOG_INFO(category, ...) \
        do { \
            if (PiiXeL::Logger::IsCategoryEnabled(PiiXeL::LogCategory::category)) { \
                PiiXeL::Logger::LogMessage(PiiXeL::LogLevel::Info, PiiXeL::LogCategory::category, __VA_ARGS__); \
            } \
        } while(0)

    #define PX_LOG_WARNING(category, ...) \
        do { \
            if (PiiXeL::Logger::IsCategoryEnabled(PiiXeL::LogCategory::category)) { \
                PiiXeL::Logger::LogMessage(PiiXeL::LogLevel::Warning, PiiXeL::LogCategory::category, __VA_ARGS__); \
            } \
        } while(0)

    #define PX_LOG_ERROR(category, ...) \
        do { \
            if (PiiXeL::Logger::IsCategoryEnabled(PiiXeL::LogCategory::category)) { \
                PiiXeL::Logger::LogMessage(PiiXeL::LogLevel::Error, PiiXeL::LogCategory::category, __VA_ARGS__); \
            } \
        } while(0)
#endif
