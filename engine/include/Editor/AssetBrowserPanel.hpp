#ifndef PIIXELENGINE_ASSETBROWSERPANEL_HPP
#define PIIXELENGINE_ASSETBROWSERPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "Components/UUID.hpp"
#include "Resources/Asset.hpp"
#include <vector>
#include <string>
#include <functional>

namespace PiiXeL {

class AssetBrowserPanel {
public:
    using AssetSelectedCallback = std::function<void(UUID)>;

    AssetBrowserPanel() = default;
    ~AssetBrowserPanel() = default;

    void Render();
    void RefreshAssetList();

    void SetAssetSelectedCallback(AssetSelectedCallback callback) {
        m_OnAssetSelected = callback;
    }

    void SetCurrentDirectory(const std::string& directory) {
        m_CurrentDirectory = directory;
        RefreshAssetList();
    }

    [[nodiscard]] const std::string& GetCurrentDirectory() const {
        return m_CurrentDirectory;
    }

private:
    void RenderDirectoryTree();
    void RenderAssetGrid();
    void RenderAssetItem(const AssetMetadata& metadata);

    std::string m_CurrentDirectory{"assets"};
    std::vector<AssetMetadata> m_CurrentAssets;
    AssetSelectedCallback m_OnAssetSelected;
    UUID m_SelectedAsset{0};
};

} // namespace PiiXeL

#endif

#endif
