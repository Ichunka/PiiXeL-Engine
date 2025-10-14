#include "Systems/RenderSystem.hpp"

#include "Components/BoxCollider2D.hpp"
#include "Components/CircleCollider2D.hpp"
#include "Components/Sprite.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Core/Logger.hpp"
#include "Debug/DebugDraw.hpp"
#include "Debug/Profiler.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace PiiXeL {

RenderSystem::RenderSystem()
#ifdef BUILD_WITH_EDITOR
    :
    m_ShowDebug{false},
    m_ShowColliders{false}, m_DefaultWhiteTexture{}
#else
    :
    m_ShowDebug{false},
    m_ShowColliders{false}, m_DefaultWhiteTexture{}
#endif
{
    Image whiteImage = GenImageColor(64, 64, WHITE);
    m_DefaultWhiteTexture = LoadTextureFromImage(whiteImage);
    UnloadImage(whiteImage);

    if (m_DefaultWhiteTexture.id != 0)
    {
        SetTextureWrap(m_DefaultWhiteTexture, TEXTURE_WRAP_CLAMP);
        PX_LOG_INFO(RENDER, "RenderSystem: Default white texture created (64x64)");
    }
    else
    { PX_LOG_ERROR(RENDER, "RenderSystem: Failed to create default white texture"); }
}

RenderSystem::~RenderSystem() {
    if (m_DefaultWhiteTexture.id != 0)
    { UnloadTexture(m_DefaultWhiteTexture); }
}

void RenderSystem::Render(entt::registry& registry) {
    RenderSprites(registry);

    if (m_ShowColliders)
    { RenderColliders(registry); }

    if (m_ShowDebug)
    { RenderDebug(registry); }

    DebugDraw::Instance().Render();
    DebugDraw::Instance().Clear();
}

void RenderSystem::RenderWithCamera(entt::registry& registry, const Camera2D& camera) {
    BeginMode2D(camera);

    RenderSprites(registry);

    if (m_ShowColliders)
    { RenderColliders(registry); }

    if (m_ShowDebug)
    { RenderDebug(registry); }

    DebugDraw::Instance().Render();

    EndMode2D();

    DebugDraw::Instance().Clear();
}

void RenderSystem::RenderSprites(entt::registry& registry) {
    PROFILE_FUNCTION();

    struct SpriteEntity {
        entt::entity entity;
        const Sprite* sprite;
        const Transform* transform;
    };

    std::vector<SpriteEntity> sprites;

    {
        PROFILE_SCOPE("RenderSprites::Collect");
        registry.view<Sprite, Transform>().each(
            [&](entt::entity entity, const Sprite& sprite, const Transform& transform) {
                sprites.push_back({entity, &sprite, &transform});
            });
    }

    {
        PROFILE_SCOPE("RenderSprites::Sort");
        std::sort(sprites.begin(), sprites.end(),
                  [](const SpriteEntity& a, const SpriteEntity& b) { return a.sprite->layer < b.sprite->layer; });
    }

    {
        PROFILE_SCOPE("RenderSprites::Draw");
        for (const SpriteEntity& spriteEntity : sprites)
        {
            const Sprite* sprite{spriteEntity.sprite};
            const Transform* transform{spriteEntity.transform};

            Texture2D texture = sprite->GetTexture();
            Rectangle sourceRect = sprite->sourceRect;

            if (texture.id == 0)
            {
                texture = m_DefaultWhiteTexture;
                sourceRect = {0.0f, 0.0f, 64.0f, 64.0f};
            }

            Vector2 originPixels{sourceRect.width * sprite->origin.x * transform->scale.x,
                                 sourceRect.height * sprite->origin.y * transform->scale.y};

            Rectangle destRect{transform->position.x, transform->position.y, sourceRect.width * transform->scale.x,
                               sourceRect.height * transform->scale.y};

            DrawTexturePro(texture, sourceRect, destRect, originPixels, transform->rotation, sprite->tint);
        }
    }
}

