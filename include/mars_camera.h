#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "error.h"
#include "jsonparser.h"
#include "mars_math.h"

namespace mars {
    struct Camera {
        static constexpr float autoAspect = 0.0f;
        glm::vec3 pos = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 dir = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
        float fov = 45.0f;
        float aspect = autoAspect;
        float maxY = 0.9f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        float sensitivity = 0.001f;
        glm::mat4 getMatrix() const noexcept {
            //dir + pos = target (position the camera is looking at)
            const glm::mat4 view = glm::lookAt(pos, dir + pos, up);
            glm::mat4 proj = glm::perspective(fov, aspect, nearPlane, farPlane);
            proj[1][1] *= -1.0f;
            return proj * view;
        }
        void rotate(float deltaX, float deltaY) noexcept {
            glm::vec3 d(dir);

            if(d.y >= maxY and deltaY > 0.0f) deltaY = 0.0f;
            else if(d.y <= -maxY and deltaY < 0.0f) deltaY = 0.0f;

            const float deltaYaw = deltaX * sensitivity;
            const float deltaPitch = deltaY * sensitivity;

            //This gives the angle of dir with the xz-plane
            float pitch = glm::asin(d.y);
            //cos(pitch) gives the length of the projection of dir onto the xz-plane
            //so, dir.z / cos(pitch) gives the cosine of the angle between dir and the z-axis
            float yaw = glm::acos(d.z / glm::cos(pitch));

            //Since -pi <= acos <= pi, we have to use d.x to increase the range of yaw
            constexpr float pi = glm::pi<float>();
            //sign(0) = 1
            const float sign = d.x == 0.0f ? 1.0f : glm::sign(d.x);
            yaw = pi - (pi - yaw) * sign;

            pitch += deltaPitch;
            yaw += deltaYaw;

            d.x = glm::sin(yaw) * glm::cos(pitch);
            d.y = glm::sin(pitch);
            d.z = glm::cos(yaw) * glm::cos(pitch);
            dir = d;
        }
    };
    class CameraBuilder {
        glm::vec3 pos = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 dir = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
        float fov = 45.0f;
        float aspect = Camera::autoAspect;
        float maxY = 0.9f;
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
        float sensitivity = 0.001f;
        public:
        CameraBuilder() = default;
        CameraBuilder& setPos(const glm::vec3& inPos) noexcept {
            pos = inPos;
            return *this;
        }
        CameraBuilder& setDir(const glm::vec3& inDir) noexcept {
            dir = inDir;
            return *this;
        }
        CameraBuilder& setUp(const glm::vec3& inUp) noexcept {
            up = inUp;
            return *this;
        }
        CameraBuilder& setFov(float inFov) noexcept {
            fov = inFov;
            return *this;
        }
        CameraBuilder& setAspect(float inAspect) noexcept {
            aspect = inAspect;
            return *this;
        }
        CameraBuilder& setMaxY(float inMaxY) noexcept {
            maxY = inMaxY;
            return *this;
        }
        CameraBuilder& setNearPlane(float inNearPlane) noexcept {
            nearPlane = inNearPlane;
            return *this;
        }
        CameraBuilder& setFarPlane(float inFarPlane) noexcept {
            farPlane = inFarPlane;
            return *this;
        }
        CameraBuilder& setSensitivity(float inSensitivity) noexcept {
            sensitivity = inSensitivity;
            return *this;
        }
        Camera build() const noexcept {
            Camera cam = {
                .pos = pos,
                .dir = glm::normalize(dir),
                .up = glm::normalize(up),
                .fov = fov,
                .aspect = aspect,
                .maxY = maxY,
                .nearPlane = nearPlane,
                .farPlane = farPlane,
                .sensitivity = sensitivity
            };
            //Lock camera direction vector to bounds of maxY
            if(cam.dir.y < -cam.maxY) cam.dir.y = -cam.maxY;
            else if(cam.dir.y > cam.maxY) cam.dir.y = cam.maxY;

            return cam;
        }
    };
}

template<>
Error<mars::Camera> JSON::valueTo(const JSON::Value& value) noexcept;
