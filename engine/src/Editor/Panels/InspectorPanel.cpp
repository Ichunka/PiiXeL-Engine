#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/InspectorPanel.hpp"

#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Components/Animator.hpp"
#include "Components/Camera.hpp"
#include "Components/ComponentModuleRegistry.hpp"
#include "Components/Script.hpp"
#include "Components/Sprite.hpp"
#include "Components/Tag.hpp"
#include "Components/Transform.hpp"
#include "Components/UUID.hpp"
#include "Core/Engine.hpp"
#include "Core/Logger.hpp"
#include "Debug/Profiler.hpp"
#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Editor/CommandHistory.hpp"
#include "Editor/EditorCommands.hpp"
#include "Reflection/Reflection.hpp"
#include "Resources/AssetManager.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/AudioAsset.hpp"
#include "Resources/TextureAsset.hpp"
#include "Scene/Scene.hpp"
#include "Scripting/ScriptComponent.hpp"
#include "Scripting/ScriptRegistry.hpp"
#include "Systems/ScriptSystem.hpp"

#include <cinttypes>
#include <cstring>
#include <imgui.h>
#include <rlImGui.h>

namespace PiiXeL {

InspectorPanel::InspectorPanel(Engine* engine, CommandHistory* commandHistory, entt::entity* selectedEntity,
                               bool* inspectorLocked, entt::entity* lockedEntity, UUID* selectedAssetUUID,
                               std::string* selectedAssetPath, AnimatorControllerEditorPanel* animatorControllerEditor,
                               Transform* cachedTransform, bool* isModifyingTransform, Texture2D* defaultWhiteTexture) :
    m_Engine{engine},
    m_CommandHistory{commandHistory}, m_SelectedEntity{selectedEntity}, m_InspectorLocked{inspectorLocked},
    m_LockedEntity{lockedEntity}, m_SelectedAssetUUID{selectedAssetUUID}, m_SelectedAssetPath{selectedAssetPath},
    m_AnimatorControllerEditor{animatorControllerEditor}, m_CachedTransform{cachedTransform},
    m_IsModifyingTransform{isModifyingTransform}, m_DefaultWhiteTexture{defaultWhiteTexture} {}

void InspectorPanel::SetRenderEntityPickerCallback(std::function<bool(const char*, entt::entity*)> callback) {
    m_RenderEntityPickerCallback = callback;
}

void InspectorPanel::SetRenderAssetPickerCallback(
    std::function<bool(const char*, UUID*, const std::string&)> callback) {
    m_RenderAssetPickerCallback = callback;
}

void InspectorPanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    ImGui::Begin("Inspector", &m_IsOpen);

    entt::entity previewEntity = *m_InspectorLocked ? *m_LockedEntity : *m_SelectedEntity;
    bool hasEntitySelection = (previewEntity != entt::null);
    bool hasAssetSelection = (m_SelectedAssetUUID->Get() != 0 || !m_SelectedAssetPath->empty());

    if (!hasEntitySelection && !hasAssetSelection && m_AnimatorControllerEditor &&
        m_AnimatorControllerEditor->IsOpen() && m_AnimatorControllerEditor->HasSelection())
    {
        m_AnimatorControllerEditor->RenderInspector();
        ImGui::End();
        return;
    }

    if (m_SelectedAssetUUID->Get() != 0 || !m_SelectedAssetPath->empty())
    {
        RenderAssetInspector();
        ImGui::End();
        return;
    }

    RenderEntityInspector();

    ImGui::End();
}

