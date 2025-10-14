#include "Animation/AnimatorAPI.hpp"

#include <raylib.h>

namespace PiiXeL {

void AnimatorAPI::Play(entt::registry& registry, entt::entity entity) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.isPlaying = true;
}

void AnimatorAPI::Stop(entt::registry& registry, entt::entity entity) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.isPlaying = false;
    animator.stateTime = 0.0f;
    animator.currentFrameIndex = 0;
    animator.frameTime = 0.0f;
}

void AnimatorAPI::Pause(entt::registry& registry, entt::entity entity) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.isPlaying = false;
}

void AnimatorAPI::SetFloat(entt::registry& registry, entt::entity entity, const std::string& paramName, float value) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.parameters[paramName] = value;
}

void AnimatorAPI::SetInt(entt::registry& registry, entt::entity entity, const std::string& paramName, int value) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.parameters[paramName] = value;
}

void AnimatorAPI::SetBool(entt::registry& registry, entt::entity entity, const std::string& paramName, bool value) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.parameters[paramName] = value;
}

void AnimatorAPI::SetTrigger(entt::registry& registry, entt::entity entity, const std::string& paramName) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.triggers[paramName] = true;
}

float AnimatorAPI::GetFloat(entt::registry& registry, entt::entity entity, const std::string& paramName) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return 0.0f; }

    Animator& animator = registry.get<Animator>(entity);
    auto it = animator.parameters.find(paramName);
    if (it != animator.parameters.end() && std::holds_alternative<float>(it->second))
    { return std::get<float>(it->second); }
    return 0.0f;
}

int AnimatorAPI::GetInt(entt::registry& registry, entt::entity entity, const std::string& paramName) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return 0; }

    Animator& animator = registry.get<Animator>(entity);
    auto it = animator.parameters.find(paramName);
    if (it != animator.parameters.end() && std::holds_alternative<int>(it->second))
    { return std::get<int>(it->second); }
    return 0;
}

bool AnimatorAPI::GetBool(entt::registry& registry, entt::entity entity, const std::string& paramName) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return false; }

    Animator& animator = registry.get<Animator>(entity);
    auto it = animator.parameters.find(paramName);
    if (it != animator.parameters.end() && std::holds_alternative<bool>(it->second))
    { return std::get<bool>(it->second); }
    return false;
}

void AnimatorAPI::SetPlaybackSpeed(entt::registry& registry, entt::entity entity, float speed) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return; }

    Animator& animator = registry.get<Animator>(entity);
    animator.playbackSpeed = speed;
}

bool AnimatorAPI::IsPlaying(entt::registry& registry, entt::entity entity) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return false; }

    const Animator& animator = registry.get<Animator>(entity);
    return animator.isPlaying;
}

std::string AnimatorAPI::GetCurrentState(entt::registry& registry, entt::entity entity) {
    if (!registry.valid(entity) || !registry.all_of<Animator>(entity))
    { return ""; }

    const Animator& animator = registry.get<Animator>(entity);
    return animator.currentState;
}

} // namespace PiiXeL
