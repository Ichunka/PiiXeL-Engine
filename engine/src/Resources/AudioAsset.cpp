#include "Resources/AudioAsset.hpp"

#include "Core/Logger.hpp"

#include <cstring>
#include <fstream>

namespace PiiXeL {

AudioAsset::AudioAsset(UUID uuid, const std::string& name) : Asset{uuid, AssetType::Audio, name} {}

AudioAsset::~AudioAsset() {
    Unload();
}

bool AudioAsset::Load(const void* data, size_t size) {
    if (m_IsLoaded) {
        Unload();
    }

    const char* fileExt = ".wav";
    m_Wave = LoadWaveFromMemory(fileExt, static_cast<const unsigned char*>(data), static_cast<int>(size));

    if (m_Wave.data == nullptr) {
        PX_LOG_ERROR(ASSET, "Failed to load audio from memory");
        return false;
    }

    m_Sound = LoadSoundFromWave(m_Wave);

    if (m_Sound.frameCount == 0) {
        PX_LOG_ERROR(ASSET, "Failed to create sound from wave");
        UnloadWave(m_Wave);
        return false;
    }

    m_IsLoaded = true;
    PX_LOG_INFO(ASSET, "Audio asset loaded: %s (%u frames)", m_Metadata.name.c_str(), m_Sound.frameCount);
    return true;
}

void AudioAsset::Unload() {
    if (m_IsLoaded) {
        if (m_Sound.frameCount > 0) {
            UnloadSound(m_Sound);
            m_Sound = Sound{};
        }
        if (m_Wave.data != nullptr) {
            UnloadWave(m_Wave);
            m_Wave = Wave{};
        }
        m_IsLoaded = false;
    }
}

size_t AudioAsset::GetMemoryUsage() const {
    if (!m_IsLoaded)
        return 0;
    return m_Wave.frameCount * m_Wave.channels * (m_Wave.sampleSize / 8);
}

std::vector<uint8_t> AudioAsset::EncodeToMemory(const std::string& sourcePath) {
    std::ifstream file{sourcePath, std::ios::binary | std::ios::ate};
    if (!file.is_open()) {
        PX_LOG_ERROR(ASSET, "Failed to open audio file: %s", sourcePath.c_str());
        return {};
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        PX_LOG_ERROR(ASSET, "Failed to read audio file: %s", sourcePath.c_str());
        return {};
    }

    return buffer;
}

} // namespace PiiXeL
