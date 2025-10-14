#include "Components/Tag.hpp"

#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(Tag)
REFLECT_FIELDS()
reflectionBuilder.Field("name", &ReflectedType::name);
END_REFLECT_MODULE()

AUTO_SERIALIZATION()

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(1)
SKIP_REGISTRY_RENDER()

EDITOR_UI() {
    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType tag{};
    tag.name = "New Entity";
    return tag;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    ReflectedType copy = original;
    copy.name = original.name + " (Copy)";
    return copy;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(Tag)

} // namespace PiiXeL
