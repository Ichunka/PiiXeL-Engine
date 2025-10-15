#ifdef BUILD_WITH_EDITOR

#include "Editor/EditorAnimatorPreviewSystem.hpp"

#include "Animation/AnimationClip.hpp"
#include "Animation/AnimatorController.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Components/Animator.hpp"
#include "Components/Sprite.hpp"
#include "Core/Engine.hpp"
#include "Resources/AssetRegistry.hpp"
#include "Scene/Scene.hpp"

#include <memory>

namespace PiiXeL {

void EditorAnimatorPreviewSystem::UpdateAnimatorPreviewInEditMode(Engine* engine) {
    if (!engine || !engine->GetActiveScene()) {
        return;
    }

    Scene* scene = engine->GetActiveScene();
    entt::registry& registry = scene->GetRegistry();

    auto view = registry.view<Animator, Sprite>();
    for (entt::entity entity : view) {
        Animator& animator = view.get<Animator>(entity);
        Sprite& sprite = view.get<Sprite>(entity);

        if (animator.controllerUUID.Get() == 0) {
            continue;
        }

        std::shared_ptr<Asset> controllerAsset = AssetRegistry::Instance().GetAsset(animator.controllerUUID);
        AnimatorController* controller = dynamic_cast<AnimatorController*>(controllerAsset.get());
        if (!controller) {
            continue;
        }

        std::string defaultStateName = controller->GetDefaultState();
        const AnimatorState* defaultState = controller->GetState(defaultStateName);
        if (!defaultState || defaultState->animationClipUUID.Get() == 0) {
            continue;
        }

        std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(defaultState->animationClipUUID);
        AnimationClip* clip = dynamic_cast<AnimationClip*>(clipAsset.get());
        if (!clip || clip->GetFrames().empty()) {
            continue;
        }

        std::shared_ptr<Asset> sheetAsset = AssetRegistry::Instance().GetAsset(clip->GetSpriteSheetUUID());
        SpriteSheet* sheet = dynamic_cast<SpriteSheet*>(sheetAsset.get());
        if (!sheet) {
            continue;
        }

        const AnimationFrame& firstFrame = clip->GetFrames()[0];
        const SpriteFrame* spriteFrame = sheet->GetFrame(firstFrame.frameIndex);
        if (spriteFrame) {
            sprite.textureAssetUUID = sheet->GetTextureUUID();
            sprite.sourceRect = spriteFrame->sourceRect;
            sprite.origin = spriteFrame->pivot;
        }
    }
}

} // namespace PiiXeL

#endif
