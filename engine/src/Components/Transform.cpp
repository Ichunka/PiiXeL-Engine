#include "Components/Transform.hpp"
#include "Reflection/Reflection.hpp"
#include <raymath.h>
#include <cmath>

namespace PiiXeL {

BEGIN_REFLECT(Transform)
    FIELD(position)
    FIELD_RANGE(rotation, -360.0f, 360.0f, 1.0f)
    FIELD(scale)
END_REFLECT(Transform)

Vector2 Transform::GetRight() const {
    float radians{rotation * DEG2RAD};
    return Vector2{std::cos(radians), std::sin(radians)};
}

Vector2 Transform::GetUp() const {
    float radians{rotation * DEG2RAD};
    return Vector2{-std::sin(radians), std::cos(radians)};
}

Matrix Transform::GetMatrix() const {
    Matrix mat{MatrixIdentity()};

    mat = MatrixMultiply(mat, MatrixTranslate(position.x, position.y, 0.0f));
    mat = MatrixMultiply(mat, MatrixRotateZ(rotation * DEG2RAD));
    mat = MatrixMultiply(mat, MatrixScale(scale.x, scale.y, 1.0f));

    return mat;
}

namespace Reflection {
void __force_link_Transform() {}
}

} // namespace PiiXeL
