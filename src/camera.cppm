module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
            : pos(inPos), dir(inDir), up(inUp), fov(inFov), aspect(inAspect) {}
        Error<noreturn> loadMatrices(glm::mat4* dst) const noexcept {
            if(dst == nullptr) return {ErrorTag::FATAL_ERROR, "nullptr passed to function loadMatrices"};
            glm::mat4& view = dst[0];
            glm::mat4& proj = dst[1];
            view = glm::lookAt(pos, dir + pos, up);
            proj = glm::perspective(fov, aspect, 0.1f, 1000.0f);
            proj[1][1] *= -1.0f;
            return success();
        }
    };
}