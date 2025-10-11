#include "Components/UUID.hpp"

namespace PiiXeL {

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

UUID::UUID()
    : m_UUID(s_UniformDistribution(s_Engine))
{
}

UUID::UUID(uint64_t uuid)
    : m_UUID(uuid)
{
}

std::string UUID::ToString() const {
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << m_UUID;
    return ss.str();
}

}
