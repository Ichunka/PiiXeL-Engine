#ifdef BUILD_WITH_EDITOR

#include "Editor/AssetInspectorPanel.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/TextureAsset.hpp"
#include "Resources/AudioAsset.hpp"
#include <imgui.h>
#include <rlImGui.h>

namespace PiiXeL {

void AssetInspectorPanel::Render() {
    ImGui::Begin("Asset Inspector");

    if (!HasSelection()) {
        ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "No asset selected");
        ImGui::End();
        return;
    }

    std::shared_ptr<Asset> asset = AssetRegistry::Instance().GetAsset(m_SelectedAssetUUID);
    if (!asset) {
        ImGui::TextColored(ImVec4{1.0f, 0.3f, 0.3f, 1.0f}, "Asset not found");
        ImGui::End();
        return;
    }

    const AssetMetadata& metadata = asset->GetMetadata();

    ImGui::Text("Name: %s", metadata.name.c_str());
    ImGui::Separator();

    ImGui::Text("UUID: %llu", metadata.uuid.Get());
    ImGui::Text("Type: %d", static_cast<int>(metadata.type));
    ImGui::Text("Version: %u", metadata.version);
    ImGui::Text("Source: %s", metadata.sourceFile.c_str());

    ImGui::Separator();

    ImGui::Text("Memory: %zu bytes", asset->GetMemoryUsage());
    ImGui::Text("Loaded: %s", asset->IsLoaded() ? "Yes" : "No");

    ImGui::Separator();

    switch (metadata.type) {
        case AssetType::Texture:
            RenderTextureAsset(asset);
            break;
        case AssetType::Audio:
            RenderAudioAsset(asset);
            break;
        default:
            RenderGenericAsset(asset);
            break;
    }

    if (ImGui::Button("Reimport")) {
        AssetRegistry::Instance().ReimportAsset(metadata.sourceFile);
    }

    ImGui::End();
}

void AssetInspectorPanel::SetSelectedAsset(UUID assetUUID) {
    m_SelectedAssetUUID = assetUUID;
}

void AssetInspectorPanel::ClearSelection() {
    m_SelectedAssetUUID = UUID{0};
}

void AssetInspectorPanel::RenderTextureAsset(std::shared_ptr<Asset> asset) {
    TextureAsset* texAsset = dynamic_cast<TextureAsset*>(asset.get());
    if (!texAsset) return;

    ImGui::Text("Dimensions: %dx%d", texAsset->GetWidth(), texAsset->GetHeight());
    ImGui::Text("Mipmaps: %d", texAsset->GetMipmaps());
    ImGui::Text("Format: %d", texAsset->GetFormat());

    ImGui::Separator();
    ImGui::Text("Preview:");

    Texture2D texture = texAsset->GetTexture();
    if (texture.id != 0) {
        float maxWidth = ImGui::GetContentRegionAvail().x - 20.0f;
        float maxHeight = 256.0f;

        float aspectRatio = static_cast<float>(texture.width) / static_cast<float>(texture.height);
        float displayWidth = maxWidth;
        float displayHeight = displayWidth / aspectRatio;

        if (displayHeight > maxHeight) {
            displayHeight = maxHeight;
            displayWidth = displayHeight * aspectRatio;
        }

        rlImGuiImageSize(&texture, static_cast<int>(displayWidth), static_cast<int>(displayHeight));
    }
}

void AssetInspectorPanel::RenderAudioAsset(std::shared_ptr<Asset> asset) {
    AudioAsset* audioAsset = dynamic_cast<AudioAsset*>(asset.get());
    if (!audioAsset) return;

    ImGui::Text("Frames: %u", audioAsset->GetFrameCount());

    ImGui::Separator();

    Sound sound = audioAsset->GetSound();
    if (sound.frameCount > 0) {
        if (ImGui::Button("Play")) {
            PlaySound(sound);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            StopSound(sound);
        }
    }
}

void AssetInspectorPanel::RenderGenericAsset(std::shared_ptr<Asset>) {
    ImGui::TextColored(ImVec4{0.6f, 0.6f, 0.6f, 1.0f}, "No preview available");
}

} // namespace PiiXeL

#endif
