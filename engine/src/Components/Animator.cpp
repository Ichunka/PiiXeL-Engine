#include "Components/Animator.hpp"

#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(Animator)
REFLECT_FIELDS()
reflectionBuilder.Field("isPlaying", &ReflectedType::isPlaying);
reflectionBuilder.Field("playbackSpeed", &ReflectedType::playbackSpeed,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.1f, .rangeMax = 5.0f, .dragSpeed = 0.1f});
END_REFLECT_MODULE()

module->SetSerializer([](const ReflectedType& animator) -> nlohmann::json {
    return nlohmann::json{{"controllerUUID", animator.controllerUUID.Get()},
                          {"isPlaying", animator.isPlaying},
                          {"playbackSpeed", animator.playbackSpeed}};
});

module->SetDeserializer([](ReflectedType& animator, const nlohmann::json& data) {
    if (data.contains("controllerUUID"))
    { animator.controllerUUID = UUID{data["controllerUUID"].get<uint64_t>()}; }
    animator.isPlaying = data.value("isPlaying", true);
    animator.playbackSpeed = data.value("playbackSpeed", 1.0f);
});

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(40)

EDITOR_UI() {
    assetPicker("Controller", &component.controllerUUID, "AnimatorController");

    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);

    if (!component.currentState.empty())
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "State: %s", component.currentState.c_str());
        ImGui::Text("Time: %.2f", component.stateTime);
        ImGui::Text("Frame: %zu", component.currentFrameIndex);
    }
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType animator{};
    animator.controllerUUID = UUID{0};
    animator.isPlaying = true;
    animator.playbackSpeed = 1.0f;
    return animator;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    ReflectedType copy{};
    copy.controllerUUID = original.controllerUUID;
    copy.isPlaying = original.isPlaying;
    copy.playbackSpeed = original.playbackSpeed;
    return copy;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(Animator)

} // namespace PiiXeL
