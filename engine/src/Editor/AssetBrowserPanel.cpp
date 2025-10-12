#ifdef BUILD_WITH_EDITOR

#include "Editor/AssetBrowserPanel.hpp"
#include "Resources/AssetRegistry.hpp"
#include <imgui.h>
#include <filesystem>
#include <algorithm>

namespace PiiXeL {

void AssetBrowserPanel::Render() {
    ImGui::Begin("Asset Browser");

    if (ImGui::Button("Refresh")) {
        RefreshAssetList();
    }

    ImGui::SameLine();
    if (ImGui::Button("Import Directory")) {
        AssetRegistry::Instance().ImportDirectory(m_CurrentDirectory);
        RefreshAssetList();
    }

    ImGui::Separator();

    ImGui::BeginChild("AssetGrid", ImVec2{0, 0}, true);

    RenderAssetGrid();

    ImGui::EndChild();

    ImGui::End();
}

void AssetBrowserPanel::RefreshAssetList() {
    std::vector<std::string> sourceFiles;

    if (std::filesystem::exists(m_CurrentDirectory)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator{m_CurrentDirectory}) {
            if (!entry.is_regular_file()) continue;

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
                ext == ".wav" || ext == ".mp3" || ext == ".ogg") {

                std::string pxaPath = entry.path().string();
                pxaPath = pxaPath.substr(0, pxaPath.find_last_of('.')) + ".pxa";

                if (!std::filesystem::exists(pxaPath)) {
                    sourceFiles.push_back(entry.path().string());
                }
            }
        }
    }

    if (!sourceFiles.empty()) {
        AssetRegistry& registry = AssetRegistry::Instance();
        for (const auto& sourcePath : sourceFiles) {
            registry.LoadAssetFromPath(sourcePath);
        }
    }

    m_CurrentAssets = AssetRegistry::Instance().GetAllAssetMetadata();
}

void AssetBrowserPanel::RenderDirectoryTree() {
    if (!std::filesystem::exists(m_CurrentDirectory)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator{m_CurrentDirectory}) {
        if (entry.is_directory()) {
            if (ImGui::TreeNode(entry.path().filename().string().c_str())) {
                ImGui::TreePop();
            }
        }
    }
}

void AssetBrowserPanel::RenderAssetGrid() {
    float cellSize = 80.0f;
    float cellPadding = 10.0f;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(panelWidth / (cellSize + cellPadding));
    if (columnCount < 1) columnCount = 1;

    ImGui::Columns(columnCount, nullptr, false);

    for (const AssetMetadata& metadata : m_CurrentAssets) {
        RenderAssetItem(metadata);
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
}

void AssetBrowserPanel::RenderAssetItem(const AssetMetadata& metadata) {
    ImGui::PushID(static_cast<int>(metadata.uuid.Get()));

    bool isSelected = (m_SelectedAsset == metadata.uuid);
    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.3f, 0.5f, 0.8f, 1.0f});
    }

    ImVec2 buttonSize{70.0f, 70.0f};
    if (ImGui::Button("##asset", buttonSize)) {
        m_SelectedAsset = metadata.uuid;
        if (m_OnAssetSelected) {
            m_OnAssetSelected(metadata.uuid);
        }
    }

    if (ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("ASSET_UUID", &metadata.uuid, sizeof(UUID));
        ImGui::Text("%s", metadata.name.c_str());
        ImGui::EndDragDropSource();
    }

    if (isSelected) {
        ImGui::PopStyleColor();
    }

    ImGui::TextWrapped("%s", metadata.name.c_str());

    const char* typeStr = "Unknown";
    switch (metadata.type) {
        case AssetType::Texture: typeStr = "Texture"; break;
        case AssetType::Audio: typeStr = "Audio"; break;
        case AssetType::Scene: typeStr = "Scene"; break;
        default: break;
    }
    ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "%s", typeStr);

    ImGui::PopID();
}

} // namespace PiiXeL

#endif
