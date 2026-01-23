module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;

namespace mars {
    export struct Objects {
        std::size_t* meshIndices;
        std::size_t* textureIndices;
        glm::vec3* positions;
        std::size_t size;
        static constexpr std::size_t capacity = 50;
        static constexpr glm::mat4 identity{1.0f};

        void getModelMatrices(glm::mat4* outMatrices) const noexcept {
            for(std::size_t i = 0; i < size; i++) {
                outMatrices[i] = glm::translate(identity, positions[i]);
            }
        }
        ~Objects() noexcept {
            delete[] meshIndices;
            delete[] textureIndices;
            delete[] positions;
        }
    };
};
