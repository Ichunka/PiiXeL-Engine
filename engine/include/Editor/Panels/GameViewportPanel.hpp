#ifndef PIIXELENGINE_GAMEVIEWPORTPANEL_HPP
#define PIIXELENGINE_GAMEVIEWPORTPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include <entt/entt.hpp>

#include <functional>
#include <raylib.h>

#include "EditorPanel.hpp"

namespace PiiXeL {

class Engine;

enum class EditorState;

class GameViewportPanel : public EditorPanel {
public:
    GameViewportPanel(Engine* engine, RenderTexture2D* gameViewportTexture, EditorState* editorState);

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Game"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

    void SetGetPrimaryCameraCallback(std::function<entt::entity()> callback);

private:
    Engine* m_Engine;
    RenderTexture2D* m_GameViewportTexture;
    EditorState* m_EditorState;

    bool m_IsOpen{true};

    std::function<entt::entity()> m_GetPrimaryCameraCallback;
};

} // namespace PiiXeL

#endif

#endif
