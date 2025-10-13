#include "Core/SplashScreen.hpp"
#include "Core/Logger.hpp"
#include "Resources/EmbeddedAssetLoader.hpp"
#include <raylib.h>

namespace PiiXeL {

SplashScreen::SplashScreen() = default;

SplashScreen::~SplashScreen() {
    if (m_UseEmbedded && m_EmbeddedTexture.id != 0) {
        UnloadTexture(m_EmbeddedTexture);
    }
}

void SplashScreen::UpdateProgress(float progress, const std::string& message) {
    m_Progress = progress;
    m_Message = message;
}

void SplashScreen::Render() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (m_UseEmbedded) {
        ClearBackground(Color{0, 0, 0, 255});

        if (m_EmbeddedTexture.id != 0) {
            float scale = 1.0f;
            int texWidth = m_EmbeddedTexture.width;
            int texHeight = m_EmbeddedTexture.height;

            if (texWidth > screenWidth || texHeight > screenHeight) {
                float scaleX = static_cast<float>(screenWidth) / static_cast<float>(texWidth);
                float scaleY = static_cast<float>(screenHeight) / static_cast<float>(texHeight);
                scale = (scaleX < scaleY) ? scaleX : scaleY;
            }

            int drawWidth = static_cast<int>(texWidth * scale);
            int drawHeight = static_cast<int>(texHeight * scale);
            int drawX = (screenWidth - drawWidth) / 2;
            int drawY = (screenHeight - drawHeight) / 2;

            unsigned char alpha = static_cast<unsigned char>(m_FadeAlpha * 255.0f);
            DrawTexturePro(
                m_EmbeddedTexture,
                Rectangle{0.0f, 0.0f, static_cast<float>(texWidth), static_cast<float>(texHeight)},
                Rectangle{static_cast<float>(drawX), static_cast<float>(drawY), static_cast<float>(drawWidth), static_cast<float>(drawHeight)},
                Vector2{0.0f, 0.0f},
                0.0f,
                Color{255, 255, 255, alpha}
            );
        }
    } else {
        ClearBackground(Color{20, 20, 25, 255});

        const char* title = "PiiXeL Engine";
        int titleFontSize = 48;
        int titleWidth = MeasureText(title, titleFontSize);
        DrawText(title, (screenWidth - titleWidth) / 2, screenHeight / 3, titleFontSize, WHITE);

        int barWidth = screenWidth - 200;
        int barHeight = 30;
        int barX = (screenWidth - barWidth) / 2;
        int barY = screenHeight / 2;

        DrawRectangle(barX, barY, barWidth, barHeight, Color{40, 40, 45, 255});
        DrawRectangle(barX, barY, static_cast<int>(barWidth * m_Progress), barHeight, Color{100, 150, 255, 255});
        DrawRectangleLines(barX, barY, barWidth, barHeight, Color{80, 80, 85, 255});

        int percentage = static_cast<int>(m_Progress * 100.0f);
        std::string percentText = std::to_string(percentage) + "%";
        int percentFontSize = 20;
        int percentWidth = MeasureText(percentText.c_str(), percentFontSize);
        DrawText(percentText.c_str(), (screenWidth - percentWidth) / 2, barY + barHeight + 20, percentFontSize, LIGHTGRAY);

        if (!m_Message.empty()) {
            int messageFontSize = 16;
            int messageWidth = MeasureText(m_Message.c_str(), messageFontSize);
            DrawText(m_Message.c_str(), (screenWidth - messageWidth) / 2, barY + barHeight + 60, messageFontSize, GRAY);
        }
    }
}

void SplashScreen::ShowEmbedded(const std::string& assetName, float duration) {
    m_UseEmbedded = true;
    m_Duration = duration;
    m_ElapsedTime = 0.0f;
    m_LoadingComplete = false;
    m_FadeAlpha = 0.0f;

    m_EmbeddedTexture = EmbeddedAssetLoader::LoadTextureFromEmbedded(assetName);
    if (m_EmbeddedTexture.id == 0) {
        PX_LOG_WARNING(ENGINE, "Failed to load embedded splash screen texture: %s", assetName.c_str());
    }
}

void SplashScreen::Update(float deltaTime) {
    if (!m_UseEmbedded) {
        return;
    }

    m_ElapsedTime += deltaTime;

    const float fadeInDuration = 0.5f;
    const float fadeOutDuration = 0.5f;

    if (m_LoadingComplete && m_ElapsedTime >= m_Duration - fadeOutDuration) {
        float fadeOutProgress = (m_ElapsedTime - (m_Duration - fadeOutDuration)) / fadeOutDuration;
        m_FadeAlpha = 1.0f - fadeOutProgress;
        if (m_FadeAlpha < 0.0f) {
            m_FadeAlpha = 0.0f;
        }
    } else if (m_ElapsedTime < fadeInDuration) {
        m_FadeAlpha = m_ElapsedTime / fadeInDuration;
        if (m_FadeAlpha > 1.0f) {
            m_FadeAlpha = 1.0f;
        }
    } else {
        m_FadeAlpha = 1.0f;
    }
}

bool SplashScreen::IsFinished() const {
    if (!m_UseEmbedded) {
        return true;
    }
    return m_LoadingComplete && m_ElapsedTime >= m_Duration;
}

void SplashScreen::MarkLoadingComplete() {
    m_LoadingComplete = true;
}

} // namespace PiiXeL