void InspectorPanel::RenderAssetInspector() {
    ImGui::TextColored(ImVec4{0.4f, 0.8f, 1.0f, 1.0f}, "Asset");
    ImGui::Separator();

    std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(*m_SelectedAssetUUID);

    bool isSceneFile = false;
    if (m_SelectedAssetPath->size() >= 6)
    {
        std::string extension = m_SelectedAssetPath->substr(m_SelectedAssetPath->size() - 6);
        isSceneFile = (extension == ".scene");
    }

    if (!asset && m_SelectedAssetUUID->Get() == 0 && !m_SelectedAssetPath->empty() && !isSceneFile)
    {
        auto result = AssetRegistry::Instance().LoadAssetFromPath(*m_SelectedAssetPath);
        if (result)
        {
            asset = result;
            *m_SelectedAssetUUID = asset->GetUUID();
        }
    }

    if (asset)
    {
        const AssetMetadata& metadata = asset->GetMetadata();

        ImGui::Text("Name: %s", metadata.name.c_str());
        ImGui::Text("UUID: %" PRIu64, metadata.uuid.Get());

        const char* typeStr = "Unknown";
        switch (metadata.type)
        {
            case AssetType::Texture:
                typeStr = "Texture";
                break;
            case AssetType::Audio:
                typeStr = "Audio";
                break;
            case AssetType::SpriteSheet:
                typeStr = "Sprite Sheet";
                break;
            case AssetType::AnimationClip:
                typeStr = "Animation Clip";
                break;
            case AssetType::AnimatorController:
                typeStr = "Animator Controller";
                break;
            case AssetType::Scene:
                typeStr = "Scene";
                break;
            default:
                break;
        }
        ImGui::Text("Type: %s", typeStr);

        ImGui::Text("Source: %s", metadata.sourceFile.c_str());
        ImGui::Text("Memory: %zu bytes", asset->GetMemoryUsage());
        ImGui::Text("Loaded: %s", asset->IsLoaded() ? "Yes" : "No");

        ImGui::Separator();

        if (metadata.type == AssetType::Texture)
        {
            TextureAsset* texAsset = dynamic_cast<TextureAsset*>(asset.get());
            if (texAsset)
            {
                ImGui::Text("Dimensions: %dx%d", texAsset->GetWidth(), texAsset->GetHeight());
                ImGui::Text("Mipmaps: %d", texAsset->GetMipmaps());
                ImGui::Text("Format: %d", texAsset->GetFormat());

                ImGui::Separator();
                ImGui::Text("Preview:");

                Texture2D texture = texAsset->GetTexture();
                if (texture.id != 0)
                {
                    float maxWidth = ImGui::GetContentRegionAvail().x - 20.0f;
                    float maxHeight = 256.0f;

                    float aspectRatio = static_cast<float>(texture.width) / static_cast<float>(texture.height);
                    float displayWidth = maxWidth;
                    float displayHeight = displayWidth / aspectRatio;

                    if (displayHeight > maxHeight)
                    {
                        displayHeight = maxHeight;
                        displayWidth = displayHeight * aspectRatio;
                    }

                    rlImGuiImageSize(&texture, static_cast<int>(displayWidth), static_cast<int>(displayHeight));
                }
            }
        }
        else if (metadata.type == AssetType::Audio)
        {
            AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
            if (audioAsset)
            {
                ImGui::Text("Frames: %u", audioAsset->GetFrameCount());

                ImGui::Separator();

                Sound sound = audioAsset->GetSound();
                if (sound.frameCount > 0)
                {
                    if (ImGui::Button("Play"))
                    { PlaySound(sound); }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop"))
                    { StopSound(sound); }
                }
            }
        }
        else if (metadata.type == AssetType::SpriteSheet)
        {
            SpriteSheet* spriteSheet = dynamic_cast<SpriteSheet*>(asset.get());
            if (spriteSheet)
            {
                ImGui::Text("Texture UUID: %" PRIu64, spriteSheet->GetTextureUUID().Get());
                ImGui::Text("Grid: %dx%d", spriteSheet->GetGridColumns(), spriteSheet->GetGridRows());
                ImGui::Text("Frame Count: %zu", spriteSheet->GetFrames().size());
            }
        }
        else if (metadata.type == AssetType::AnimationClip)
        {
            AnimationClip* animClip = dynamic_cast<AnimationClip*>(asset.get());
            if (animClip)
            {
                ImGui::Text("Sprite Sheet UUID: %" PRIu64, animClip->GetSpriteSheetUUID().Get());
                ImGui::Text("Frame Count: %zu", animClip->GetFrames().size());
                ImGui::Text("Frame Rate: %.1f fps", animClip->GetFrameRate());
                ImGui::Text("Duration: %.2f seconds", animClip->GetTotalDuration());

                const char* wrapModeStr = "Unknown";
                switch (animClip->GetWrapMode())
                {
                    case AnimationWrapMode::Once:
                        wrapModeStr = "Once";
                        break;
                    case AnimationWrapMode::Loop:
                        wrapModeStr = "Loop";
                        break;
                    case AnimationWrapMode::PingPong:
                        wrapModeStr = "Ping Pong";
                        break;
                }
                ImGui::Text("Wrap Mode: %s", wrapModeStr);
            }
        }
        else if (metadata.type == AssetType::AnimatorController)
        {
            AnimatorController* controller = dynamic_cast<AnimatorController*>(asset.get());
            if (controller)
            {
                ImGui::Text("Default State: %s", controller->GetDefaultState().c_str());
                ImGui::Text("Parameters: %zu", controller->GetParameters().size());
                ImGui::Text("States: %zu", controller->GetStates().size());
                ImGui::Text("Transitions: %zu", controller->GetTransitions().size());
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Reimport"))
        { AssetRegistry::Instance().ReimportAsset(metadata.sourceFile); }
    }
    else
    {
        ImGui::Text("Path: %s", m_SelectedAssetPath->c_str());
        ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "Scene file (not an importable asset)");
    }
}

void InspectorPanel::RenderEntityInspector() {
    if (ImGui::Button(*m_InspectorLocked ? "Unlock" : "Lock"))
    {
        *m_InspectorLocked = !*m_InspectorLocked;
        if (*m_InspectorLocked)
        { *m_LockedEntity = *m_SelectedEntity; }
    }
    ImGui::SameLine();
    ImGui::TextDisabled(*m_InspectorLocked ? "(Inspector Locked)" : "(Inspector Unlocked)");

    entt::entity inspectedEntity = *m_InspectorLocked ? *m_LockedEntity : *m_SelectedEntity;

    if (inspectedEntity != entt::null && m_Engine && m_Engine->GetActiveScene())
    {
        Scene* scene = m_Engine->GetActiveScene();
        entt::registry& registry = scene->GetRegistry();

        if (registry.valid(inspectedEntity))
        {
            if (registry.all_of<Tag>(inspectedEntity))
            {
                Tag& tag = registry.get<Tag>(inspectedEntity);

                char buffer[256]{};
                size_t copyLen = (tag.name.length() < sizeof(buffer) - 1) ? tag.name.length() : sizeof(buffer) - 1;
                std::memcpy(buffer, tag.name.c_str(), copyLen);
                buffer[sizeof(buffer) - 1] = '\0';

                if (ImGui::InputText("Name", buffer, sizeof(buffer)))
                { tag.name = std::string(buffer); }
            }

            ImGui::Separator();

            if (registry.all_of<Transform>(inspectedEntity))
            { RenderTransformComponent(registry, inspectedEntity); }

            if (registry.all_of<Sprite>(inspectedEntity))
            { RenderSpriteComponent(registry, inspectedEntity); }

            ComponentModuleRegistry::Instance().RenderInspectorForEntity(registry, inspectedEntity, *m_CommandHistory,
                                                                         m_RenderEntityPickerCallback,
                                                                         m_RenderAssetPickerCallback);

            if (registry.all_of<Script>(inspectedEntity))
            { RenderScriptComponent(registry, inspectedEntity); }

            ImGui::Separator();

            RenderAddComponentMenu(registry, inspectedEntity);
        }
    }
}

void InspectorPanel::RenderTransformComponent(entt::registry& registry, entt::entity entity) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Transform& transform = registry.get<Transform>(entity);

        if (!*m_IsModifyingTransform)
        { *m_CachedTransform = transform; }

        bool posModified = ImGui::DragFloat2("Position", &transform.position.x, 1.0f);
        bool rotModified = ImGui::DragFloat("Rotation", &transform.rotation, 0.5f);
        bool scaleModified = ImGui::DragFloat2("Scale", &transform.scale.x, 1.0f);

        if (posModified || rotModified || scaleModified)
        {
            if (!*m_IsModifyingTransform)
            { *m_IsModifyingTransform = true; }
        }

        if (*m_IsModifyingTransform && !ImGui::IsAnyItemActive())
        {
            if (m_CachedTransform->position.x != transform.position.x ||
                m_CachedTransform->position.y != transform.position.y ||
                m_CachedTransform->rotation != transform.rotation || m_CachedTransform->scale.x != transform.scale.x ||
                m_CachedTransform->scale.y != transform.scale.y)
            {
                m_CommandHistory->AddCommand(
                    std::make_unique<ModifyTransformCommand>(&registry, entity, *m_CachedTransform, transform));
            }
            *m_IsModifyingTransform = false;
        }
    }
}

