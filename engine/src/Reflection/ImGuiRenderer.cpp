#ifdef BUILD_WITH_EDITOR

#include "Reflection/ImGuiRenderer.hpp"

#include "Components/UUID.hpp"
#include "Scripting/EntityRef.hpp"

#include <imgui.h>
#include <raylib.h>

namespace PiiXeL::Reflection {

bool ImGuiRenderer::RenderField(const FieldInfo& field, void* fieldPtr, EntityPickerCallback entityPicker,
                                AssetPickerCallback assetPicker) {
    bool modified = false;

    switch (field.type)
    {
        case FieldType::Float: {
            float* value = static_cast<float*>(fieldPtr);
            if (field.flags & FieldFlags::Range)
            {
                modified = ImGui::DragFloat(field.name.c_str(), value, field.metadata.dragSpeed,
                                            field.metadata.rangeMin, field.metadata.rangeMax);
            }
            else
            { modified = ImGui::DragFloat(field.name.c_str(), value, 0.1f); }
            break;
        }

        case FieldType::Int: {
            int* value = static_cast<int*>(fieldPtr);
            if (field.flags & FieldFlags::Range)
            {
                modified = ImGui::DragInt(field.name.c_str(), value, 1.0f, static_cast<int>(field.metadata.rangeMin),
                                          static_cast<int>(field.metadata.rangeMax));
            }
            else
            { modified = ImGui::DragInt(field.name.c_str(), value); }
            break;
        }

        case FieldType::Bool: {
            bool* value = static_cast<bool*>(fieldPtr);
            modified = ImGui::Checkbox(field.name.c_str(), value);
            break;
        }

        case FieldType::String: {
            std::string* value = static_cast<std::string*>(fieldPtr);
            char buffer[256];
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
            strncpy(buffer, value->c_str(), sizeof(buffer) - 1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
            buffer[sizeof(buffer) - 1] = '\0';
            if (ImGui::InputText(field.name.c_str(), buffer, sizeof(buffer)))
            {
                *value = buffer;
                modified = true;
            }
            break;
        }

        case FieldType::Vector2: {
            Vector2* value = static_cast<Vector2*>(fieldPtr);
            modified = ImGui::DragFloat2(field.name.c_str(), &value->x, 1.0f);
            break;
        }

        case FieldType::Color: {
            Color* value = static_cast<Color*>(fieldPtr);
            float col[4] = {value->r / 255.0f, value->g / 255.0f, value->b / 255.0f, value->a / 255.0f};
            if (ImGui::ColorEdit4(field.name.c_str(), col))
            {
                value->r = static_cast<unsigned char>(col[0] * 255.0f);
                value->g = static_cast<unsigned char>(col[1] * 255.0f);
                value->b = static_cast<unsigned char>(col[2] * 255.0f);
                value->a = static_cast<unsigned char>(col[3] * 255.0f);
                modified = true;
            }
            break;
        }

        case FieldType::Entity: {
            if (entityPicker && (field.flags & FieldFlags::EntityPicker))
            {
                entt::entity* value = static_cast<entt::entity*>(fieldPtr);
                modified = entityPicker(field.name.c_str(), value);
            }
            else
            {
                entt::entity* value = static_cast<entt::entity*>(fieldPtr);
                uint32_t entityId = static_cast<uint32_t>(*value);
                ImGui::Text("%s: Entity(%u)", field.name.c_str(), entityId);
            }
            break;
        }

        case FieldType::EntityRef: {
            if (entityPicker && (field.flags & FieldFlags::EntityPicker))
            {
                EntityRef* value = static_cast<EntityRef*>(fieldPtr);
                entt::entity entity = value->Get();
                if (entityPicker(field.name.c_str(), &entity))
                {
                    value->Set(entity);
                    modified = true;
                }
            }
            else
            {
                EntityRef* value = static_cast<EntityRef*>(fieldPtr);
                entt::entity entity = value->Get();
                uint32_t entityId = static_cast<uint32_t>(entity);
                ImGui::Text("%s: Entity(%u)", field.name.c_str(), entityId);
            }
            break;
        }

        case FieldType::AssetRef: {
            if (assetPicker && (field.flags & FieldFlags::AssetPicker))
            {
                UUID* value = static_cast<UUID*>(fieldPtr);
                if (assetPicker(field.name.c_str(), value, field.metadata.assetType))
                { modified = true; }
            }
            else
            {
                UUID* value = static_cast<UUID*>(fieldPtr);
                ImGui::Text("%s: %s", field.name.c_str(), value->ToString().c_str());
            }
            break;
        }

        case FieldType::Custom:
        default:
            ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "%s: <custom type>", field.name.c_str());
            break;
    }

    return modified;
}

} // namespace PiiXeL::Reflection

#endif
