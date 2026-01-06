module;

#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;
import gpubuffer;

namespace mars {
    export struct Model {
        GPUBuffer mesh;
        VkImageView material;
    };

    export struct Object {
        Model* model;
        glm::vec3 pos;

        glm::mat4 getModelMatrix() const noexcept {
            return glm::translate(glm::mat4(1.0f), pos);
        }
    };
};
