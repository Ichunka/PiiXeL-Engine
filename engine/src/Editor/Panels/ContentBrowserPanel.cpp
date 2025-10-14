#ifdef BUILD_WITH_EDITOR

#include "Editor/Panels/ContentBrowserPanel.hpp"
#include "Core/Logger.hpp"
#include "Components/UUID.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/AssetManager.hpp"
#include "Resources/AssetImporter.hpp"
#include "Editor/SpriteSheetEditorPanel.hpp"
#include "Editor/AnimationClipEditorPanel.hpp"
#include "Editor/AnimatorControllerEditorPanel.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Animation/AnimationSerializer.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include "Debug/Profiler.hpp"
#include <imgui.h>
#include <rlImGui.h>
#include <filesystem>
#include <cstring>
#include <algorithm>

namespace PiiXeL {

ContentBrowserPanel::ContentBrowserPanel(
    UUID* selectedAssetUUID,
    std::string* selectedAssetPath,
    entt::entity* selectedEntity,
    SpriteSheetEditorPanel* spriteSheetEditor,
    AnimationClipEditorPanel* animationClipEditor,
    AnimatorControllerEditorPanel* animatorControllerEditor
)
    : m_SelectedAssetUUID{selectedAssetUUID}
    , m_SelectedAssetPath{selectedAssetPath}
    , m_SelectedEntity{selectedEntity}
    , m_SpriteSheetEditor{spriteSheetEditor}
    , m_AnimationClipEditor{animationClipEditor}
    , m_AnimatorControllerEditor{animatorControllerEditor}
{}

void ContentBrowserPanel::SetDeleteAssetCallback(std::function<void(const std::string&)> callback) {
    m_DeleteAssetCallback = callback;
}

void ContentBrowserPanel::SetLoadSceneCallback(std::function<void()> callback) {
    m_LoadSceneCallback = callback;
}

void ContentBrowserPanel::OnImGuiRender() {
    PROFILE_FUNCTION();
    ImGui::Begin("Content Browser");

    // m_Directories
    // m_Files
    // m_CurrentPath
    // m_NeedsRefresh
    // m_DraggedAsset
    // m_ThumbnailSize
    // m_RightClickedItem
    // m_IsRightClickFolder
    // m_ShowNewScenePopup
    // m_ShowNewFolderPopup
    // m_ShowRenamePopup
    // m_ShowNewSpriteSheetPopup
    // m_ShowNewAnimClipPopup
    // m_ShowNewAnimControllerPopup
    // m_NewItemName

    if (m_NeedsRefresh) {
        m_Directories.clear();
        m_Files.clear();

        if (std::filesystem::exists(m_CurrentPath)) {
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(m_CurrentPath)) {
                if (entry.is_directory()) {
                    std::string dirPath = entry.path().string();
                    for (char& c : dirPath) {
                        if (c == '\\') c = '/';
                    }
                    m_Directories.push_back(dirPath);
                } else if (entry.is_regular_file()) {
                    AssetInfo info{};
                    info.path = entry.path().string();
                    for (char& c : info.path) {
                        if (c == '\\') c = '/';
                    }
                    info.filename = entry.path().filename().string();
                    info.extension = entry.path().extension().string();
                    info.fileSize = std::filesystem::file_size(entry.path());

                    if (info.extension == ".pxa" || info.extension == ".package") {
                        continue;
                    }

                    if (info.extension == ".png" || info.extension == ".jpg" || info.extension == ".jpeg" || info.extension == ".bmp" || info.extension == ".tga") {
                        info.type = "texture";
                        Image img = LoadImage(info.path.c_str());
                        if (img.data != nullptr) {
                            info.width = img.width;
                            info.height = img.height;
                            UnloadImage(img);
                        }
                    } else if (info.extension == ".wav" || info.extension == ".ogg" || info.extension == ".mp3") {
                        info.type = "audio";
                    } else if (info.extension == ".scene") {
                        info.type = "scene";
                    } else if (info.extension == ".spritesheet") {
                        info.type = "spritesheet";
                    } else if (info.extension == ".animclip") {
                        info.type = "animclip";
                    } else if (info.extension == ".animcontroller") {
                        info.type = "animcontroller";
                    } else {
                        info.type = "unknown";
                    }

                    info.uuid = AssetRegistry::Instance().GetUUIDFromPath(info.path);

                    m_Files.push_back(info);
                }
            }
        }
        m_NeedsRefresh = false;
    }

