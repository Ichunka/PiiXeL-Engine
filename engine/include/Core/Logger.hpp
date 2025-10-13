#pragma once

#include <raylib.h>

namespace PiiXeL {

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
    GAME
};

class Logger {
public:
    static bool IsCategoryEnabled(LogCategory category);
    static void SetCategoryEnabled(LogCategory category, bool enabled);
    static const char* GetCategoryName(LogCategory category);

private:
    static bool s_CategoryEnabled[11];
};

} // namespace PiiXeL

#if defined(NDEBUG) && !defined(BUILD_WITH_EDITOR)
    #define PX_LOG_INFO(category, ...) ((void)0)
    #define PX_LOG_WARNING(category, ...) ((void)0)
    #define PX_LOG_ERROR(category, ...) TraceLog(LOG_ERROR, "[%s] " __VA_ARGS__, PiiXeL::Logger::GetCategoryName(PiiXeL::LogCategory::category))
#else
    #define PX_LOG_INFO(category, ...) \
        do { \
            if (PiiXeL::Logger::IsCategoryEnabled(PiiXeL::LogCategory::category)) { \
                TraceLog(LOG_INFO, "[%s] " __VA_ARGS__, PiiXeL::Logger::GetCategoryName(PiiXeL::LogCategory::category)); \
            } \
        } while(0)

    #define PX_LOG_WARNING(category, ...) \
        do { \
            if (PiiXeL::Logger::IsCategoryEnabled(PiiXeL::LogCategory::category)) { \
                TraceLog(LOG_WARNING, "[%s] " __VA_ARGS__, PiiXeL::Logger::GetCategoryName(PiiXeL::LogCategory::category)); \
            } \
        } while(0)

    #define PX_LOG_ERROR(category, ...) \
        do { \
            if (PiiXeL::Logger::IsCategoryEnabled(PiiXeL::LogCategory::category)) { \
                TraceLog(LOG_ERROR, "[%s] " __VA_ARGS__, PiiXeL::Logger::GetCategoryName(PiiXeL::LogCategory::category)); \
            } \
        } while(0)
#endif
