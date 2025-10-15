#pragma once

#ifdef BUILD_WITH_EDITOR

#include <entt/entt.hpp>

#include <string>

namespace PiiXeL {

class Engine;
class UUID;

class EditorAssetPickerUtility {
public:
    static bool RenderEntityPicker(const char* label, entt::entity* entity, Engine* engine);
    static bool RenderAssetPicker(const char* label, UUID* uuid, const std::string& assetType);
};

} // namespace PiiXeL

#endif