void RenderSystem::RenderDebug(entt::registry& registry) {
    registry.view<Transform>().each([](const Transform& transform) {
        Rectangle rect{transform.position.x, transform.position.y, transform.scale.x, transform.scale.y};

        Vector2 origin{transform.scale.x * 0.5f, transform.scale.y * 0.5f};
        DrawRectanglePro(rect, origin, transform.rotation, Color{100, 200, 100, 150});

        Vector2 corners[4];
        float halfW = transform.scale.x * 0.5f;
        float halfH = transform.scale.y * 0.5f;
        float cosR = std::cos(transform.rotation * DEG2RAD);
        float sinR = std::sin(transform.rotation * DEG2RAD);

        corners[0] = Vector2{transform.position.x + (-halfW * cosR - (-halfH) * sinR),
                             transform.position.y + (-halfW * sinR + (-halfH) * cosR)};
        corners[1] = Vector2{transform.position.x + (halfW * cosR - (-halfH) * sinR),
                             transform.position.y + (halfW * sinR + (-halfH) * cosR)};
        corners[2] = Vector2{transform.position.x + (halfW * cosR - halfH * sinR),
                             transform.position.y + (halfW * sinR + halfH * cosR)};
        corners[3] = Vector2{transform.position.x + (-halfW * cosR - halfH * sinR),
                             transform.position.y + (-halfW * sinR + halfH * cosR)};

        DrawLineV(corners[0], corners[1], Color{100, 255, 100, 255});
        DrawLineV(corners[1], corners[2], Color{100, 255, 100, 255});
        DrawLineV(corners[2], corners[3], Color{100, 255, 100, 255});
        DrawLineV(corners[3], corners[0], Color{100, 255, 100, 255});

        Vector2 right{transform.GetRight()};
        Vector2 endX{transform.position.x + right.x * 50.0f, transform.position.y + right.y * 50.0f};
        DrawLineV(transform.position, endX, RED);

        Vector2 up{transform.GetUp()};
        Vector2 endY{transform.position.x + up.x * 50.0f, transform.position.y + up.y * 50.0f};
        DrawLineV(transform.position, endY, GREEN);

        DrawCircleV(transform.position, 5.0f, YELLOW);
    });
}

void RenderSystem::RenderColliders(entt::registry& registry) {
    registry.view<Transform, BoxCollider2D>().each([](const Transform& transform, const BoxCollider2D& collider) {
        float scaledWidth = collider.size.x * transform.scale.x;
        float scaledHeight = collider.size.y * transform.scale.y;
        float halfW = scaledWidth * 0.5f;
        float halfH = scaledHeight * 0.5f;
        float cosR = std::cos(transform.rotation * DEG2RAD);
        float sinR = std::sin(transform.rotation * DEG2RAD);

        Vector2 centerPos{transform.position.x + collider.offset.x * transform.scale.x,
                          transform.position.y + collider.offset.y * transform.scale.y};

        Vector2 corners[4];
        corners[0] =
            Vector2{centerPos.x + (-halfW * cosR - (-halfH) * sinR), centerPos.y + (-halfW * sinR + (-halfH) * cosR)};
        corners[1] =
            Vector2{centerPos.x + (halfW * cosR - (-halfH) * sinR), centerPos.y + (halfW * sinR + (-halfH) * cosR)};
        corners[2] = Vector2{centerPos.x + (halfW * cosR - halfH * sinR), centerPos.y + (halfW * sinR + halfH * cosR)};
        corners[3] =
            Vector2{centerPos.x + (-halfW * cosR - halfH * sinR), centerPos.y + (-halfW * sinR + halfH * cosR)};

        Color colliderColor = collider.isTrigger ? Color{0, 255, 255, 180} : Color{0, 255, 0, 180};

        DrawLineV(corners[0], corners[1], colliderColor);
        DrawLineV(corners[1], corners[2], colliderColor);
        DrawLineV(corners[2], corners[3], colliderColor);
        DrawLineV(corners[3], corners[0], colliderColor);
    });
    registry.view<Transform, CircleCollider2D>().each([](const Transform& transform, const CircleCollider2D& collider) {
        float scaledRadius = collider.radius * (transform.scale.x + transform.scale.y) * 0.5f;

        Vector2 centerPos{transform.position.x + collider.offset.x * transform.scale.x,
                          transform.position.y + collider.offset.y * transform.scale.y};

        Color colliderColor = collider.isTrigger ? Color{0, 255, 255, 180} : Color{0, 255, 0, 180};

        DrawCircleLines(static_cast<int>(centerPos.x), static_cast<int>(centerPos.y), scaledRadius, colliderColor);
    });
}

} // namespace PiiXeL
