#pragma once

#include "Resources/Asset.hpp"

#include <raylib.h>
#include <string>
#include <vector>

namespace PiiXeL {

struct SpriteFrame {
    Rectangle sourceRect{0.0f, 0.0f, 0.0f, 0.0f};
    Vector2 pivot{0.5f, 0.5f};
    std::string name;
};

struct FrameGroup {
    std::string name;
    std::vector<size_t> frameIndices;
};

class SpriteSheet : public Asset {
public:
    SpriteSheet(UUID uuid, const std::string& name);
    ~SpriteSheet() override = default;

    bool Load(const void* data, size_t size) override;
    void Unload() override;
    size_t GetMemoryUsage() const override;

    void SetTexture(UUID textureUUID);
    UUID GetTextureUUID() const { return m_TextureUUID; }

    void AddFrame(const SpriteFrame& frame);
    void SetFrames(const std::vector<SpriteFrame>& frames);
    const std::vector<SpriteFrame>& GetFrames() const { return m_Frames; }
    const SpriteFrame* GetFrame(size_t index) const;
    size_t GetFrameCount() const { return m_Frames.size(); }

    void SetGridSize(int columns, int rows) {
        m_GridColumns = columns;
        m_GridRows = rows;
    }
    int GetGridColumns() const { return m_GridColumns; }
    int GetGridRows() const { return m_GridRows; }

    void SetGridSpacing(int spacingX, int spacingY) {
        m_GridSpacingX = spacingX;
        m_GridSpacingY = spacingY;
    }
    int GetGridSpacingX() const { return m_GridSpacingX; }
    int GetGridSpacingY() const { return m_GridSpacingY; }

    void AddFrameGroup(const FrameGroup& group);
    void RemoveFrameGroup(size_t index);
    void UpdateFrameGroup(size_t index, const FrameGroup& group);
    void SetFrameGroups(const std::vector<FrameGroup>& groups);
    const std::vector<FrameGroup>& GetFrameGroups() const { return m_FrameGroups; }
    FrameGroup* GetFrameGroup(size_t index);
    const FrameGroup* GetFrameGroup(size_t index) const;
    size_t GetFrameGroupCount() const { return m_FrameGroups.size(); }

private:
    UUID m_TextureUUID{0};
    std::vector<SpriteFrame> m_Frames;
    std::vector<FrameGroup> m_FrameGroups;
    int m_GridColumns{1};
    int m_GridRows{1};
    int m_GridSpacingX{0};
    int m_GridSpacingY{0};
};

} // namespace PiiXeL
