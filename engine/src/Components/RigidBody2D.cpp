#include "Components/RigidBody2D.hpp"

#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(RigidBody2D)
REFLECT_FIELDS()
reflectionBuilder.Field("mass", &ReflectedType::mass,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.1f, .rangeMax = 1000.0f, .dragSpeed = 0.5f});
reflectionBuilder.Field("friction", &ReflectedType::friction,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 1.0f, .dragSpeed = 0.01f});
reflectionBuilder.Field("restitution", &ReflectedType::restitution,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 1.0f, .dragSpeed = 0.01f});
reflectionBuilder.Field("fixedRotation", &ReflectedType::fixedRotation);
reflectionBuilder.Field("velocity", &ReflectedType::velocity, ::PiiXeL::Reflection::FieldFlags::None);
reflectionBuilder.Field("angularVelocity", &ReflectedType::angularVelocity, ::PiiXeL::Reflection::FieldFlags::None,
                        ::PiiXeL::Reflection::FieldMetadata{
                            .rangeMin = -360.0f, .rangeMax = 360.0f, .dragSpeed = 1.0f});
END_REFLECT_MODULE()

module->SetSerializer([](const ReflectedType& rb) -> nlohmann::json {
    return nlohmann::json{{"type", static_cast<int>(rb.type)},
                          {"mass", rb.mass},
                          {"friction", rb.friction},
                          {"restitution", rb.restitution},
                          {"fixedRotation", rb.fixedRotation}};
});

module->SetDeserializer([](ReflectedType& rb, const nlohmann::json& data) {
    rb.type = static_cast<BodyType>(data.value("type", 0));
    rb.mass = data.value("mass", 1.0f);
    rb.friction = data.value("friction", 0.3f);
    rb.restitution = data.value("restitution", 0.0f);
    rb.fixedRotation = data.value("fixedRotation", false);
    rb.velocity = Vector2{0.0f, 0.0f};
    rb.angularVelocity = 0.0f;
    rb.box2dBodyId = b2_nullBodyId;
});

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(20)

EDITOR_UI() {
    const char* bodyTypeNames[] = {"Static", "Dynamic", "Kinematic"};
    int currentType = static_cast<int>(component.type);
    if (ImGui::Combo("Body Type", &currentType, bodyTypeNames, 3))
    { component.type = static_cast<BodyType>(currentType); }

    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType rb{};
    rb.type = BodyType::Dynamic;
    rb.mass = 10.0f;
    rb.friction = 0.3f;
    rb.restitution = 0.0f;
    rb.fixedRotation = false;
    rb.velocity = Vector2{0.0f, 0.0f};
    rb.angularVelocity = 0.0f;
    rb.box2dBodyId = b2_nullBodyId;
    return rb;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    ReflectedType copy = original;
    copy.velocity = Vector2{0.0f, 0.0f};
    copy.angularVelocity = 0.0f;
    copy.box2dBodyId = b2_nullBodyId;
    return copy;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(RigidBody2D)

} // namespace PiiXeL
