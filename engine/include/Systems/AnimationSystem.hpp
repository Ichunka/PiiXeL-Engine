#pragma once

#include <entt/entt.hpp>
#include <variant>

namespace PiiXeL {

class AnimationSystem {
public:
    static void Update(entt::registry& registry, float deltaTime);
    static void ResetAnimators(entt::registry& registry);

private:
    static void UpdateAnimator(entt::registry& registry, entt::entity entity, float deltaTime);
    static void EvaluateTransitions(entt::registry& registry, entt::entity entity);
    static void UpdateAnimation(entt::registry& registry, entt::entity entity, float deltaTime);
    static bool EvaluateCondition(const std::variant<float, int, bool>& paramValue,
                                  const std::variant<float, int, bool>& conditionValue,
                                  int conditionType);
};

} // namespace PiiXeL