    if (ImGui::Button("<-") && m_CurrentPath != "content") {
        size_t lastSlash = m_CurrentPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            m_CurrentPath = m_CurrentPath.substr(0, lastSlash);
        } else {
            m_CurrentPath = "content";
        }
        m_NeedsRefresh = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        m_NeedsRefresh = true;
    }

    ImGui::SameLine();
    ImGui::Text("Path: %s", m_CurrentPath.c_str());

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Size", &m_ThumbnailSize, 64, 256);

    ImGui::Separator();

    if (m_Directories.empty() && m_Files.empty()) {
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Empty folder");
        ImGui::SetCursorPos(ImVec2{
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        });
        ImGui::TextDisabled("Empty folder");
    } else {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = static_cast<int>(panelWidth / (m_ThumbnailSize + 10));
        if (columnCount < 1) columnCount = 1;

        ImGui::Columns(columnCount, nullptr, false);

        for (size_t dirIdx = 0; dirIdx < m_Directories.size(); ++dirIdx) {
            const std::string& dirPath = m_Directories[dirIdx];
            std::string dirName = dirPath;
            size_t lastSlash = dirPath.find_last_of('/');
            if (lastSlash != std::string::npos) {
                dirName = dirPath.substr(lastSlash + 1);
            }

            ImGui::PushID(static_cast<int>(dirIdx));
            ImGui::BeginGroup();

            ImVec4 folderColor{1.0f, 0.9f, 0.4f, 1.0f};
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.3f, 0.3f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.4f, 0.4f, 0.4f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_Text, folderColor);

            if (ImGui::Button(dirName.c_str(), ImVec2{static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)})) {
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                    m_CurrentPath = dirPath;
                    m_NeedsRefresh = true;
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_CurrentPath = dirPath;
                m_NeedsRefresh = true;
            }

            if (ImGui::IsItemHovered() && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                m_RightClickedItem = dirPath;
                m_IsRightClickFolder = true;
                m_ShowRenamePopup = true;
                std::string itemName = dirName;
                std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
            }

            if (ImGui::BeginPopupContextItem()) {
                m_RightClickedItem = dirPath;
                m_IsRightClickFolder = true;

                if (ImGui::MenuItem("Rename")) {
                    m_ShowRenamePopup = true;
                    std::string itemName = dirName;
                    std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                    m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                }

                if (ImGui::MenuItem("Delete")) {
                    try {
                        std::filesystem::remove_all(dirPath);
                        m_NeedsRefresh = true;
                        PX_LOG_INFO(EDITOR, "Deleted: %s", dirPath.c_str());
                    } catch (const std::filesystem::filesystem_error& e) {
                        PX_LOG_ERROR(EDITOR, "Failed to delete folder: %s", e.what());
                    }
                }

                ImGui::EndPopup();
            }

            ImGui::PopStyleColor(3);
            ImGui::TextWrapped("%s", dirName.c_str());
            ImGui::EndGroup();
            ImGui::NextColumn();
            ImGui::PopID();
        }

        for (size_t fileIdx = 0; fileIdx < m_Files.size(); ++fileIdx) {
            AssetInfo& asset = m_Files[fileIdx];

            if (asset.uuid.Get() == 0) {
                UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                if (existingUUID.Get() != 0) {
                    asset.uuid = existingUUID;
                }
            }

            ImGui::PushID(static_cast<int>(m_Directories.size() + fileIdx));
            ImGui::BeginGroup();

            bool isScene = asset.extension == ".scene";

            if (asset.type == "texture") {
                Texture2D tex = AssetManager::Instance().LoadTexture(asset.path);

                if (tex.id != 0) {
                    float aspectRatio = static_cast<float>(tex.width) / static_cast<float>(tex.height);
                    float displayWidth = static_cast<float>(m_ThumbnailSize);
                    float displayHeight = static_cast<float>(m_ThumbnailSize);

                    if (aspectRatio > 1.0f) {
                        displayHeight = displayWidth / aspectRatio;
                    } else {
                        displayWidth = displayHeight * aspectRatio;
                    }

                    ImVec2 imageSize{displayWidth, displayHeight};
                    ImTextureID texId = static_cast<ImTextureID>(static_cast<intptr_t>(tex.id));

                    if (ImGui::ImageButton("##texture", texId, imageSize)) {
                        *m_SelectedAssetPath = asset.path;
                        if (m_AnimatorControllerEditor) {
                            m_AnimatorControllerEditor->ClearSelection();
                        }

                        UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                        std::shared_ptr<Asset> existingAsset = existingUUID.Get() != 0
                            ? AssetRegistry::Instance().GetAsset(existingUUID)
                            : nullptr;

                        if (!existingAsset) {
                            std::shared_ptr<Asset> loadedAsset = AssetRegistry::Instance().LoadAssetFromPath(asset.path);
                            if (loadedAsset) {
                                *m_SelectedAssetUUID = loadedAsset->GetUUID();
                            }
                        } else {
                            *m_SelectedAssetUUID = existingUUID;
                        }

                        *m_SelectedEntity = entt::null;
                    }

                    if (*m_SelectedAssetPath == asset.path && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                        m_RightClickedItem = asset.path;
                        m_IsRightClickFolder = false;
                        m_ShowRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                        m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                    }

                    if (ImGui::BeginPopupContextItem()) {
                        m_RightClickedItem = asset.path;
                        m_IsRightClickFolder = false;

                        if (ImGui::MenuItem("Rename")) {
                            m_ShowRenamePopup = true;
                            std::string itemName = asset.filename;
                            size_t dotPos = itemName.find_last_of('.');
                            if (dotPos != std::string::npos) {
                                itemName = itemName.substr(0, dotPos);
                            }
                            std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                            m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                        }

                        if (ImGui::MenuItem("Delete")) {
                            m_DeleteAssetCallback(asset.path);
                            m_NeedsRefresh = true;
                        }

                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        m_DraggedAsset = &asset;
                        ImGui::SetDragDropPayload("ASSET_TEXTURE", &m_DraggedAsset, sizeof(AssetInfo*));
                        ImGui::Image(texId, ImVec2{64, 64});
                        ImGui::Text("%s", asset.filename.c_str());
                        ImGui::EndDragDropSource();
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("File: %s", asset.filename.c_str());
                        ImGui::Text("Size: %dx%d", asset.width, asset.height);
                        ImGui::Text("Format: %s", asset.extension.c_str());
                        ImGui::Text("File size: %.2f KB", asset.fileSize / 1024.0f);
                        ImGui::EndTooltip();
                    }
                }
            } else if (isScene) {
                ImVec4 sceneColor{0.4f, 0.9f, 1.0f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.3f, 0.4f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.4f, 0.5f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_Text, sceneColor);

                if (ImGui::Button("SCENE", ImVec2{static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)})) {
                    *m_SelectedAssetPath = asset.path;
                    *m_SelectedAssetUUID = UUID{0};
                    *m_SelectedEntity = entt::null;
                    if (m_AnimatorControllerEditor) {
                        m_AnimatorControllerEditor->ClearSelection();
                    }
                }

                if (*m_SelectedAssetPath == asset.path && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                    m_RightClickedItem = asset.path;
                    m_IsRightClickFolder = false;
                    m_ShowRenamePopup = true;
                    std::string itemName = asset.filename;
                    size_t dotPos = itemName.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        itemName = itemName.substr(0, dotPos);
                    }
                    std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                    m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (m_LoadSceneCallback) {
                        m_LoadSceneCallback();
                    }
                }

                if (ImGui::BeginPopupContextItem()) {
                    m_RightClickedItem = asset.path;
                    m_IsRightClickFolder = false;

                    if (ImGui::MenuItem("Rename")) {
                        m_ShowRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                        m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                    }

                    if (ImGui::MenuItem("Delete")) {
                        m_DeleteAssetCallback(asset.path);
                        m_NeedsRefresh = true;
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);
            } else if (asset.type == "spritesheet" || asset.type == "animclip" || asset.type == "animcontroller") {
                ImVec4 animColor = asset.type == "spritesheet" ? ImVec4{0.4f, 0.8f, 0.6f, 1.0f} :
                                   asset.type == "animclip" ? ImVec4{0.6f, 0.8f, 0.4f, 1.0f} :
                                   ImVec4{0.8f, 0.6f, 0.4f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.3f, 0.3f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.4f, 0.4f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_Text, animColor);

                std::string buttonLabel = asset.type == "spritesheet" ? "SHEET" :
                                         asset.type == "animclip" ? "CLIP" : "CTRL";
                if (ImGui::Button(buttonLabel.c_str(), ImVec2{static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)})) {
                    *m_SelectedAssetPath = asset.path;
                    *m_SelectedEntity = entt::null;
                    if (m_AnimatorControllerEditor) {
                        m_AnimatorControllerEditor->ClearSelection();
                    }

                    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                    std::shared_ptr<Asset> existingAsset = existingUUID.Get() != 0
                        ? AssetRegistry::Instance().GetAsset(existingUUID)
                        : nullptr;

                    if (!existingAsset) {
                        std::shared_ptr<Asset> loadedAsset = AssetRegistry::Instance().LoadAssetFromPath(asset.path);
                        if (loadedAsset) {
                            *m_SelectedAssetUUID = loadedAsset->GetUUID();
                        } else {
                            *m_SelectedAssetUUID = UUID{0};
                        }
                    } else {
                        *m_SelectedAssetUUID = existingUUID;
                    }
                }

                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    m_DraggedAsset = &asset;
                    ImGui::SetDragDropPayload("ASSET_ANIM", &m_DraggedAsset, sizeof(AssetInfo*));
                    ImGui::Text("%s: %s", buttonLabel.c_str(), asset.filename.c_str());
                    ImGui::EndDragDropSource();
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (asset.type == "spritesheet" && m_SpriteSheetEditor) {
                        m_SpriteSheetEditor->Open(asset.path);
                    } else if (asset.type == "animclip" && m_AnimationClipEditor) {
                        m_AnimationClipEditor->Open(asset.path);
                    } else if (asset.type == "animcontroller" && m_AnimatorControllerEditor) {
                        m_AnimatorControllerEditor->Open(asset.path);
                    }
                }

                if (ImGui::BeginPopupContextItem()) {
                    m_RightClickedItem = asset.path;
                    m_IsRightClickFolder = false;

                    if (ImGui::MenuItem("Rename")) {
                        m_ShowRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                        m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                    }

                    if (ImGui::MenuItem("Delete")) {
                        m_DeleteAssetCallback(asset.path);
                        m_NeedsRefresh = true;
                    }

                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);
            } else {
                ImVec4 fileColor = asset.type == "audio" ? ImVec4{0.8f, 0.4f, 0.8f, 1.0f} : ImVec4{0.6f, 0.6f, 0.6f, 1.0f};
                ImGui::PushStyleColor(ImGuiCol_Text, fileColor);

                std::string buttonLabel = asset.type == "audio" ? "AUDIO" : "FILE";
                if (ImGui::Button(buttonLabel.c_str(), ImVec2{static_cast<float>(m_ThumbnailSize), static_cast<float>(m_ThumbnailSize)})) {
                    *m_SelectedAssetPath = asset.path;
                    if (m_AnimatorControllerEditor) {
                        m_AnimatorControllerEditor->ClearSelection();
                    }

                    UUID existingUUID = AssetRegistry::Instance().GetUUIDFromPath(asset.path);
                    std::shared_ptr<Asset> existingAsset = existingUUID.Get() != 0
                        ? AssetRegistry::Instance().GetAsset(existingUUID)
                        : nullptr;

                    if (!existingAsset) {
                        std::shared_ptr<Asset> loadedAsset = AssetRegistry::Instance().LoadAssetFromPath(asset.path);
                        if (loadedAsset) {
                            *m_SelectedAssetUUID = loadedAsset->GetUUID();
                        }
                    } else {
                        *m_SelectedAssetUUID = existingUUID;
                    }

                    *m_SelectedEntity = entt::null;
                }

                if (*m_SelectedAssetPath == asset.path && ImGui::IsKeyPressed(ImGuiKey_F2)) {
                    m_RightClickedItem = asset.path;
                    m_IsRightClickFolder = false;
                    m_ShowRenamePopup = true;
                    std::string itemName = asset.filename;
                    size_t dotPos = itemName.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        itemName = itemName.substr(0, dotPos);
                    }
                    std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                    m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                }

                ImGui::PopStyleColor();

                if (ImGui::BeginPopupContextItem()) {
                    m_RightClickedItem = asset.path;
                    m_IsRightClickFolder = false;

                    if (ImGui::MenuItem("Rename")) {
                        m_ShowRenamePopup = true;
                        std::string itemName = asset.filename;
                        size_t dotPos = itemName.find_last_of('.');
                        if (dotPos != std::string::npos) {
                            itemName = itemName.substr(0, dotPos);
                        }
                        std::memcpy(m_NewItemName, itemName.c_str(), std::min(itemName.size(), sizeof(m_NewItemName) - 1));
                        m_NewItemName[sizeof(m_NewItemName) - 1] = '\0';
                    }

                    if (ImGui::MenuItem("Delete")) {
                        m_DeleteAssetCallback(asset.path);
                        m_NeedsRefresh = true;
                    }

                    ImGui::EndPopup();
                }
            }

            std::string displayName = std::filesystem::path(asset.filename).stem().string();
            ImGui::TextWrapped("%s", displayName.c_str());
            ImGui::EndGroup();
            ImGui::NextColumn();
            ImGui::PopID();
        }

        ImGui::Columns(1);
    }

    if (ImGui::BeginPopupContextWindow("ContentContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
        m_RightClickedItem.clear();

        if (ImGui::BeginMenu("Animation")) {
            if (ImGui::MenuItem("Sprite Sheet")) {
                m_ShowNewSpriteSheetPopup = true;
                std::memset(m_NewItemName, 0, sizeof(m_NewItemName));
            }

            if (ImGui::MenuItem("Animation Clip")) {
                m_ShowNewAnimClipPopup = true;
                std::memset(m_NewItemName, 0, sizeof(m_NewItemName));
            }

            if (ImGui::MenuItem("Animator Controller")) {
                m_ShowNewAnimControllerPopup = true;
                std::memset(m_NewItemName, 0, sizeof(m_NewItemName));
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("New Folder")) {
            m_ShowNewFolderPopup = true;
            std::memset(m_NewItemName, 0, sizeof(m_NewItemName));
        }

        ImGui::EndPopup();
    }

    if (m_ShowNewFolderPopup) {
        ImGui::OpenPopup("New Folder");
        m_ShowNewFolderPopup = false;
    }

    if (ImGui::BeginPopupModal("New Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter folder name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##FolderName", m_NewItemName, sizeof(m_NewItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(m_NewItemName) > 0) {
                std::string folderName = std::string(m_NewItemName);
                std::string newFolderPath = m_CurrentPath + "/" + folderName;

                try {
                    std::filesystem::create_directory(newFolderPath);
                    m_NeedsRefresh = true;
                    PX_LOG_INFO(EDITOR, "Created folder: %s", newFolderPath.c_str());
                } catch (const std::filesystem::filesystem_error& e) {
                    PX_LOG_ERROR(EDITOR, "Failed to create folder: %s", e.what());
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_ShowNewSpriteSheetPopup) {
        ImGui::OpenPopup("New Sprite Sheet");
        m_ShowNewSpriteSheetPopup = false;
    }

    if (ImGui::BeginPopupModal("New Sprite Sheet", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter sprite sheet name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##SpriteSheetName", m_NewItemName, sizeof(m_NewItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(m_NewItemName) > 0) {
                std::string name = std::string(m_NewItemName);
                std::string newPath = m_CurrentPath + "/" + name + ".spritesheet";

                UUID newUUID{};
                SpriteSheet spriteSheet{newUUID, name};
                AnimationSerializer::SerializeSpriteSheet(spriteSheet, newPath);

                AssetImporter importer{};
                importer.LoadUUIDCache();
                importer.ForceUUID(newPath, newUUID);
                AssetImporter::ImportResult result = importer.ImportAsset(newPath);
                importer.SaveUUIDCache();

                if (result.success) {
                    AssetRegistry::Instance().ReimportAsset(newPath);
                }

                m_NeedsRefresh = true;
                PX_LOG_INFO(EDITOR, "Created sprite sheet: %s", newPath.c_str());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_ShowNewAnimClipPopup) {
        ImGui::OpenPopup("New Animation Clip");
        m_ShowNewAnimClipPopup = false;
    }

    if (ImGui::BeginPopupModal("New Animation Clip", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter animation clip name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##AnimClipName", m_NewItemName, sizeof(m_NewItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(m_NewItemName) > 0) {
                std::string name = std::string(m_NewItemName);
                std::string newPath = m_CurrentPath + "/" + name + ".animclip";

                UUID newUUID{};
                AnimationClip clip{newUUID, name};
                AnimationSerializer::SerializeAnimationClip(clip, newPath);

                AssetImporter importer{};
                importer.LoadUUIDCache();
                importer.ForceUUID(newPath, newUUID);
                AssetImporter::ImportResult result = importer.ImportAsset(newPath);
                importer.SaveUUIDCache();

                if (result.success) {
                    AssetRegistry::Instance().ReimportAsset(newPath);
                }

                m_NeedsRefresh = true;
                PX_LOG_INFO(EDITOR, "Created animation clip: %s", newPath.c_str());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_ShowNewAnimControllerPopup) {
        ImGui::OpenPopup("New Animator Controller");
        m_ShowNewAnimControllerPopup = false;
    }

    if (ImGui::BeginPopupModal("New Animator Controller", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter animator controller name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##AnimControllerName", m_NewItemName, sizeof(m_NewItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(m_NewItemName) > 0) {
                std::string name = std::string(m_NewItemName);
                std::string newPath = m_CurrentPath + "/" + name + ".animcontroller";

                UUID newUUID{};
                AnimatorController controller{newUUID, name};
                AnimationSerializer::SerializeAnimatorController(controller, newPath);

                AssetImporter importer{};
                importer.LoadUUIDCache();
                importer.ForceUUID(newPath, newUUID);
                AssetImporter::ImportResult result = importer.ImportAsset(newPath);
                importer.SaveUUIDCache();

                if (result.success) {
                    AssetRegistry::Instance().ReimportAsset(newPath);
                }

                m_NeedsRefresh = true;
                PX_LOG_INFO(EDITOR, "Created animator controller: %s", newPath.c_str());
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (m_ShowRenamePopup) {
        ImGui::OpenPopup("Rename");
        m_ShowRenamePopup = false;
    }

    if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Enter new name:");
        ImGui::Separator();

        bool enterPressed = ImGui::InputText("##RenameName", m_NewItemName, sizeof(m_NewItemName), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2{120, 0}) || enterPressed) {
            if (std::strlen(m_NewItemName) > 0 && !m_RightClickedItem.empty()) {
                std::string newName = std::string(m_NewItemName);
                std::string parentPath = m_CurrentPath;

                size_t lastSlash = m_RightClickedItem.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    parentPath = m_RightClickedItem.substr(0, lastSlash);
                }

                std::string newPath;
                if (m_IsRightClickFolder) {
                    newPath = parentPath + "/" + newName;
                } else {
                    std::string extension;
                    size_t dotPos = m_RightClickedItem.find_last_of('.');
                    if (dotPos != std::string::npos) {
                        extension = m_RightClickedItem.substr(dotPos);
                    }
                    newPath = parentPath + "/" + newName + extension;
                }

                try {
                    std::filesystem::rename(m_RightClickedItem, newPath);
                    m_NeedsRefresh = true;
                    PX_LOG_INFO(EDITOR, "Renamed: %s -> %s", m_RightClickedItem.c_str(), newPath.c_str());
                    m_RightClickedItem.clear();
                } catch (const std::filesystem::filesystem_error& e) {
                    PX_LOG_ERROR(EDITOR, "Failed to rename: %s", e.what());
                }

                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2{120, 0})) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::End();
}



}

#endif
