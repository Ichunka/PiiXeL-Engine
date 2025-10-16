#include "Scripting/AudioSourceHandle.hpp"

#include "Components/AudioSource.hpp"
#include "Core/Engine.hpp"
#include "Scene/Scene.hpp"
#include "Systems/AudioSystem.hpp"

namespace PiiXeL {

AudioSourceHandle::AudioSourceHandle(Scene* scene, entt::entity entity, AudioSource* component) :
    m_Scene{scene}, m_Entity{entity}, m_Component{component} {}

bool AudioSourceHandle::IsValid() const {
    if (!m_Scene || m_Entity == entt::null || !m_Component) {
        return false;
    }

    entt::registry& registry = m_Scene->GetRegistry();
    return registry.valid(m_Entity) && registry.all_of<AudioSource>(m_Entity);
}

void AudioSourceHandle::Play() {
    if (!IsValid()) return;

    m_Component->state = AudioSourceState::Playing;
    m_Component->hasPlayedOnAwake = false;
}

void AudioSourceHandle::Pause() {
    if (!IsValid()) return;

    m_Component->state = AudioSourceState::Paused;
}

void AudioSourceHandle::Stop() {
    if (!IsValid()) return;

    m_Component->state = AudioSourceState::Stopped;
}

void AudioSourceHandle::PlayOneShot(UUID clipUuid) {
    PlayOneShot(clipUuid, 1.0f);
}

void AudioSourceHandle::PlayOneShot(UUID clipUuid, float volumeScale) {
    if (!IsValid()) return;

    UUID originalClip = m_Component->audioClip;
    bool originalLoop = m_Component->loop;
    float originalVolume = m_Component->volume;

    m_Component->audioClip = clipUuid;
    m_Component->loop = false;
    m_Component->volume = originalVolume * volumeScale;
    m_Component->state = AudioSourceState::Playing;
    m_Component->hasPlayedOnAwake = false;

    m_Component->audioClip = originalClip;
    m_Component->loop = originalLoop;
    m_Component->volume = originalVolume;
}

void AudioSourceHandle::SetClip(UUID clipUuid) {
    if (IsValid()) {
        m_Component->audioClip = clipUuid;
    }
}

UUID AudioSourceHandle::GetClip() const {
    if (IsValid()) {
        return m_Component->audioClip;
    }
    return UUID{0};
}

void AudioSourceHandle::SetVolume(float volume) {
    if (IsValid()) {
        m_Component->volume = volume;
    }
}

float AudioSourceHandle::GetVolume() const {
    if (IsValid()) {
        return m_Component->volume;
    }
    return 1.0f;
}

void AudioSourceHandle::SetPitch(float pitch) {
    if (IsValid()) {
        m_Component->pitch = pitch;
    }
}

float AudioSourceHandle::GetPitch() const {
    if (IsValid()) {
        return m_Component->pitch;
    }
    return 1.0f;
}

void AudioSourceHandle::SetLoop(bool loop) {
    if (IsValid()) {
        m_Component->loop = loop;
    }
}

bool AudioSourceHandle::IsLooping() const {
    if (IsValid()) {
        return m_Component->loop;
    }
    return false;
}

void AudioSourceHandle::SetMute(bool mute) {
    if (IsValid()) {
        m_Component->mute = mute;
    }
}

bool AudioSourceHandle::IsMuted() const {
    if (IsValid()) {
        return m_Component->mute;
    }
    return false;
}

void AudioSourceHandle::SetSpatialize(bool spatialize) {
    if (IsValid()) {
        m_Component->spatialize = spatialize;
    }
}

bool AudioSourceHandle::IsSpatial() const {
    if (IsValid()) {
        return m_Component->spatialize;
    }
    return false;
}

void AudioSourceHandle::SetSpatialBlend(float blend) {
    if (IsValid()) {
        m_Component->spatialBlend = blend;
    }
}

float AudioSourceHandle::GetSpatialBlend() const {
    if (IsValid()) {
        return m_Component->spatialBlend;
    }
    return 1.0f;
}

void AudioSourceHandle::SetMinDistance(float distance) {
    if (IsValid()) {
        m_Component->minDistance = distance;
    }
}

float AudioSourceHandle::GetMinDistance() const {
    if (IsValid()) {
        return m_Component->minDistance;
    }
    return 1.0f;
}

void AudioSourceHandle::SetMaxDistance(float distance) {
    if (IsValid()) {
        m_Component->maxDistance = distance;
    }
}

float AudioSourceHandle::GetMaxDistance() const {
    if (IsValid()) {
        return m_Component->maxDistance;
    }
    return 500.0f;
}

bool AudioSourceHandle::IsPlaying() const {
    if (IsValid()) {
        return m_Component->state == AudioSourceState::Playing;
    }
    return false;
}

bool AudioSourceHandle::IsPaused() const {
    if (IsValid()) {
        return m_Component->state == AudioSourceState::Paused;
    }
    return false;
}

bool AudioSourceHandle::IsStopped() const {
    if (IsValid()) {
        return m_Component->state == AudioSourceState::Stopped;
    }
    return false;
}

void AudioSourceHandle::UpdateBackend() {
}

} // namespace PiiXeL
