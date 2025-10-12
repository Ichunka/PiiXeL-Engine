#ifndef PIIXELENGINE_CORE_SPLASHSCREEN_HPP
#define PIIXELENGINE_CORE_SPLASHSCREEN_HPP

#include <string>
#include <raylib.h>

namespace PiiXeL {

class SplashScreen {
public:
    SplashScreen(int width, int height, const std::string& title);
    ~SplashScreen();

    void UpdateProgress(float progress, const std::string& message);
    void Render();
    void Close();

    [[nodiscard]] bool ShouldClose() const;

private:
    int m_Width;
    int m_Height;
    float m_Progress{0.0f};
    std::string m_Message{};
    bool m_IsOpen{true};
};

} // namespace PiiXeL

#endif
