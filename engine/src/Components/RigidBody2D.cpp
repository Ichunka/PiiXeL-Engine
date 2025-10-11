#include "Components/RigidBody2D.hpp"
#include "Reflection/Reflection.hpp"

namespace PiiXeL {

BEGIN_REFLECT(RigidBody2D)
    FIELD_RANGE(mass, 0.1f, 1000.0f, 0.5f)
    FIELD_RANGE(friction, 0.0f, 1.0f, 0.01f)
    FIELD_RANGE(restitution, 0.0f, 1.0f, 0.01f)
    FIELD(fixedRotation)
    FIELD(velocity)
    FIELD_RANGE(angularVelocity, -360.0f, 360.0f, 1.0f)
END_REFLECT(RigidBody2D)

namespace Reflection {
void __force_link_RigidBody2D() {}
}

}
