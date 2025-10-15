#include "Components/AudioListener.hpp"

#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(AudioListener)
REFLECT_FIELDS()
reflectionBuilder.Field("isActive", &ReflectedType::isActive);
reflectionBuilder.Field("volume", &ReflectedType::volume,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 1.0f, .dragSpeed = 0.01f});
reflectionBuilder.Field("pauseOnFocusLoss", &ReflectedType::pauseOnFocusLoss);
END_REFLECT_MODULE()

AUTO_SERIALIZATION()

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(51)

EDITOR_UI() {
    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType listener{};
    listener.isActive = true;
    listener.volume = 1.0f;
    listener.pauseOnFocusLoss = true;
    return listener;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    ReflectedType copy{};
    copy.isActive = original.isActive;
    copy.volume = original.volume;
    copy.pauseOnFocusLoss = original.pauseOnFocusLoss;
    return copy;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(AudioListener)

} // namespace PiiXeL
