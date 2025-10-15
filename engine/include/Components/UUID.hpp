#ifndef PIIXELENGINE_UUID_HPP
#define PIIXELENGINE_UUID_HPP

#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace PiiXeL {

class UUID {
public:
    UUID();
    explicit UUID(uint64_t uuid);

    operator uint64_t() const { return m_UUID; }
    [[nodiscard]] uint64_t Get() const { return m_UUID; }
    [[nodiscard]] std::string ToString() const;

    bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
    bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }
    bool operator<(const UUID& other) const { return m_UUID < other.m_UUID; }

private:
    uint64_t m_UUID;
};

} // namespace PiiXeL

namespace std {
template <>
struct hash<PiiXeL::UUID> {
    size_t operator()(const PiiXeL::UUID& uuid) const { return hash<uint64_t>()(uuid.Get()); }
};
} // namespace std

#endif
