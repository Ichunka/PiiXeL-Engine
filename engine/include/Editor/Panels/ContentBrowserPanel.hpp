#ifndef PIIXELENGINE_CONTENTBROWSERPANEL_HPP
#define PIIXELENGINE_CONTENTBROWSERPANEL_HPP

#ifdef BUILD_WITH_EDITOR

#include "EditorPanel.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Resources/AssetManager.hpp"
#include <string>
#include <vector>
#include <functional>
#include <entt/entt.hpp>

namespace PiiXeL {

class UUID;
class SpriteSheetEditorPanel;
class AnimationClipEditorPanel;
class AnimatorControllerEditorPanel;

class ContentBrowserPanel : public EditorPanel {
public:
    ContentBrowserPanel(
        UUID* selectedAssetUUID,
        std::string* selectedAssetPath,
        entt::entity* selectedEntity,
        SpriteSheetEditorPanel* spriteSheetEditor,
        AnimationClipEditorPanel* animationClipEditor,
        AnimatorControllerEditorPanel* animatorControllerEditor
    );

    void OnImGuiRender() override;
    const char* GetTitle() const override { return "Content Browser"; }
    bool IsOpen() const override { return m_IsOpen; }
    void SetOpen(bool open) override { m_IsOpen = open; }

    void SetDeleteAssetCallback(std::function<void(const std::string&)> callback);
    void SetLoadSceneCallback(std::function<void()> callback);

private:
    void RefreshDirectory();
    void RenderNavigationBar();
    void RenderDirectories();
    void RenderFiles();
    void HandleContextMenus();
    void HandlePopups();

private:
    UUID* m_SelectedAssetUUID;
    std::string* m_SelectedAssetPath;
    entt::entity* m_SelectedEntity;
    SpriteSheetEditorPanel* m_SpriteSheetEditor;
    AnimationClipEditorPanel* m_AnimationClipEditor;
    AnimatorControllerEditorPanel* m_AnimatorControllerEditor;

    bool m_IsOpen{true};

    std::vector<std::string> m_Directories;
    std::vector<AssetInfo> m_Files;
    std::string m_CurrentPath{"content"};
    bool m_NeedsRefresh{true};
    AssetInfo* m_DraggedAsset{nullptr};
    int m_ThumbnailSize{96};
    std::string m_RightClickedItem;
    bool m_IsRightClickFolder{false};
    bool m_ShowNewScenePopup{false};
    bool m_ShowNewFolderPopup{false};
    bool m_ShowRenamePopup{false};
    bool m_ShowNewSpriteSheetPopup{false};
    bool m_ShowNewAnimClipPopup{false};
    bool m_ShowNewAnimControllerPopup{false};
    char m_NewItemName[256]{};

    std::function<void(const std::string&)> m_DeleteAssetCallback;
    std::function<void()> m_LoadSceneCallback;
};

}

#endif

#endif