void InspectorPanel::RenderSpriteComponent(entt::registry& registry, entt::entity entity) {
    ImGui::Separator();
    bool removeSprite = false;

    ImGui::AlignTextToFramePadding();
    bool spriteOpen = ImGui::TreeNodeEx("Sprite", ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 25);
    if (ImGui::SmallButton("X##RemoveSprite"))
    { removeSprite = true; }

    if (spriteOpen)
    {
        Sprite& sprite = registry.get<Sprite>(entity);

        ImGui::Text("Texture");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.2f, 0.2f, 1.0f});

        Texture2D displayTexture = *m_DefaultWhiteTexture;
        float previewWidth = 100.0f;
        float previewHeight = 100.0f;
        bool isDefaultTexture = true;

        if (sprite.IsValid())
        {
            Texture2D texture = sprite.GetTexture();
            if (texture.id != 0)
            {
                displayTexture = texture;
                isDefaultTexture = false;
                float aspectRatio = static_cast<float>(texture.width) / static_cast<float>(texture.height);
                previewHeight = previewWidth / aspectRatio;
            }
        }

        if (displayTexture.id == 0)
        { ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "ERROR: No texture to display!"); }
        else
        {
            ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(displayTexture.id));
            if (ImGui::ImageButton("##TexturePreview", texId, ImVec2{previewWidth, previewHeight}))
            {}

            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_TEXTURE"))
                {
                    AssetInfo* assetInfo = *static_cast<AssetInfo**>(payload->Data);
                    if (assetInfo)
                    {
                        std::shared_ptr<Asset> asset = AssetRegistry::Instance().LoadAssetFromPath(assetInfo->path);
                        if (asset && asset->GetMetadata().type == AssetType::Texture)
                        {
                            sprite.SetTexture(asset->GetUUID());
                            PX_LOG_INFO(EDITOR, "Texture assigned: %s", assetInfo->path.c_str());
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (isDefaultTexture)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4{0.7f, 0.7f, 0.7f, 1.0f}, "(None)");
            }
        }

        ImGui::PopStyleColor();

        if (sprite.IsValid())
        {
            Texture2D texture = sprite.GetTexture();
            if (texture.id != 0)
            { ImGui::Text("%dx%d", texture.width, texture.height); }
        }

        Reflection::ImGuiRenderer::RenderProperties(sprite, m_RenderEntityPickerCallback, m_RenderAssetPickerCallback);

        ImGui::TreePop();
    }

    if (removeSprite)
    { m_CommandHistory->ExecuteCommand(std::make_unique<RemoveComponentCommand<Sprite>>(&registry, entity)); }
}

