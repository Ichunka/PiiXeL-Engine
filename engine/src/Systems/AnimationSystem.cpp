#include "Systems/AnimationSystem.hpp"
#include "Components/Animator.hpp"
#include "Components/Sprite.hpp"
#include "Animation/AnimatorController.hpp"
#include "Animation/AnimationClip.hpp"
#include "Animation/SpriteSheet.hpp"
#include "Resources/AssetRegistry.hpp"
#include <raylib.h>

namespace PiiXeL {

void AnimationSystem::Update(entt::registry& registry, float deltaTime) {
    auto view = registry.view<Animator>();
    for (entt::entity entity : view) {
        UpdateAnimator(registry, entity, deltaTime);
    }
}

void AnimationSystem::UpdateAnimator(entt::registry& registry, entt::entity entity, float deltaTime) {
    Animator& animator = registry.get<Animator>(entity);

    if (!animator.isPlaying || animator.controllerUUID == 0) {
        return;
    }

    std::shared_ptr<Asset> controllerAsset = AssetRegistry::Instance().GetAsset(animator.controllerUUID);
    if (!controllerAsset) {
        return;
    }

    AnimatorController* controller = dynamic_cast<AnimatorController*>(controllerAsset.get());
    if (!controller) {
        return;
    }

    if (animator.currentState.empty()) {
        animator.currentState = controller->GetDefaultState();
        animator.stateTime = 0.0f;
        animator.currentFrameIndex = 0;
        animator.frameTime = 0.0f;
    }

    EvaluateTransitions(registry, entity);

    if (animator.isTransitioning) {
        animator.transitionTime += deltaTime;
        if (animator.transitionTime >= animator.transitionDuration) {
            animator.currentState = animator.transitionToState;
            animator.transitionToState.clear();
            animator.isTransitioning = false;
            animator.transitionTime = 0.0f;
            animator.stateTime = 0.0f;
            animator.currentFrameIndex = 0;
            animator.frameTime = 0.0f;
        }
    }

    UpdateAnimation(registry, entity, deltaTime * animator.playbackSpeed);

    animator.stateTime += deltaTime * animator.playbackSpeed;
}

void AnimationSystem::EvaluateTransitions(entt::registry& registry, entt::entity entity) {
    Animator& animator = registry.get<Animator>(entity);

    if (animator.isTransitioning || animator.controllerUUID == 0) {
        return;
    }

    std::shared_ptr<Asset> controllerAsset = AssetRegistry::Instance().GetAsset(animator.controllerUUID);
    if (!controllerAsset) {
        return;
    }

    AnimatorController* controller = dynamic_cast<AnimatorController*>(controllerAsset.get());
    if (!controller) {
        return;
    }

    std::vector<AnimatorTransition> transitions = controller->GetTransitionsFromState(animator.currentState);

    for (const AnimatorTransition& transition : transitions) {
        bool shouldTransition = true;

        if (transition.hasExitTime) {
            const AnimatorState* state = controller->GetState(animator.currentState);
            if (state && state->animationClipUUID != 0) {
                std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(state->animationClipUUID);
                AnimationClip* clip = dynamic_cast<AnimationClip*>(clipAsset.get());
                if (clip) {
                    float normalizedTime = animator.stateTime / clip->GetTotalDuration();
                    if (normalizedTime < transition.exitTime) {
                        shouldTransition = false;
                    }
                }
            }
        }

        if (shouldTransition) {
            for (const TransitionCondition& condition : transition.conditions) {
                auto paramIt = animator.parameters.find(condition.parameterName);
                if (paramIt == animator.parameters.end()) {
                    auto triggerIt = animator.triggers.find(condition.parameterName);
                    if (triggerIt != animator.triggers.end() && triggerIt->second) {
                        animator.triggers[condition.parameterName] = false;
                        continue;
                    }
                    shouldTransition = false;
                    break;
                }

                if (!EvaluateCondition(paramIt->second, condition.value, static_cast<int>(condition.type))) {
                    shouldTransition = false;
                    break;
                }
            }
        }

        if (shouldTransition) {
            animator.transitionToState = transition.toState;
            animator.transitionDuration = transition.transitionDuration;
            animator.transitionTime = 0.0f;
            animator.isTransitioning = true;
            break;
        }
    }
}

void AnimationSystem::UpdateAnimation(entt::registry& registry, entt::entity entity, float deltaTime) {
    Animator& animator = registry.get<Animator>(entity);

    if (animator.controllerUUID == 0) {
        return;
    }

    std::shared_ptr<Asset> controllerAsset = AssetRegistry::Instance().GetAsset(animator.controllerUUID);
    if (!controllerAsset) {
        return;
    }

    AnimatorController* controller = dynamic_cast<AnimatorController*>(controllerAsset.get());
    if (!controller) {
        return;
    }

    const AnimatorState* state = controller->GetState(animator.currentState);
    if (!state || state->animationClipUUID == 0) {
        return;
    }

    std::shared_ptr<Asset> clipAsset = AssetRegistry::Instance().GetAsset(state->animationClipUUID);
    if (!clipAsset) {
        return;
    }

    AnimationClip* clip = dynamic_cast<AnimationClip*>(clipAsset.get());
    if (!clip || clip->GetFrames().empty()) {
        return;
    }

    std::shared_ptr<Asset> sheetAsset = AssetRegistry::Instance().GetAsset(clip->GetSpriteSheetUUID());
    if (!sheetAsset) {
        return;
    }

    SpriteSheet* sheet = dynamic_cast<SpriteSheet*>(sheetAsset.get());
    if (!sheet) {
        return;
    }

    const std::vector<AnimationFrame>& frames = clip->GetFrames();
    const AnimationFrame& currentFrame = frames[animator.currentFrameIndex];

    animator.frameTime += deltaTime * state->speed;

    while (animator.frameTime >= currentFrame.duration) {
        animator.frameTime -= currentFrame.duration;

        if (clip->GetWrapMode() == AnimationWrapMode::Loop) {
            animator.currentFrameIndex = (animator.currentFrameIndex + 1) % frames.size();
        } else if (clip->GetWrapMode() == AnimationWrapMode::Once) {
            if (animator.currentFrameIndex < frames.size() - 1) {
                animator.currentFrameIndex++;
            } else {
                animator.frameTime = currentFrame.duration;
                break;
            }
        }
    }

    if (registry.all_of<Sprite>(entity)) {
        Sprite& sprite = registry.get<Sprite>(entity);
        const SpriteFrame* spriteFrame = sheet->GetFrame(frames[animator.currentFrameIndex].frameIndex);
        if (spriteFrame) {
            sprite.textureAssetUUID = sheet->GetTextureUUID();
            sprite.sourceRect = spriteFrame->sourceRect;
            sprite.origin = spriteFrame->pivot;
        }
    }
}

bool AnimationSystem::EvaluateCondition(const std::variant<float, int, bool>& paramValue,
                                        const std::variant<float, int, bool>& conditionValue,
                                        int conditionType) {
    if (std::holds_alternative<float>(paramValue) && std::holds_alternative<float>(conditionValue)) {
        float pVal = std::get<float>(paramValue);
        float cVal = std::get<float>(conditionValue);

        switch (conditionType) {
            case 0: return pVal > cVal;
            case 1: return pVal < cVal;
            case 2: return pVal == cVal;
            case 3: return pVal != cVal;
        }
    } else if (std::holds_alternative<int>(paramValue) && std::holds_alternative<int>(conditionValue)) {
        int pVal = std::get<int>(paramValue);
        int cVal = std::get<int>(conditionValue);

        switch (conditionType) {
            case 0: return pVal > cVal;
            case 1: return pVal < cVal;
            case 2: return pVal == cVal;
            case 3: return pVal != cVal;
        }
    } else if (std::holds_alternative<bool>(paramValue) && std::holds_alternative<bool>(conditionValue)) {
        bool pVal = std::get<bool>(paramValue);
        bool cVal = std::get<bool>(conditionValue);

        switch (conditionType) {
            case 2: return pVal == cVal;
            case 3: return pVal != cVal;
        }
    }

    return false;
}

} // namespace PiiXeL
