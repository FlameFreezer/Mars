#include "mars_math.h"

#include <format>

template<>
Error<glm::vec2> JSON::valueTo(const JSON::Value& value) noexcept {
    switch (value.getType()) {
    case JSON::Type::jarray: {
		const JSON::Array& vector = value.getArray().value();
		glm::vec2 result{};
		TRY_ASSIGN(result.x, vector[0].getNumberAs<float>());
		TRY_ASSIGN(result.y, vector[1].getNumberAs<float>());
		return result;
    }
    case JSON::Type::jobject: {
        const JSON::Object& vector = value.getObject().value();
        glm::vec2 result{};
        TRY_ASSIGN(result.x, vector.at("x").getNumberAs<float>());
        TRY_ASSIGN(result.y, vector.at("y").getNumberAs<float>());
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
		TRY_ASSIGN(result.x, vector[0].getNumberAs<float>());
		TRY_ASSIGN(result.y, vector[1].getNumberAs<float>());
		TRY_ASSIGN(result.z, vector[2].getNumberAs<float>());
		return result;
    }
    case JSON::Type::jobject: {
        const JSON::Object& vector = value.getObject().value();
        glm::vec3 result{};
        TRY_ASSIGN(result.x, vector.at("x").getNumberAs<float>());
        TRY_ASSIGN(result.y, vector.at("y").getNumberAs<float>());
        TRY_ASSIGN(result.z, vector.at("z").getNumberAs<float>());
        return result;
    }
    default: 
        FATAL(std::format("Expected a JSON array of numbers or a JSON object, got {}", JSON::typeToString(value.getType())));
    }
}
