#ifndef PIIXELENGINE_ASSETINSPECTORPANEL_HPP
#define PIIXELENGINE_ASSETINSPECTORPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "Components/UUID.hpp"
#include "Resources/Asset.hpp"
#include <memory>
#include <string>

namespace PiiXeL {

class AssetInspectorPanel {
public:
    AssetInspectorPanel() = default;
    ~AssetInspectorPanel() = default;

    void Render();
    void SetSelectedAsset(UUID assetUUID);
    void ClearSelection();

    [[nodiscard]] bool HasSelection() const { return m_SelectedAssetUUID.Get() != 0; }
    [[nodiscard]] UUID GetSelectedAsset() const { return m_SelectedAssetUUID; }

private:
    void RenderTextureAsset(std::shared_ptr<Asset> asset);
    void RenderAudioAsset(std::shared_ptr<Asset> asset);
    void RenderGenericAsset(std::shared_ptr<Asset> asset);

    UUID m_SelectedAssetUUID{0};
};

} // namespace PiiXeL

#endif

#endif
