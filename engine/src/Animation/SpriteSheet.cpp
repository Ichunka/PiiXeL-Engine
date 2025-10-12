#include "Animation/SpriteSheet.hpp"
#include <nlohmann/json.hpp>
#include <raylib.h>

namespace PiiXeL {

SpriteSheet::SpriteSheet(UUID uuid, const std::string& name)
    : Asset(uuid, AssetType::SpriteSheet, name) {
}

bool SpriteSheet::Load(const void* data, size_t size) {
    if (!data || size == 0) {
        TraceLog(LOG_ERROR, "Invalid data for SpriteSheet");
        return false;
    }

    try {
        std::string jsonStr{reinterpret_cast<const char*>(data), size};
        nlohmann::json json = nlohmann::json::parse(jsonStr);

        if (json.contains("textureUUID")) {
            m_TextureUUID = UUID{json["textureUUID"].get<uint64_t>()};
        }

        if (json.contains("gridColumns") && json.contains("gridRows")) {
            m_GridColumns = json["gridColumns"].get<int>();
            m_GridRows = json["gridRows"].get<int>();
        }

        if (json.contains("frames") && json["frames"].is_array()) {
            m_Frames.clear();
            for (const auto& frameJson : json["frames"]) {
                SpriteFrame frame{};
                frame.name = frameJson.value("name", "");

                if (frameJson.contains("sourceRect") && frameJson["sourceRect"].size() == 4) {
                    frame.sourceRect.x = frameJson["sourceRect"][0].get<float>();
                    frame.sourceRect.y = frameJson["sourceRect"][1].get<float>();
                    frame.sourceRect.width = frameJson["sourceRect"][2].get<float>();
                    frame.sourceRect.height = frameJson["sourceRect"][3].get<float>();
                }

                if (frameJson.contains("pivot") && frameJson["pivot"].size() == 2) {
                    frame.pivot.x = frameJson["pivot"][0].get<float>();
                    frame.pivot.y = frameJson["pivot"][1].get<float>();
                }

                m_Frames.push_back(frame);
            }
        }

        m_IsLoaded = true;
        return true;
    } catch (const nlohmann::json::exception& e) {
        TraceLog(LOG_ERROR, "Failed to parse SpriteSheet JSON: %s", e.what());
        return false;
    }
}

void SpriteSheet::Unload() {
    m_Frames.clear();
    m_TextureUUID = UUID{0};
    m_IsLoaded = false;
}

size_t SpriteSheet::GetMemoryUsage() const {
    size_t total = sizeof(SpriteSheet);
    total += m_Frames.capacity() * sizeof(SpriteFrame);
    for (const SpriteFrame& frame : m_Frames) {
        total += frame.name.capacity();
    }
    return total;
}

void SpriteSheet::SetTexture(UUID textureUUID) {
    m_TextureUUID = textureUUID;
}

void SpriteSheet::AddFrame(const SpriteFrame& frame) {
    m_Frames.push_back(frame);
}

void SpriteSheet::SetFrames(const std::vector<SpriteFrame>& frames) {
    m_Frames = frames;
}

const SpriteFrame* SpriteSheet::GetFrame(size_t index) const {
    if (index >= m_Frames.size()) {
        return nullptr;
    }
    return &m_Frames[index];
}

} // namespace PiiXeL
