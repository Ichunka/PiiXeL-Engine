#include "Components/BoxCollider2D.hpp"

#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include "Components/Sprite.hpp"

#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(BoxCollider2D)
REFLECT_FIELDS()
reflectionBuilder.Field("size", &ReflectedType::size);
reflectionBuilder.Field("offset", &ReflectedType::offset);
reflectionBuilder.Field("isTrigger", &ReflectedType::isTrigger);
END_REFLECT_MODULE()

AUTO_SERIALIZATION()

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(29)

EDITOR_UI() {
    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);

    if (registry.all_of<Sprite>(entity))
    {
        if (ImGui::Button("Fit to Sprite"))
        {
            const Sprite& sprite = registry.get<Sprite>(entity);
            Vector2 spriteSize = sprite.GetSize();
            component.size = spriteSize;
            component.offset = Vector2{0.0f, 0.0f};
        }
    }
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType collider{};
    collider.size = Vector2{16.0f, 16.0f};
    if (registry.all_of<Sprite>(entity))
    {
        const Sprite& sprite = registry.get<Sprite>(entity);
        if (sprite.sourceRect.width > 0.0f && sprite.sourceRect.height > 0.0f)
        { collider.size = Vector2{sprite.sourceRect.width, sprite.sourceRect.height}; }
    }
    return collider;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    ReflectedType copy = original;
    copy.box2dFixture = nullptr;
    return copy;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(BoxCollider2D)

} // namespace PiiXeL
