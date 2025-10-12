#include "Core/SplashScreen.hpp"
#include <raylib.h>

namespace PiiXeL {

SplashScreen::SplashScreen(int width, int height, const std::string& title)
    : m_Width{width}
    , m_Height{height}
{
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
}

SplashScreen::~SplashScreen() {
    if (m_IsOpen) {
        Close();
    }
}

void SplashScreen::UpdateProgress(float progress, const std::string& message) {
    m_Progress = progress;
    m_Message = message;
}

void SplashScreen::Render() {
    BeginDrawing();
    ClearBackground(Color{20, 20, 25, 255});

    const char* title = "PiiXeL Engine";
    int titleFontSize = 48;
    int titleWidth = MeasureText(title, titleFontSize);
    DrawText(title, (m_Width - titleWidth) / 2, m_Height / 3, titleFontSize, WHITE);

    int barWidth = m_Width - 200;
    int barHeight = 30;
    int barX = (m_Width - barWidth) / 2;
    int barY = m_Height / 2;

    DrawRectangle(barX, barY, barWidth, barHeight, Color{40, 40, 45, 255});
    DrawRectangle(barX, barY, static_cast<int>(barWidth * m_Progress), barHeight, Color{100, 150, 255, 255});
    DrawRectangleLines(barX, barY, barWidth, barHeight, Color{80, 80, 85, 255});

    int percentage = static_cast<int>(m_Progress * 100.0f);
    std::string percentText = std::to_string(percentage) + "%";
    int percentFontSize = 20;
    int percentWidth = MeasureText(percentText.c_str(), percentFontSize);
    DrawText(percentText.c_str(), (m_Width - percentWidth) / 2, barY + barHeight + 20, percentFontSize, LIGHTGRAY);

    if (!m_Message.empty()) {
        int messageFontSize = 16;
        int messageWidth = MeasureText(m_Message.c_str(), messageFontSize);
        DrawText(m_Message.c_str(), (m_Width - messageWidth) / 2, barY + barHeight + 60, messageFontSize, GRAY);
    }

    EndDrawing();
}

void SplashScreen::Close() {
    CloseWindow();
    m_IsOpen = false;
}

bool SplashScreen::ShouldClose() const {
    return WindowShouldClose();
}

} // namespace PiiXeL
