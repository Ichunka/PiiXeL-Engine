#ifndef PIIXELENGINE_SCENEVIEWPORTPANEL_HPP
#define PIIXELENGINE_SCENEVIEWPORTPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "EditorPanel.hpp"
#include <raylib.h>
#include <imgui.h>
#include <functional>

namespace PiiXeL {

class Engine;

class SceneViewportPanel : public EditorPanel {
public:
    SceneViewportPanel(
        Engine* engine,
        RenderTexture2D* viewportTexture,
        Rectangle* viewportBounds,
        bool* viewportHovered,
        bool* viewportFocused,
        Vector2* cameraPosition,
        float* cameraZoom,
        Vector2* lastMousePos,
        bool* isPanning,
        ImVec2* viewportPos,
        ImVec2* viewportSize
    );

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Scene"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

    void SetHandleGizmoInteractionCallback(std::function<void()> callback);
    void SetHandleEntitySelectionCallback(std::function<void()> callback);
    void SetRenderGizmosCallback(std::function<void()> callback);

private:
    Engine* m_Engine;
    RenderTexture2D* m_ViewportTexture;
    Rectangle* m_ViewportBounds;
    bool* m_ViewportHovered;
    bool* m_ViewportFocused;
    Vector2* m_CameraPosition;
    float* m_CameraZoom;
    Vector2* m_LastMousePos;
    bool* m_IsPanning;
    ImVec2* m_ViewportPos;
    ImVec2* m_ViewportSize;

    bool m_IsOpen{true};

    std::function<void()> m_HandleGizmoInteractionCallback;
    std::function<void()> m_HandleEntitySelectionCallback;
    std::function<void()> m_RenderGizmosCallback;
};

}

#endif

#endif