void InspectorPanel::RenderScriptComponent(entt::registry& registry, entt::entity entity) {
    ImGui::Separator();
    bool removeScriptComponent = false;

    ImGui::AlignTextToFramePadding();
    bool scriptOpen = ImGui::TreeNodeEx("Scripts", ImGuiTreeNodeFlags_DefaultOpen);

    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
    if (ImGui::SmallButton("X##RemoveScript"))
    { removeScriptComponent = true; }

    if (scriptOpen)
    {
        Script& scriptComponent = registry.get<Script>(entity);

        int scriptToRemove = -1;
        for (size_t i = 0; i < scriptComponent.scripts.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            ScriptInstance& script = scriptComponent.scripts[i];

            ImGui::AlignTextToFramePadding();
            bool scriptItemOpen = ImGui::TreeNodeEx((std::string("##Script") + std::to_string(i)).c_str(),
                                                    ImGuiTreeNodeFlags_DefaultOpen);

            ImGui::SameLine();
            ImGui::Text("[%zu] %s", i, script.scriptName.c_str());

            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
            if (ImGui::SmallButton("X"))
            { scriptToRemove = static_cast<int>(i); }

            if (scriptItemOpen)
            {
                if (script.instance)
                {
                    ImGui::Checkbox("Enabled", &script.instance->m_Enabled);
                    ImGui::Spacing();

                    const Reflection::TypeInfo* typeInfo =
                        Reflection::TypeRegistry::Instance().GetTypeInfo(typeid(*script.instance));
                    if (typeInfo)
                    {
                        for (const Reflection::FieldInfo& field : typeInfo->GetFields())
                        {
                            if (field.flags & Reflection::FieldFlags::ReadOnly)
                                continue;
                            void* fieldPtr = field.getPtr(static_cast<void*>(script.instance.get()));
                            Reflection::ImGuiRenderer::RenderField(field, fieldPtr, m_RenderEntityPickerCallback,
                                                                   m_RenderAssetPickerCallback);
                        }
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4{1.0f, 0.5f, 0.0f, 1.0f}, "Script not loaded: %s",
                                       script.scriptName.c_str());
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (scriptToRemove >= 0)
        { scriptComponent.RemoveScript(static_cast<size_t>(scriptToRemove)); }

        ImGui::Spacing();
        if (m_Engine && m_Engine->GetScriptSystem())
        {
            ScriptSystem* scriptSystem = m_Engine->GetScriptSystem();
            const auto& registeredScripts = ScriptRegistry::Instance().GetAllScripts();

            if (ImGui::Button("+ Add Script", ImVec2{-1, 0}))
            { ImGui::OpenPopup("AddScriptPopup"); }

            if (ImGui::BeginPopup("AddScriptPopup"))
            {
                ImGui::Text("Select a script:");
                ImGui::Separator();
                for (const auto& [name, factory] : registeredScripts)
                {
                    if (ImGui::Selectable(name.c_str()))
                    {
                        std::shared_ptr<ScriptComponent> instance = scriptSystem->CreateScript(name);
                        if (instance)
                        {
                            instance->Initialize(entity, m_Engine->GetActiveScene());
                            scriptComponent.AddScript(instance, name);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
        }

        ImGui::TreePop();
    }

    if (removeScriptComponent)
    { registry.remove<Script>(entity); }
}

void InspectorPanel::RenderAddComponentMenu(entt::registry& registry, entt::entity entity) {
    if (ImGui::Button("Add Component"))
    { ImGui::OpenPopup("AddComponentPopup"); }

    if (ImGui::BeginPopup("AddComponentPopup"))
    {
        if (ImGui::MenuItem("Sprite"))
        {
            if (!registry.all_of<Sprite>(entity))
            {
                m_CommandHistory->ExecuteCommand(
                    std::make_unique<AddComponentCommand<Sprite>>(&registry, entity, Sprite{}));
            }
        }
        ComponentModuleRegistry::Instance().RenderAddComponentMenu(registry, entity, *m_CommandHistory);

        if (ImGui::MenuItem("Script"))
        {
            if (!registry.all_of<Script>(entity))
            { registry.emplace<Script>(entity); }
        }
        ImGui::EndPopup();
    }
}

} // namespace PiiXeL

#endif
