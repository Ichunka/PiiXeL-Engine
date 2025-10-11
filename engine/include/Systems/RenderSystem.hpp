#ifndef PIIXELENGINE_RENDERSYSTEM_HPP
#define PIIXELENGINE_RENDERSYSTEM_HPP

#include <entt/entt.hpp>
#include <raylib.h>

namespace PiiXeL {

class RenderSystem {
public:
    RenderSystem();
    ~RenderSystem();

    void Render(entt::registry& registry);
    void RenderWithCamera(entt::registry& registry, const Camera2D& camera);

    void SetShowDebug(bool show) { m_ShowDebug = show; }
    void SetShowColliders(bool show) { m_ShowColliders = show; }

    [[nodiscard]] bool GetShowDebug() const { return m_ShowDebug; }
    [[nodiscard]] bool GetShowColliders() const { return m_ShowColliders; }

private:
    void RenderSprites(entt::registry& registry);
    void RenderDebug(entt::registry& registry);
    void RenderColliders(entt::registry& registry);

private:
    bool m_ShowDebug{false};
    bool m_ShowColliders{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_RENDERSYSTEM_HPP
