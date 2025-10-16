#ifndef PIIXELENGINE_AUDIOSYSTEM_HPP
#define PIIXELENGINE_AUDIOSYSTEM_HPP

#include <entt/entt.hpp>

#include <raylib.h>

namespace PiiXeL {

class Scene;

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    void Initialize();
    void Shutdown();
    void Update(float deltaTime, entt::registry& registry);

    void SetScene(Scene* scene) { m_Scene = scene; }

    void PlaySource(entt::registry& registry, entt::entity entity);
    void PauseSource(entt::registry& registry, entt::entity entity);
    void StopSource(entt::registry& registry, entt::entity entity);
    void StopAllSources(entt::registry& registry);

    static void ResetAudioSources(entt::registry& registry);

    void SetMasterVolume(float volume);
    [[nodiscard]] float GetMasterVolume() const { return m_MasterVolume; }

private:
    void UpdateSources(entt::registry& registry);
    void UpdateListener(entt::registry& registry);
    void UpdateSpatialAudio(entt::registry& registry);

private:
    Scene* m_Scene{nullptr};
    entt::entity m_ListenerEntity{entt::null};
    float m_MasterVolume{1.0f};
    bool m_IsInitialized{false};
};

} // namespace PiiXeL

#endif
