#ifndef PIIXELENGINE_CORE_SPLASHSCREEN_HPP
#define PIIXELENGINE_CORE_SPLASHSCREEN_HPP

#include <raylib.h>
#include <string>

namespace PiiXeL {

class SplashScreen {
public:
    SplashScreen();
    ~SplashScreen();

    void UpdateProgress(float progress, const std::string& message);
    void Render();

    void ShowEmbedded(const std::string& assetName, float duration);
    void Update(float deltaTime);
    [[nodiscard]] bool IsFinished() const;
    void MarkLoadingComplete();

private:
    float m_Progress{0.0f};
    std::string m_Message{};

    Texture2D m_EmbeddedTexture{};
    float m_Duration{0.0f};
    float m_ElapsedTime{0.0f};
    bool m_LoadingComplete{false};
    bool m_UseEmbedded{false};
    float m_FadeAlpha{0.0f};
};

} // namespace PiiXeL

#endif
