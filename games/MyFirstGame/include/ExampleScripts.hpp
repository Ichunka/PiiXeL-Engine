#ifndef MYFIRSTGAME_EXAMPLESCRIPT_HPP
#define MYFIRSTGAME_EXAMPLESCRIPT_HPP

#include "Scripting/ScriptComponent.hpp"
#include "Scripting/ScriptComponentHandles.inl"
#include "Scripting/AssetRef.hpp"
#include "Scripting/EntityRef.hpp"
#include "Components/Transform.hpp"
#include "Components/RigidBody2D.hpp"
#include "Systems/ScriptSystem.hpp"
#include "Reflection/Reflection.hpp"
#include <raylib.h>

class PlayerController : public PiiXeL::ScriptComponent {
public:
    PlayerController() = default;

    float moveSpeed{300.0f};
    float jumpForce{600.0f};
    PiiXeL::AssetRef<Texture2D> playerTexture;

protected:
    void OnAwake() override {
        TraceLog(LOG_INFO, "PlayerController: Awake!");
    }

    void OnUpdate(float deltaTime) override {
        (void)deltaTime;

        auto rb = GetHandle<PiiXeL::RigidBody2D>();
        if (!rb) return;

        Vector2 vel = rb->GetVelocity();

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            vel.x = -moveSpeed;
        } else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            vel.x = moveSpeed;
        } else {
            vel.x = 0.0f;
        }

        rb->SetVelocity(vel);

        if (rb->IsGrounded() && (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE))) {
            rb->AddImpulse({0.0f, -jumpForce});
        }
    }

    void OnTriggerEnter(entt::entity other) override {
        (void)other;
        TraceLog(LOG_INFO, "PlayerController: Trigger Enter - Collected item!");
    }
};

BEGIN_REFLECT(PlayerController)
    FIELD_RANGE(moveSpeed, 0.0f, 1000.0f, 10.0f)
    FIELD_RANGE(jumpForce, 0.0f, 2000.0f, 10.0f)
    FIELD(playerTexture)
END_REFLECT(PlayerController)

class RotatingPlatform : public PiiXeL::ScriptComponent {
public:
    RotatingPlatform() = default;

    float rotationSpeed{90.0f};
    float amplitude{100.0f};
    float frequency{1.0f};

protected:
    void OnUpdate(float deltaTime) override {
        m_Time += deltaTime;

        Vector2 pos = GetPosition();
        float offset = sinf(m_Time * frequency) * amplitude;
        SetPosition({pos.x, m_StartY + offset});
    }

    void OnAwake() override {
        m_StartY = GetPosition().y;
    }

private:
    float m_Time{0.0f};
    float m_StartY{0.0f};
};

BEGIN_REFLECT(RotatingPlatform)
    FIELD_RANGE(rotationSpeed, 0.0f, 360.0f, 5.0f)
    FIELD_RANGE(amplitude, 0.0f, 500.0f, 10.0f)
    FIELD_RANGE(frequency, 0.0f, 10.0f, 0.1f)
END_REFLECT(RotatingPlatform)

class FollowCamera : public PiiXeL::ScriptComponent {
public:
    FollowCamera() = default;

    PiiXeL::EntityRef target;
    float smoothSpeed{5.0f};
    Vector2 offset{0.0f, 0.0f};

protected:
    void OnUpdate(float deltaTime) override {
        if (!target.IsValid() || !GetScene()) return;

        entt::registry& registry = GetScene()->GetRegistry();
        if (!registry.valid(target.Get())) return;

        if (registry.all_of<PiiXeL::Transform>(target.Get())) {
            Vector2 targetPos = registry.get<PiiXeL::Transform>(target.Get()).position;
            Vector2 desiredPos = {targetPos.x + offset.x, targetPos.y + offset.y};

            Vector2 currentPos = GetPosition();
            Vector2 smoothedPos = {
                currentPos.x + (desiredPos.x - currentPos.x) * smoothSpeed * deltaTime,
                currentPos.y + (desiredPos.y - currentPos.y) * smoothSpeed * deltaTime
            };

            SetPosition(smoothedPos);
        }
    }
};

BEGIN_REFLECT(FollowCamera)
    FIELD_ENTITY(target)
    FIELD_RANGE(smoothSpeed, 0.0f, 20.0f, 0.1f)
    FIELD(offset)
END_REFLECT(FollowCamera)

REGISTER_SCRIPT(PlayerController, "PlayerController")
REGISTER_SCRIPT(RotatingPlatform, "RotatingPlatform")
REGISTER_SCRIPT(FollowCamera, "FollowCamera")

#endif
