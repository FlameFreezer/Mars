module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;

namespace mars {
    export struct Object {
        std::size_t meshIndex;
        std::size_t textureIndex;
        glm::vec3 pos;

        glm::mat4 getModelMatrix() const noexcept {
            return glm::translate(glm::mat4(1.0f), pos);
        }
    };
};
