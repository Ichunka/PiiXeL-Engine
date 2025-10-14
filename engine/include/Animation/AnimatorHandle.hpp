#ifndef PIIXELENGINE_ANIMATORHANDLE_HPP
#define PIIXELENGINE_ANIMATORHANDLE_HPP

#include <entt/entt.hpp>

#include <string>
#include <variant>

namespace PiiXeL {

class Scene;
struct Animator;

class AnimatorHandle {
public:
    AnimatorHandle(Scene* scene, entt::entity entity, Animator* component);

    [[nodiscard]] bool IsValid() const;

    void Play();
    void Pause();
    void Stop();

    void SetSpeed(float speed);
    [[nodiscard]] float GetSpeed() const;

    void SetFloat(const std::string& name, float value);
    void SetInt(const std::string& name, int value);
    void SetBool(const std::string& name, bool value);
    void SetTrigger(const std::string& name);

    [[nodiscard]] float GetFloat(const std::string& name) const;
    [[nodiscard]] int GetInt(const std::string& name) const;
    [[nodiscard]] bool GetBool(const std::string& name) const;

    [[nodiscard]] std::string GetCurrentState() const;
    [[nodiscard]] bool IsPlaying() const;
    [[nodiscard]] float GetStateTime() const;

private:
    Scene* m_Scene;
    entt::entity m_Entity;
    Animator* m_Component;
};

} // namespace PiiXeL

#endif
