#ifndef PIIXELENGINE_AUDIOSOURCEHANDLE_HPP
#define PIIXELENGINE_AUDIOSOURCEHANDLE_HPP

#include "Components/AudioSource.hpp"
#include "Components/UUID.hpp"

#include <entt/entt.hpp>

namespace PiiXeL {

class Scene;

class AudioSourceHandle {
public:
    AudioSourceHandle(Scene* scene, entt::entity entity, AudioSource* component);

    [[nodiscard]] bool IsValid() const;

    void Play();
    void Pause();
    void Stop();
    void PlayOneShot(UUID clipUuid);
    void PlayOneShot(UUID clipUuid, float volumeScale);

    void SetClip(UUID clipUuid);
    [[nodiscard]] UUID GetClip() const;

    void SetVolume(float volume);
    [[nodiscard]] float GetVolume() const;

    void SetPitch(float pitch);
    [[nodiscard]] float GetPitch() const;

    void SetLoop(bool loop);
    [[nodiscard]] bool IsLooping() const;

    void SetMute(bool mute);
    [[nodiscard]] bool IsMuted() const;

    void SetSpatialize(bool spatialize);
    [[nodiscard]] bool IsSpatial() const;

    void SetSpatialBlend(float blend);
    [[nodiscard]] float GetSpatialBlend() const;

    void SetMinDistance(float distance);
    [[nodiscard]] float GetMinDistance() const;

    void SetMaxDistance(float distance);
    [[nodiscard]] float GetMaxDistance() const;

    [[nodiscard]] bool IsPlaying() const;
    [[nodiscard]] bool IsPaused() const;
    [[nodiscard]] bool IsStopped() const;

private:
    Scene* m_Scene;
    entt::entity m_Entity;
    AudioSource* m_Component;

    void UpdateBackend();
};

} // namespace PiiXeL

#endif
