#pragma once

#include <glm/glm.hpp>

#include "error.h"
#include "jsonparser.h"

template<>
Error<glm::vec2> JSON::valueTo(const JSON::Value& value) noexcept;

template<>
Error<glm::vec3> JSON::valueTo(const JSON::Value& value) noexcept;

namespace mars {
    template<class T> requires std::is_arithmetic<T>::value
    static consteval T bitWidth() {
        T count = 0;
        while ((std::numeric_limits<T>::max() ^ (static_cast<T>(1U) << count)) > (static_cast<T>(1U) << count)) count++;
        return count + 1;
    }

    struct Rectangle {
        glm::vec2 position;
        glm::vec2 scale;
    };
}
