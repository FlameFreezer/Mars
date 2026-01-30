module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;
import multimap;

namespace mars {
    export using ID = std::uint64_t;
    export class Objects : public ArrayMultimap {
        public:
        ID* meshIDs;
        ID* textureIDs;
        glm::vec3* positions;
        glm::vec3* scales;
        static constexpr glm::mat4 identity{1.0f};

        Objects(std::size_t capacity) noexcept :
            ArrayMultimap(capacity),
            meshIDs(new ID[capacity]),
            textureIDs(new ID[capacity]),
            positions(new glm::vec3[capacity]),
            scales(new glm::vec3[capacity]) {}

        void getModelMatrices(glm::mat4* outMatrices) const noexcept {
            for(std::size_t i = 0; i < size; i++) {
                outMatrices[i] = glm::translate(identity, positions[i]) * glm::scale(identity, scales[i]);
            }
        }
        ~Objects() noexcept {
            delete[] meshIDs;
            delete[] textureIDs;
            delete[] positions;
            delete[] scales;
        }
    };
};
