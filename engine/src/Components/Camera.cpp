#include "Components/Camera.hpp"
#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(Camera)
    REFLECT_FIELDS()
        reflectionBuilder.Field("isPrimary", &ReflectedType::isPrimary);
        reflectionBuilder.Field("offset", &ReflectedType::offset);
        reflectionBuilder.Field("zoom", &ReflectedType::zoom,
            ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
            ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.1f, .rangeMax = 10.0f, .dragSpeed = 0.1f});
        reflectionBuilder.Field("rotation", &ReflectedType::rotation,
            ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
            ::PiiXeL::Reflection::FieldMetadata{.rangeMin = -360.0f, .rangeMax = 360.0f, .dragSpeed = 1.0f});
    END_REFLECT_MODULE()

    AUTO_SERIALIZATION()

    #ifdef BUILD_WITH_EDITOR
    EDITOR_DISPLAY_ORDER(5)

    EDITOR_UI() {
        ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);
    }
    EDITOR_UI_END()

    EDITOR_CREATE_DEFAULT() {
        ReflectedType camera{};
        camera.isPrimary = true;
        camera.offset = Vector2{0.0f, 0.0f};
        camera.zoom = 1.0f;
        camera.rotation = 0.0f;
        return camera;
    }
    EDITOR_CREATE_DEFAULT_END()

    EDITOR_DUPLICATE() {
        ReflectedType copy = original;
        copy.isPrimary = false;
        return copy;
    }
    EDITOR_DUPLICATE_END()
    #endif
END_COMPONENT_MODULE(Camera)

}
