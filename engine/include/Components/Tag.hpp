#ifndef PIIXELENGINE_TAG_HPP
#define PIIXELENGINE_TAG_HPP

#include <string>

namespace PiiXeL {

struct Tag {
    std::string name{"Entity"};

    Tag() = default;
    explicit Tag(const std::string& n) : name{n} {}
};

} // namespace PiiXeL

#endif // PIIXELENGINE_TAG_HPP
