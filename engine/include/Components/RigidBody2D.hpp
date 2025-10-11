#ifndef PIIXELENGINE_RIGIDBODY2D_HPP
#define PIIXELENGINE_RIGIDBODY2D_HPP

#include <raylib.h>
#include <box2d/box2d.h>

namespace PiiXeL {

enum class BodyType {
    Static,
    Dynamic,
    Kinematic
};

struct RigidBody2D {
    BodyType type{BodyType::Dynamic};
    float mass{10.0f};
    float friction{0.3f};
    float restitution{0.0f};
    bool fixedRotation{false};
    Vector2 velocity{0.0f, 0.0f};
    float angularVelocity{0.0f};
    b2BodyId box2dBodyId{b2_nullBodyId};

    RigidBody2D() = default;
    explicit RigidBody2D(BodyType t) : type{t} {}
};

} // namespace PiiXeL

#endif // PIIXELENGINE_RIGIDBODY2D_HPP
