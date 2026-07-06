#include "mars_math.h"

#include <format>

template<>
Error<glm::vec2> JSON::valueTo(const JSON::Value& value) noexcept {
    switch (value.getType()) {
    case JSON::Type::jarray: {
		const JSON::Array& vector = value.getArray().value();
		glm::vec2 result{};
        TRY_ASSIGN(result.x, JSON::valueTo<float>(vector[0]));
        TRY_ASSIGN(result.y, JSON::valueTo<float>(vector[1]));
		return result;
    }
    case JSON::Type::jobject: {
        const JSON::Object& vector = value.getObject().value();
        glm::vec2 result{};
        TRY_ASSIGN(result.x, JSON::valueTo<float>(vector.at("x")));
        TRY_ASSIGN(result.y, JSON::valueTo<float>(vector.at("y")));
        return result;
    }
    default: 
        FATAL(std::format("Expected a JSON array of numbers or a JSON object, got {}", JSON::typeToString(value.getType())));
    }
}

template<>
Error<glm::vec3> JSON::valueTo(const JSON::Value& value) noexcept {
    switch (value.getType()) {
    case JSON::Type::jarray: {
		const JSON::Array& vector = value.getArray().value();
		glm::vec3 result{};
        TRY_ASSIGN(result.x, JSON::valueTo<float>(vector[0]));
        TRY_ASSIGN(result.y, JSON::valueTo<float>(vector[1]));
        TRY_ASSIGN(result.z, JSON::valueTo<float>(vector[2]));
		return result;
    }
    case JSON::Type::jobject: {
        const JSON::Object& vector = value.getObject().value();
        glm::vec3 result{};
        TRY_ASSIGN(result.x, JSON::valueTo<float>(vector.at("x")));
        TRY_ASSIGN(result.y, JSON::valueTo<float>(vector.at("y")));
        TRY_ASSIGN(result.z, JSON::valueTo<float>(vector.at("z")));
        return result;
    }
    default: 
        FATAL(std::format("Expected a JSON array of numbers or a JSON object, got {}", JSON::typeToString(value.getType())));
    }
}
