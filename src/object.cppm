module;

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;
import multimap;

namespace mars {
    export class Objects : public ArrayMultimap {
        void realloc() noexcept {
            this->ArrayMultimap::realloc();
            std::size_t const cap = this->capacity();
            std::size_t const s = this->size();

            ID* newIDs = new ID[cap];
            glm::vec3* newVectors = new glm::vec3[cap];
            for(std::size_t i = 0; i < s; i++) {
                newIDs[i] = meshIDs[i];
                newVectors[i] = positions[i];
            }
            delete[] meshIDs;
            delete[] positions;
            meshIDs = newIDs;
            positions = newVectors;

            for(std::size_t i = 0; i < s; i++) {
                newIDs[i] = textureIDs[i];
                newVectors[i] = scales[i];
            }
            delete[] textureIDs;
            delete[] scales;
            textureIDs = newIDs;
            scales = newVectors;
        }
        public:
        ID* meshIDs;
        ID* textureIDs;
        glm::vec3* positions;
        glm::vec3* scales;
        static constexpr glm::mat4 identity{1.0f};

        Objects() noexcept : ArrayMultimap() {
            meshIDs = new ID[this->capacity()];
            textureIDs = new ID[this->capacity()];
            positions = new glm::vec3[this->capacity()];
            scales = new glm::vec3[this->capacity()];
        }

        void getModelMatrices(glm::mat4* outMatrices) const noexcept {
            if(outMatrices == nullptr) return;
            std::size_t const s = this->size();
            for(std::size_t i = 0; i < s; i++) {
                outMatrices[i] = glm::translate(identity, positions[i]) * glm::scale(identity, scales[i]);
            }
        }
        ~Objects() noexcept {
            delete[] meshIDs;
            delete[] textureIDs;
            delete[] positions;
            delete[] scales;
        }

        ID append(ID meshID, ID textureID, glm::vec3 const& position, glm::vec3 const& scale) noexcept {
            if(this->size() == this->capacity()) {
                realloc();
            }
            std::size_t const s = this->size();
            meshIDs[s] = meshID;
            textureIDs[s] = textureID;
            positions[s] = position;
            scales[s] = scale;
            return this->ArrayMultimap::append();
        }
    };
};
