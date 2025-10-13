#pragma once

#include "Components/Animator.hpp"
#include <entt/entt.hpp>
#include <string>

namespace PiiXeL {

class AnimatorAPI {
public:
    static void Play(entt::registry& registry, entt::entity entity);
    static void Stop(entt::registry& registry, entt::entity entity);
    static void Pause(entt::registry& registry, entt::entity entity);

    static void SetFloat(entt::registry& registry, entt::entity entity, const std::string& paramName, float value);
    static void SetInt(entt::registry& registry, entt::entity entity, const std::string& paramName, int value);
    static void SetBool(entt::registry& registry, entt::entity entity, const std::string& paramName, bool value);
    static void SetTrigger(entt::registry& registry, entt::entity entity, const std::string& paramName);

    static float GetFloat(entt::registry& registry, entt::entity entity, const std::string& paramName);
    static int GetInt(entt::registry& registry, entt::entity entity, const std::string& paramName);
    static bool GetBool(entt::registry& registry, entt::entity entity, const std::string& paramName);

    static void SetPlaybackSpeed(entt::registry& registry, entt::entity entity, float speed);
    static bool IsPlaying(entt::registry& registry, entt::entity entity);

    static std::string GetCurrentState(entt::registry& registry, entt::entity entity);
};

} // namespace PiiXeL
