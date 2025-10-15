#ifndef MYFIRSTGAME_EXAMPLESCRIPT_HPP
#define MYFIRSTGAME_EXAMPLESCRIPT_HPP

#include "Components/Animator.hpp"
#include "Components/RigidBody2D.hpp"
#include "Components/Transform.hpp"
#include "Core/Logger.hpp"
#include "Reflection/Reflection.hpp"
#include "Scripting/AssetRef.hpp"
#include "Scripting/EntityRef.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Scripting/ScriptComponentHandles.inl"

#include <raylib.h>

class PlayerController : public PiiXeL::ScriptComponent {
public:
    PlayerController() = default;

    float moveSpeed{300.0f};
    float jumpForce{600.0f};

protected:
    void OnAwake() override { PX_LOG_INFO(GAME, "PlayerController: Awake!"); }

    void OnStart() override { PX_LOG_INFO(GAME, "PlayerController: Start!"); }

    void OnUpdate(float deltaTime) override {
        (void)deltaTime;

        auto rb = GetHandle<PiiXeL::RigidBody2D>();
        if (!rb)
            return;

        Vector2 vel = rb->GetVelocity();

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            vel.x = -moveSpeed;
        }
        else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            vel.x = moveSpeed;
        }
        else {
            vel.x = 0.0f;
        }

        rb->SetVelocity(vel);

        if (rb->IsGrounded() && (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE))) {
            rb->AddImpulse({0.0f, -jumpForce});
        }
    }

    void OnCollisionEnter(entt::entity other) override {
        (void)other;
        PX_LOG_INFO(GAME, "PlayerController: Collision Enter!");
    }

    void OnCollisionStay(entt::entity other) override { (void)other; }

    void OnCollisionExit(entt::entity other) override {
        (void)other;
        PX_LOG_INFO(GAME, "PlayerController: Collision Exit!");
    }

    void OnTriggerEnter(entt::entity other) override {
        (void)other;
        PX_LOG_INFO(GAME, "PlayerController: Trigger Enter - Collected item!");
    }

    void OnTriggerStay(entt::entity other) override { (void)other; }

    void OnTriggerExit(entt::entity other) override {
        (void)other;
        PX_LOG_INFO(GAME, "PlayerController: Trigger Exit!");
    }
};

BEGIN_REFLECT(PlayerController)
FIELD_RANGE(moveSpeed, 0.0f, 1000.0f, 10.0f)
FIELD_RANGE(jumpForce, 0.0f, 2000.0f, 10.0f)
END_REFLECT(PlayerController)

class RotatingPlatform : public PiiXeL::ScriptComponent {
public:
    RotatingPlatform() = default;

    float amplitude{100.0f};
    float frequency{1.0f};

protected:
    void OnUpdate(float deltaTime) override {
        m_Time += deltaTime;

        auto rb = GetHandle<PiiXeL::RigidBody2D>();
        if (rb) {
            float targetY = m_StartY + sinf(m_Time * frequency) * amplitude;
            float velocityY = amplitude * frequency * cosf(m_Time * frequency);

            rb->SetVelocity({0.0f, velocityY});
        }
        else {
            Vector2 pos = GetPosition();
            float offset = sinf(m_Time * frequency) * amplitude;
            SetPosition({pos.x, m_StartY + offset});
        }
    }

    void OnAwake() override {
        Vector2 startPos = GetPosition();
        m_StartX = startPos.x;
        m_StartY = startPos.y;
    }

private:
    float m_Time{0.0f};
    float m_StartX{0.0f};
    float m_StartY{0.0f};
};

BEGIN_REFLECT(RotatingPlatform)
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
        if (!target.IsValid() || !GetScene())
            return;

        entt::registry& registry = GetScene()->GetRegistry();
        if (!registry.valid(target.Get()))
            return;

        if (registry.all_of<PiiXeL::Transform>(target.Get())) {
            Vector2 targetPos = registry.get<PiiXeL::Transform>(target.Get()).position;
            Vector2 desiredPos = {targetPos.x + offset.x, targetPos.y + offset.y};

            Vector2 currentPos = GetPosition();
            Vector2 smoothedPos = {currentPos.x + (desiredPos.x - currentPos.x) * smoothSpeed * deltaTime,
                                   currentPos.y + (desiredPos.y - currentPos.y) * smoothSpeed * deltaTime};

            SetPosition(smoothedPos);
        }
    }
};

BEGIN_REFLECT(FollowCamera)
FIELD_ENTITY(target)
FIELD_RANGE(smoothSpeed, 0.0f, 20.0f, 0.1f)
FIELD(offset)
END_REFLECT(FollowCamera)

class AnimatedCharacter : public PiiXeL::ScriptComponent {
public:
    AnimatedCharacter() = default;

    float moveSpeed{200.0f};
    float jumpForce{500.0f};
    float groundCheckDistance{5.0f};

protected:
    void OnStart() override {
        m_Animator = GetHandle<PiiXeL::Animator>();
        m_RigidBody = GetHandle<PiiXeL::RigidBody2D>();

        if (m_Animator) {
            m_Animator->Play();
            PX_LOG_INFO(GAME, "AnimatedCharacter: Animator initialized");
        }
    }

    void OnUpdate(float deltaTime) override {
        (void)deltaTime;

        if (!m_RigidBody || !m_Animator)
            return;

        Vector2 velocity = m_RigidBody->GetVelocity();
        float horizontalInput = 0.0f;
        bool isGrounded = m_RigidBody->IsGrounded(groundCheckDistance);

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
            horizontalInput = -1.0f;
        }
        else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
            horizontalInput = 1.0f;
        }

        velocity.x = horizontalInput * moveSpeed;
        m_RigidBody->SetVelocity(velocity);

        if (isGrounded && (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE))) {
            m_RigidBody->AddImpulse({0.0f, -jumpForce});
            m_Animator->SetTrigger("Jump");
        }

        float absHorizontalVelocity = fabsf(velocity.x);
        float absVerticalVelocity = fabsf(velocity.y);

        m_Animator->SetFloat("Speed", absHorizontalVelocity);
        m_Animator->SetFloat("VelocityY", velocity.y);
        m_Animator->SetBool("IsGrounded", isGrounded);
        m_Animator->SetBool("IsMoving", absHorizontalVelocity > 10.0f);
        m_Animator->SetBool("IsFalling", !isGrounded && velocity.y > 0.0f);

        if (IsKeyPressed(KEY_E)) {
            m_Animator->SetTrigger("Attack");
        }

        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            m_Animator->SetSpeed(2.0f);
        }
        else if (IsKeyReleased(KEY_LEFT_SHIFT)) {
            m_Animator->SetSpeed(1.0f);
        }
    }

private:
    std::optional<PiiXeL::AnimatorHandle> m_Animator;
    std::optional<PiiXeL::RigidBodyHandle> m_RigidBody;
};

BEGIN_REFLECT(AnimatedCharacter)
FIELD_RANGE(moveSpeed, 0.0f, 1000.0f, 10.0f)
FIELD_RANGE(jumpForce, 0.0f, 2000.0f, 10.0f)
FIELD_RANGE(groundCheckDistance, 0.0f, 50.0f, 1.0f)
END_REFLECT(AnimatedCharacter)

#endif
