#include "mars_camera.h"

#include <format>

template<>
Error<mars::Camera> JSON::valueTo(const JSON::Value& value) noexcept {
    mars::CameraBuilder cb{};
    if (value.getType() != JSON::Type::jobject) {
        FATAL(std::format("Expected a JSON object, got {}", JSON::typeToString(value.getType())));
    }
    const JSON::Object& camData = value.getObject().value();
    if (camData.contains("position")) {
        TRY_INIT(glm::vec3, pos, JSON::valueTo<glm::vec3>(camData.at("position")));
        cb.setPos(pos);
    }
    if (camData.contains("direction")) {
        TRY_INIT(glm::vec3, dir, JSON::valueTo<glm::vec3>(camData.at("direction")));
        cb.setDir(dir);
    }
    if (camData.contains("upVector")) {
        TRY_INIT(glm::vec3, up, JSON::valueTo<glm::vec3>(camData.at("upVector")));
        cb.setUp(up);
    }
    if (camData.contains("fov")) {
        TRY_INIT(float, fov, JSON::valueTo<float>(camData.at("fov")));
        cb.setFov(fov);
    }
    if (camData.contains("aspect")) {
        TRY_INIT(float, aspect, JSON::valueTo<float>(camData.at("aspect")));
        cb.setAspect(aspect);
    }
    if (camData.contains("sensitivity")) {
        TRY_INIT(float, sensitivity, JSON::valueTo<float>(camData.at("sensitivity")));
        cb.setSensitivity(sensitivity);
    }
    if (camData.contains("maxY")) {
        TRY_INIT(float, maxY, JSON::valueTo<float>(camData.at("maxY")));
        cb.setMaxY(maxY);
    }
    if (camData.contains("nearPlane")) {
        TRY_INIT(float, near, JSON::valueTo<float>(camData.at("nearPlane")));
        cb.setNearPlane(near);
    }
    if (camData.contains("farPlane")) {
        TRY_INIT(float, far, JSON::valueTo<float>(camData.at("farPlane")));
        cb.setFarPlane(far);
    }
    return cb.build();
}