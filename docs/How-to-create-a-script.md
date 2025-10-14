# How to Create a Script

## Quick Start

### 1. Create Script Header

Create a header file in `games/YourGame/include/`.

```cpp
#pragma once
#include "Scripting/ScriptComponent.hpp"
#include "Physics/ComponentHandles.hpp"

class MyScript : public PiiXeL::ScriptComponent {
public:
    MyScript() = default;

    float speed{100.0f};

private:
    std::optional<PiiXeL::RigidBodyHandle> m_RigidBody;
    std::optional<PiiXeL::AnimatorHandle> m_Animator;

    void OnStart() override {
        m_RigidBody = GetHandle<PiiXeL::RigidBody2D>();
        m_Animator = GetHandle<PiiXeL::Animator>();
    }

    void OnUpdate(float deltaTime) override {
        if (!m_RigidBody) return;

        Vector2 velocity{0.0f, 0.0f};
        if (IsKeyDown(KEY_RIGHT)) velocity.x = speed;
        if (IsKeyDown(KEY_LEFT)) velocity.x = -speed;

        m_RigidBody->SetVelocity(velocity);

        if (m_Animator) {
            m_Animator->SetFloat("Speed", std::abs(velocity.x));
        }
    }

    void OnDestroy() override {}
};
```

### 2. Register Script

Add to `games/YourGame/include/ExampleScripts.hpp`:

```cpp
#include "MyScript.hpp"

BEGIN_REFLECT(MyScript)
    FIELD_RANGE(speed, 0.0f, 500.0f, 10.0f)
END_REFLECT(MyScript)
```

## Lifecycle Methods

```cpp
void OnAwake()                         // Called when script is initialized
void OnStart()                         // Called on first frame
void OnUpdate(float deltaTime)         // Called every frame
void OnFixedUpdate(float fixedDelta)   // Called at fixed timestep
void OnDestroy()                       // Called when entity is destroyed

// Box2D collision callbacks
void OnCollisionEnter(entt::entity other)
void OnCollisionStay(entt::entity other)
void OnCollisionExit(entt::entity other)

// Trigger callbacks
void OnTriggerEnter(entt::entity other)
void OnTriggerStay(entt::entity other)
void OnTriggerExit(entt::entity other)
```

## Component Handles

### RigidBodyHandle
```cpp
m_RigidBody->SetVelocity(Vector2{100.0f, 0.0f});
m_RigidBody->AddForce(Vector2{0.0f, -500.0f});
m_RigidBody->AddImpulse(Vector2{0.0f, -300.0f});
Vector2 vel = m_RigidBody->GetVelocity();
bool grounded = m_RigidBody->IsGrounded();
```

### AnimatorHandle
```cpp
m_Animator->SetFloat("Speed", 5.0f);
m_Animator->SetBool("IsGrounded", true);
m_Animator->SetTrigger("Jump");
m_Animator->SetSpeed(2.0f);
std::string state = m_Animator->GetCurrentState();
```

## Exposed Properties (Inspector)

Public fields are automatically exposed in the inspector when reflected:

```cpp
// Simple types
float speed{100.0f};
int health{100};
bool enabled{true};
Vector2 offset{0.0f, 0.0f};
```

## Reflection Macros

```cpp
BEGIN_REFLECT(MyScript)
    FIELD(speed)                                    // Basic field
    FIELD_RANGE(health, 0.0f, 100.0f, 1.0f)        // Field with range
    FIELD_ENTITY(target)                            // Entity reference
    FIELD(offset)                                   // Vector2
END_REFLECT(MyScript)
```

## Example: Player Controller

```cpp
class PlayerController : public PiiXeL::ScriptComponent {
public:
    PlayerController() = default;

    float moveSpeed{200.0f};
    float jumpForce{500.0f};

private:
    std::optional<PiiXeL::RigidBodyHandle> m_RigidBody;

    void OnStart() override {
        m_RigidBody = GetHandle<PiiXeL::RigidBody2D>();
    }

    void OnUpdate(float deltaTime) override {
        if (!m_RigidBody) return;

        Vector2 velocity = m_RigidBody->GetVelocity();
        velocity.x = 0.0f;

        if (IsKeyDown(KEY_RIGHT)) velocity.x = moveSpeed;
        if (IsKeyDown(KEY_LEFT)) velocity.x = -moveSpeed;

        if (IsKeyPressed(KEY_SPACE) && m_RigidBody->IsGrounded()) {
            m_RigidBody->AddImpulse(Vector2{0.0f, -jumpForce});
        }

        m_RigidBody->SetVelocity(velocity);
    }
};

BEGIN_REFLECT(PlayerController)
    FIELD_RANGE(moveSpeed, 0.0f, 1000.0f, 10.0f)
    FIELD_RANGE(jumpForce, 0.0f, 2000.0f, 10.0f)
END_REFLECT(PlayerController)
```
