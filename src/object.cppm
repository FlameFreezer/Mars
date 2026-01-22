module;

#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;

namespace mars {
    export struct Objects {
        std::vector<std::size_t> meshIndices;
        std::vector<std::size_t> textureIndices;
        std::vector<glm::vec3> positions;

        void getModelMatrices(std::vector<glm::mat4>& outMatrices) const noexcept {
            for(std::size_t i = 0; i < positions.size(); i++) {
                outMatrices.push_back(glm::translate(glm::mat4(1.0f), positions[i]));
            }
        }
    };
};
