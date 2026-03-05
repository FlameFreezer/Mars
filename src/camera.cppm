module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define SENSITIVITY 0.001f
#define MAX_Y 0.90f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

export module mars:camera;
import error;

namespace mars {
    export struct Camera {
        static constexpr float autoAspect = 0.0f;
        glm::vec3 pos;
        glm::vec3 dir;
        glm::vec3 up;
        float fov;
        float aspect;
        Camera() noexcept : aspect(autoAspect), fov(0.0f) {}
        Camera(const glm::vec3& inPos, const glm::vec3& inDir, glm::vec3 inUp, float inFov, float inAspect) noexcept 
            : pos(inPos), dir(glm::normalize(inDir)), up(glm::normalize(inUp)), fov(inFov), aspect(inAspect) {
                if(dir.y < -MAX_Y) dir.y = -MAX_Y;
                else if(dir.y > MAX_Y) dir.y = MAX_Y;
            }
        glm::mat4 loadMatrices() const noexcept {
            //dir + pos = target (position the camera is looking at)
            const glm::mat4 view = glm::lookAt(pos, dir + pos, up);
            glm::mat4 proj = glm::perspective(fov, aspect, NEAR_PLANE, FAR_PLANE);
            proj[1][1] *= -1.0f;
            return proj * view;
        }
        void rotate(float deltaX, float deltaY) noexcept {
            glm::vec3 d(dir);

            if(d.y >= MAX_Y and deltaY > 0.0f) deltaY = 0.0f;
            else if(d.y <= -MAX_Y and deltaY < 0.0f) deltaY = 0.0f;

            const float deltaYaw = deltaX * SENSITIVITY;
            const float deltaPitch = deltaY * SENSITIVITY;

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
}
