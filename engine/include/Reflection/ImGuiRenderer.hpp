#ifndef PIIXELENGINE_IMGUIRENDERER_HPP
#define PIIXELENGINE_IMGUIRENDERER_HPP

#ifdef BUILD_WITH_EDITOR

#include "Components/UUID.hpp"
#include "Reflection/TypeInfo.hpp"
#include "Reflection/TypeRegistry.hpp"

#include <entt/entt.hpp>

#include <functional>

namespace PiiXeL::Reflection {

using EntityPickerCallback = std::function<bool(const char*, entt::entity*)>;
using AssetPickerCallback = std::function<bool(const char*, UUID*, const std::string&)>;

class ImGuiRenderer {
public:
    template <typename T>
    static bool RenderProperties(T& object, EntityPickerCallback entityPicker = nullptr,
                                 AssetPickerCallback assetPicker = nullptr) {
        const TypeInfo* typeInfo = TypeRegistry::Instance().GetTypeInfo<T>();
        if (!typeInfo) {
            return false;
        }

        bool modified = false;
        for (const FieldInfo& field : typeInfo->GetFields()) {
            if (field.flags & FieldFlags::ReadOnly) {
                continue;
            }

            void* fieldPtr = field.getPtr(static_cast<void*>(&object));
            if (RenderField(field, fieldPtr, entityPicker, assetPicker)) {
                modified = true;
            }
        }

        return modified;
    }

    static bool RenderField(const FieldInfo& field, void* fieldPtr, EntityPickerCallback entityPicker,
                            AssetPickerCallback assetPicker);
};

} // namespace PiiXeL::Reflection

#endif

#endif
