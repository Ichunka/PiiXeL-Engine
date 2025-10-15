#include "Components/Transform.hpp"

#include "Components/ComponentModuleMacros.hpp"

#include <cmath>
#include <raymath.h>

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(Transform)
REFLECT_FIELDS()
reflectionBuilder.Field("position", &ReflectedType::position);
reflectionBuilder.Field("rotation", &ReflectedType::rotation,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{
                            .rangeMin = -360.0f, .rangeMax = 360.0f, .dragSpeed = 1.0f});
reflectionBuilder.Field("scale", &ReflectedType::scale);
END_REFLECT_MODULE()

AUTO_SERIALIZATION()

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(0)
SKIP_REGISTRY_RENDER()

EDITOR_UI() {
    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType transform{};
    transform.position = Vector2{0.0f, 0.0f};
    transform.rotation = 0.0f;
    transform.scale = Vector2{1.0f, 1.0f};
    return transform;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    return original;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(Transform)

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

} // namespace PiiXeL
