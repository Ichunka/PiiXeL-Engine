#include "Resources/Asset.hpp"

namespace PiiXeL {

Asset::Asset(UUID uuid, AssetType type, const std::string& name) {
    m_Metadata.uuid = uuid;
    m_Metadata.type = type;
    m_Metadata.name = name;
}

} // namespace PiiXeL
