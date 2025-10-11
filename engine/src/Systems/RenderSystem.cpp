#include "Systems/RenderSystem.hpp"
#include "Components/Transform.hpp"
#include "Components/Sprite.hpp"
#include "Components/Tag.hpp"
#include "Components/BoxCollider2D.hpp"
#include "Debug/DebugDraw.hpp"
#include <algorithm>
#include <vector>
#include <cmath>

namespace PiiXeL {

RenderSystem::RenderSystem()
#ifdef BUILD_WITH_EDITOR
    : m_ShowDebug{false}
    , m_ShowColliders{false}
#else
    : m_ShowDebug{false}
    , m_ShowColliders{false}
#endif
{
}

RenderSystem::~RenderSystem() = default;

void RenderSystem::Render(entt::registry& registry) {
    RenderSprites(registry);

    if (m_ShowColliders) {
        RenderColliders(registry);
    }

    if (m_ShowDebug) {
        RenderDebug(registry);
    }

    DebugDraw::Instance().Render();
    DebugDraw::Instance().Clear();
}

void RenderSystem::RenderWithCamera(entt::registry& registry, const Camera2D& camera) {
    BeginMode2D(camera);

    RenderSprites(registry);

    if (m_ShowColliders) {
        RenderColliders(registry);
    }

    if (m_ShowDebug) {
        RenderDebug(registry);
    }

    DebugDraw::Instance().Render();

    EndMode2D();

    DebugDraw::Instance().Clear();
}

void RenderSystem::RenderSprites(entt::registry& registry) {
    struct SpriteEntity {
        entt::entity entity;
        const Sprite* sprite;
        const Transform* transform;
    };

    std::vector<SpriteEntity> sprites;

    registry.view<Sprite, Transform>().each([&](entt::entity entity, const Sprite& sprite, const Transform& transform) {
        if (sprite.IsValid()) {
            sprites.push_back({entity, &sprite, &transform});
        }
    });

    std::sort(sprites.begin(), sprites.end(), [](const SpriteEntity& a, const SpriteEntity& b) {
        return a.sprite->layer < b.sprite->layer;
    });

    for (const SpriteEntity& spriteEntity : sprites) {
        const Sprite* sprite{spriteEntity.sprite};
        const Transform* transform{spriteEntity.transform};

        Vector2 originPixels{
            sprite->sourceRect.width * sprite->origin.x * transform->scale.x,
            sprite->sourceRect.height * sprite->origin.y * transform->scale.y
        };

        Rectangle destRect{
            transform->position.x,
            transform->position.y,
            sprite->sourceRect.width * transform->scale.x,
            sprite->sourceRect.height * transform->scale.y
        };

        DrawTexturePro(
            sprite->texture,
            sprite->sourceRect,
            destRect,
            originPixels,
            transform->rotation,
            sprite->tint
        );
    }
}

void RenderSystem::RenderDebug(entt::registry& registry) {
    registry.view<Transform>().each([](const Transform& transform) {
        Rectangle rect{
            transform.position.x,
            transform.position.y,
            transform.scale.x,
            transform.scale.y
        };

        Vector2 origin{transform.scale.x * 0.5f, transform.scale.y * 0.5f};
        DrawRectanglePro(rect, origin, transform.rotation, Color{100, 200, 100, 150});

        Vector2 corners[4];
        float halfW = transform.scale.x * 0.5f;
        float halfH = transform.scale.y * 0.5f;
        float cosR = std::cos(transform.rotation * DEG2RAD);
        float sinR = std::sin(transform.rotation * DEG2RAD);

        corners[0] = Vector2{
            transform.position.x + (-halfW * cosR - (-halfH) * sinR),
            transform.position.y + (-halfW * sinR + (-halfH) * cosR)
        };
        corners[1] = Vector2{
            transform.position.x + (halfW * cosR - (-halfH) * sinR),
            transform.position.y + (halfW * sinR + (-halfH) * cosR)
        };
        corners[2] = Vector2{
            transform.position.x + (halfW * cosR - halfH * sinR),
            transform.position.y + (halfW * sinR + halfH * cosR)
        };
        corners[3] = Vector2{
            transform.position.x + (-halfW * cosR - halfH * sinR),
            transform.position.y + (-halfW * sinR + halfH * cosR)
        };

        DrawLineV(corners[0], corners[1], Color{100, 255, 100, 255});
        DrawLineV(corners[1], corners[2], Color{100, 255, 100, 255});
        DrawLineV(corners[2], corners[3], Color{100, 255, 100, 255});
        DrawLineV(corners[3], corners[0], Color{100, 255, 100, 255});

        Vector2 right{transform.GetRight()};
        Vector2 endX{
            transform.position.x + right.x * 50.0f,
            transform.position.y + right.y * 50.0f
        };
        DrawLineV(transform.position, endX, RED);

        Vector2 up{transform.GetUp()};
        Vector2 endY{
            transform.position.x + up.x * 50.0f,
            transform.position.y + up.y * 50.0f
        };
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

        Vector2 centerPos{
            transform.position.x + collider.offset.x * transform.scale.x,
            transform.position.y + collider.offset.y * transform.scale.y
        };

        Vector2 corners[4];
        corners[0] = Vector2{
            centerPos.x + (-halfW * cosR - (-halfH) * sinR),
            centerPos.y + (-halfW * sinR + (-halfH) * cosR)
        };
        corners[1] = Vector2{
            centerPos.x + (halfW * cosR - (-halfH) * sinR),
            centerPos.y + (halfW * sinR + (-halfH) * cosR)
        };
        corners[2] = Vector2{
            centerPos.x + (halfW * cosR - halfH * sinR),
            centerPos.y + (halfW * sinR + halfH * cosR)
        };
        corners[3] = Vector2{
            centerPos.x + (-halfW * cosR - halfH * sinR),
            centerPos.y + (-halfW * sinR + halfH * cosR)
        };

        Color colliderColor = collider.isTrigger ? Color{0, 255, 255, 180} : Color{0, 255, 0, 180};

        DrawLineV(corners[0], corners[1], colliderColor);
        DrawLineV(corners[1], corners[2], colliderColor);
        DrawLineV(corners[2], corners[3], colliderColor);
        DrawLineV(corners[3], corners[0], colliderColor);
    });
}

} // namespace PiiXeL
