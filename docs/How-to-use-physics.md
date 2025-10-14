# How to Use Physics (Box2D)

## Quick Setup

1. Add **RigidBody2D** component
2. Add **Collider** component (Box or Circle)
3. Configure properties

## RigidBody2D Types

```
Static      → Never moves (walls, platforms)
Dynamic     → Moves with physics (player, enemies, props)
Kinematic   → Moves without physics simulation (moving platforms)
```

## Component Properties

### RigidBody2D
```
Type:           Static / Dynamic / Kinematic
Mass:           10.0 (kg)
Friction:       0.3 (0.0 - 1.0)
Restitution:    0.0 (bounciness, 0.0 - 1.0)
Fixed Rotation: true/false (prevent rotation)
```

### BoxCollider2D
```
Size:       Vector2{64.0f, 64.0f}
Offset:     Vector2{0.0f, 0.0f}
Is Trigger: false (if true, no collision response)
```

### CircleCollider2D
```
Radius:     32.0f
Offset:     Vector2{0.0f, 0.0f}
Is Trigger: false
```

## Script Control

```cpp
class PhysicsObject : public PiiXeL::ScriptComponent {
private:
    std::optional<PiiXeL::RigidBodyHandle> m_RigidBody;

    void OnStart() override {
        m_RigidBody = GetHandle<PiiXeL::RigidBody2D>();
    }

    void OnUpdate(float deltaTime) override {
        if (!m_RigidBody) return;

        // Set velocity directly
        m_RigidBody->SetVelocity(Vector2{100.0f, 0.0f});

        // Apply continuous force (acceleration)
        m_RigidBody->AddForce(Vector2{500.0f, 0.0f});

        // Apply instant impulse (jump)
        if (IsKeyPressed(KEY_SPACE)) {
            m_RigidBody->AddImpulse(Vector2{0.0f, -500.0f});
        }

        // Read current velocity
        Vector2 velocity = m_RigidBody->GetVelocity();

        // Check if grounded
        bool isGrounded = m_RigidBody->IsGrounded();
    }
};
```

## RigidBodyHandle API

```cpp
// Velocity
void SetVelocity(Vector2 velocity);
Vector2 GetVelocity() const;

// Forces
void AddForce(Vector2 force);              // Continuous acceleration
void AddImpulse(Vector2 impulse);          // Instant velocity change

// Angular
void SetAngularVelocity(float velocity);
float GetAngularVelocity() const;

// Kinematic movement
void SetKinematicTarget(Vector2 target);
void MoveKinematic(Vector2 translation);

// Queries
bool IsGrounded(float checkDistance = 5.0f) const;
```

## Collision Detection

```cpp
class CollisionHandler : public PiiXeL::ScriptComponent {
    void OnCollisionEnter(entt::entity other) override {
        PX_LOG_INFO(GAME, "Collided with entity!");
    }

    void OnCollisionStay(entt::entity other) override {
        // Called while collision persists
    }

    void OnCollisionExit(entt::entity other) override {
        PX_LOG_INFO(GAME, "Collision ended!");
    }
};
```

## Triggers (No Collision Response)

Set `Is Trigger = true` on collider:

```cpp
class TriggerZone : public PiiXeL::ScriptComponent {
    void OnTriggerEnter(entt::entity other) override {
        PX_LOG_INFO(GAME, "Entity entered trigger zone!");
    }

    void OnTriggerStay(entt::entity other) override {
        // Called while entity stays in trigger
    }

    void OnTriggerExit(entt::entity other) override {
        PX_LOG_INFO(GAME, "Entity exited trigger zone!");
    }
};
```

## Common Patterns

### Platform Movement
```cpp
// Kinematic body moves without physics
m_RigidBody->SetVelocity(Vector2{100.0f, 0.0f});
```

### Jump
```cpp
if (m_RigidBody->IsGrounded() && IsKeyPressed(KEY_SPACE)) {
    m_RigidBody->AddImpulse(Vector2{0.0f, -jumpForce});
}
```

### Character Controller
```cpp
void OnUpdate(float deltaTime) override {
    Vector2 velocity = m_RigidBody->GetVelocity();

    // Horizontal movement (keep vertical velocity)
    velocity.x = 0.0f;
    if (IsKeyDown(KEY_RIGHT)) velocity.x = moveSpeed;
    if (IsKeyDown(KEY_LEFT)) velocity.x = -moveSpeed;

    m_RigidBody->SetVelocity(velocity);
}
```

### Ground Check
```cpp
bool IsGrounded() {
    return m_RigidBody && m_RigidBody->IsGrounded();
}
```

## Performance Tips

- Use **Static** bodies for non-moving objects (huge performance gain)
- Keep **mass** reasonable (0.1 - 100.0)
- Avoid very small or very large objects (use realistic sizes)
- Use **Fixed Rotation** for characters (prevents tumbling)
- Prefer **AddImpulse** over **SetVelocity** for physics-driven movement

## Debug Visualization

Colliders are automatically drawn in **Edit Mode** (green wireframes).

In **Play Mode**, use Debug Draw API (TODO: Document).
