#include "Core/SplashScreen.hpp"
#include <cmath>
#include <algorithm>

namespace PiiXeL {

SplashScreen::SplashScreen()
    : m_SplashTexture{0}
{
}

SplashScreen::~SplashScreen() {
    if (m_IsShowing && m_SplashTexture.id != 0) {
        UnloadTexture(m_SplashTexture);
    }
}

void SplashScreen::Show(const std::string& imagePath, float minDurationSeconds) {
    m_SplashTexture = LoadTexture(imagePath.c_str());

    if (m_SplashTexture.id == 0) {
        TraceLog(LOG_WARNING, ("Failed to load splash screen: " + imagePath).c_str());
        m_Finished = true;
        return;
    }

    m_MinDuration = minDurationSeconds;
    m_ElapsedTime = 0.0f;
    m_FadeAlpha = 0.0f;
    m_IsShowing = true;
    m_LoadingComplete = false;
    m_Finished = false;
    m_FadingOut = false;

    TraceLog(LOG_INFO, "Splash screen initialized");
}

void SplashScreen::Update(float deltaTime) {
    if (m_Finished || !m_IsShowing) {
        return;
    }

    m_ElapsedTime += deltaTime;

    if (!m_FadingOut) {
        if (m_ElapsedTime < FADE_IN_DURATION) {
            m_FadeAlpha = m_ElapsedTime / FADE_IN_DURATION;
        } else {
            m_FadeAlpha = 1.0f;
        }

        bool timerExpired = m_ElapsedTime >= m_MinDuration;
        if (m_LoadingComplete && timerExpired) {
            m_FadingOut = true;
            m_ElapsedTime = 0.0f;
        }
    } else {
        m_FadeAlpha = 1.0f - (m_ElapsedTime / FADE_OUT_DURATION);

        if (m_ElapsedTime >= FADE_OUT_DURATION) {
            m_Finished = true;
            m_IsShowing = false;

            if (m_SplashTexture.id != 0) {
                UnloadTexture(m_SplashTexture);
                m_SplashTexture = {0};
            }
        }
    }
}

void SplashScreen::Render() {
    if (m_Finished || !m_IsShowing || m_SplashTexture.id == 0) {
        return;
    }

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    float targetSize = std::min(screenWidth, screenHeight) * 0.5f;
    float scale = targetSize / static_cast<float>(m_SplashTexture.width);

    float scaledWidth = static_cast<float>(m_SplashTexture.width) * scale;
    float scaledHeight = static_cast<float>(m_SplashTexture.height) * scale;

    float posX = (static_cast<float>(screenWidth) - scaledWidth) * 0.5f;
    float posY = (static_cast<float>(screenHeight) - scaledHeight) * 0.5f;

    Rectangle sourceRec{0.0f, 0.0f, static_cast<float>(m_SplashTexture.width), static_cast<float>(m_SplashTexture.height)};
    Rectangle destRec{posX, posY, scaledWidth, scaledHeight};
    Vector2 origin{0.0f, 0.0f};

    unsigned char alpha = static_cast<unsigned char>(m_FadeAlpha * 255.0f);
    Color tint{255, 255, 255, alpha};

    DrawTexturePro(m_SplashTexture, sourceRec, destRec, origin, 0.0f, tint);

    if (!m_FadingOut) {
        float spinnerRadius = 30.0f;
        float spinnerX = static_cast<float>(screenWidth) * 0.5f;
        float spinnerY = posY + scaledHeight + 80.0f;

        float rotationSpeed = 2.0f;
        float rotation = m_ElapsedTime * rotationSpeed;

        int dotCount = 8;
        for (int i = 0; i < dotCount; ++i) {
            float angle = (static_cast<float>(i) / static_cast<float>(dotCount)) * 2.0f * PI + rotation;
            float dotX = spinnerX + std::cos(angle) * spinnerRadius;
            float dotY = spinnerY + std::sin(angle) * spinnerRadius;

            float dotAlpha = (1.0f - (static_cast<float>(i) / static_cast<float>(dotCount))) * 0.8f + 0.2f;
            unsigned char dotAlphaInt = static_cast<unsigned char>(dotAlpha * m_FadeAlpha * 255.0f);

            DrawCircle(static_cast<int>(dotX), static_cast<int>(dotY), 4.0f, Color{200, 200, 200, dotAlphaInt});
        }
    }
}

} // namespace PiiXeL
