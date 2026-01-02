module;

#include <algorithm>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define SENSITIVITY 0.001f
#define MAX_Y 0.90f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 1000.0f

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
        Camera(glm::vec3 const& inPos, glm::vec3 const& inDir, glm::vec3 inUp, float inFov, float inAspect) noexcept 
            : pos(inPos), dir(glm::normalize(inDir)), up(glm::normalize(inUp)), fov(inFov), aspect(inAspect) {
                dir.y = std::clamp(dir.y, -MAX_Y, MAX_Y);
            }
        glm::mat4 loadMatrices() const noexcept {
            //dir + pos = target (position the camera is looking at)
            glm::mat4 const view = glm::lookAt(pos, dir + pos, up);
            glm::mat4 proj = glm::perspective(fov, aspect, NEAR_PLANE, FAR_PLANE);
            proj[1][1] *= -1.0f;
            return proj * view;
        }
        void rotate(float dx, float dy) noexcept {
            glm::vec3 d(dir);

            if(d.y >= MAX_Y and dy > 0.0f) dy = 0.0f;
            else if(d.y <= -MAX_Y and dy < 0.0f) dy = 0.0f;

            float const deltaYaw = -dx * SENSITIVITY;
            float const deltaPitch = dy * SENSITIVITY;

            //This gives the angle of dir with the xz-plane
            float pitch = glm::asin(d.y);
            //cos(pitch) gives the length of the projection of dir onto the xz-plane
            //so, dir.x / cos(pitch) gives the cosine of the angle between dir and the x-axis
            float yaw = glm::acos(d.x / glm::cos(pitch));
            //Since -pi <= acos <= pi, we can use dir.z to tell what quadrant dir is in
            constexpr float pi = glm::pi<float>();
            yaw = pi - (pi - yaw) * glm::sign(d.z);

            pitch += deltaPitch;
            yaw += deltaYaw;

            d.x = glm::cos(yaw) * glm::cos(pitch);
            d.y = glm::sin(pitch);
            d.z = glm::sin(yaw) * glm::cos(pitch);
            dir = d;
        }
    };
}