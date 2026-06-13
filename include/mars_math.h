#pragma once

#include <glm/glm.hpp>

#include "error.h"
#include "jsonparser.h"

template<>
Error<glm::vec2> JSON::valueTo(const JSON::Value& value) noexcept;

template<>
Error<glm::vec3> JSON::valueTo(const JSON::Value& value) noexcept;
