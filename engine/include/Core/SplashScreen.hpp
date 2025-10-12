#ifndef PIIXELENGINE_SPLASHSCREEN_HPP
#define PIIXELENGINE_SPLASHSCREEN_HPP

#include <raylib.h>
#include <string>
#include <string_view>

namespace PiiXeL {

class SplashScreen {
public:
    SplashScreen();
    ~SplashScreen();

    void Show(const std::string& imagePath, float minDurationSeconds = 3.0f);
    void ShowEmbedded(std::string_view assetName, float minDurationSeconds = 3.0f);
    void Update(float deltaTime);
    void Render();

    [[nodiscard]] bool IsFinished() const { return m_Finished; }
    void MarkLoadingComplete() { m_LoadingComplete = true; }

private:
    Texture2D m_SplashTexture;
    float m_ElapsedTime{0.0f};
    float m_MinDuration{3.0f};
    float m_FadeAlpha{0.0f};
    bool m_LoadingComplete{false};
    bool m_Finished{false};
    bool m_IsShowing{false};
    bool m_FadingOut{false};

    static constexpr float FADE_IN_DURATION{0.3f};
    static constexpr float FADE_OUT_DURATION{0.5f};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_SPLASHSCREEN_HPP
