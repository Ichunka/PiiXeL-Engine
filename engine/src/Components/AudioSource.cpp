#include "Components/AudioSource.hpp"

#include "Components/ComponentModuleMacros.hpp"

#ifdef BUILD_WITH_EDITOR
#include <imgui.h>
#endif

namespace PiiXeL {

BEGIN_COMPONENT_MODULE(AudioSource)
REFLECT_FIELDS()
reflectionBuilder.Field("audioClip", &ReflectedType::audioClip,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable |
                            ::PiiXeL::Reflection::FieldFlags::AssetPicker,
                        ::PiiXeL::Reflection::FieldMetadata{.assetType = "Audio"});
reflectionBuilder.Field("playOnAwake", &ReflectedType::playOnAwake);
reflectionBuilder.Field("loop", &ReflectedType::loop);
reflectionBuilder.Field("mute", &ReflectedType::mute);
reflectionBuilder.Field("spatialize", &ReflectedType::spatialize);
reflectionBuilder.Field("volume", &ReflectedType::volume,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 1.0f, .dragSpeed = 0.01f});
reflectionBuilder.Field("pitch", &ReflectedType::pitch,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.1f, .rangeMax = 3.0f, .dragSpeed = 0.01f});
reflectionBuilder.Field("spatialBlend", &ReflectedType::spatialBlend,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 1.0f, .dragSpeed = 0.01f});
reflectionBuilder.Field("minDistance", &ReflectedType::minDistance,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.1f, .rangeMax = 1000.0f, .dragSpeed = 1.0f});
reflectionBuilder.Field("maxDistance", &ReflectedType::maxDistance,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{
                            .rangeMin = 1.0f, .rangeMax = 10000.0f, .dragSpeed = 10.0f});
reflectionBuilder.Field("priority", &ReflectedType::priority,
                        ::PiiXeL::Reflection::FieldFlags::Public | ::PiiXeL::Reflection::FieldFlags::Serializable,
                        ::PiiXeL::Reflection::FieldMetadata{.rangeMin = 0.0f, .rangeMax = 256.0f, .dragSpeed = 1.0f});
END_REFLECT_MODULE()

module->SetSerializer([](const ReflectedType& source) -> nlohmann::json {
    return nlohmann::json{{"audioClip", source.audioClip.Get()},
                          {"playOnAwake", source.playOnAwake},
                          {"loop", source.loop},
                          {"mute", source.mute},
                          {"spatialize", source.spatialize},
                          {"volume", source.volume},
                          {"pitch", source.pitch},
                          {"spatialBlend", source.spatialBlend},
                          {"minDistance", source.minDistance},
                          {"maxDistance", source.maxDistance},
                          {"priority", source.priority}};
});

module->SetDeserializer([](ReflectedType& source, const nlohmann::json& data) {
    if (data.contains("audioClip") && data["audioClip"].is_number()) {
        source.audioClip = UUID{data["audioClip"].get<uint64_t>()};
    }
    source.playOnAwake = data.value("playOnAwake", false);
    source.loop = data.value("loop", false);
    source.mute = data.value("mute", false);
    source.spatialize = data.value("spatialize", true);
    source.volume = data.value("volume", 1.0f);
    source.pitch = data.value("pitch", 1.0f);
    source.spatialBlend = data.value("spatialBlend", 1.0f);
    source.minDistance = data.value("minDistance", 1.0f);
    source.maxDistance = data.value("maxDistance", 500.0f);
    source.priority = data.value("priority", 128.0f);
});

#ifdef BUILD_WITH_EDITOR
EDITOR_DISPLAY_ORDER(50)

EDITOR_UI() {
    ::PiiXeL::Reflection::ImGuiRenderer::RenderProperties(component, entityPicker, assetPicker);
}
EDITOR_UI_END()

EDITOR_CREATE_DEFAULT() {
    ReflectedType source{};
    source.volume = 1.0f;
    source.pitch = 1.0f;
    source.spatialBlend = 1.0f;
    source.minDistance = 1.0f;
    source.maxDistance = 500.0f;
    source.priority = 128.0f;
    source.spatialize = true;
    return source;
}
EDITOR_CREATE_DEFAULT_END()

EDITOR_DUPLICATE() {
    ReflectedType copy{};
    copy.audioClip = original.audioClip;
    copy.playOnAwake = original.playOnAwake;
    copy.loop = original.loop;
    copy.mute = original.mute;
    copy.spatialize = original.spatialize;
    copy.volume = original.volume;
    copy.pitch = original.pitch;
    copy.spatialBlend = original.spatialBlend;
    copy.minDistance = original.minDistance;
    copy.maxDistance = original.maxDistance;
    copy.priority = original.priority;
    return copy;
}
EDITOR_DUPLICATE_END()
#endif
END_COMPONENT_MODULE(AudioSource)

} // namespace PiiXeL
