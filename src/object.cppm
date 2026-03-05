module;

#include <format>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

export module object;
import multimap;
import error;

namespace mars {
    export class Objects : public ArrayMultimap {
        ID* meshIDs;
        ID* textureIDs;
        glm::vec3* positions;
        glm::vec3* scales;
        glm::mat4* models;
        static constexpr glm::mat4 identity{1.0f};

        void clear() noexcept {
            delete[] meshIDs;
            delete[] textureIDs;
            delete[] positions;
            delete[] scales;
            delete[] models;
        }

        void realloc() noexcept {
            this->ArrayMultimap::realloc();
            const std::size_t cap = this->capacity();
            const std::size_t s = this->size();

            ID* newMeshes = new ID[cap];
            ID* newTextures = new ID[cap];
            glm::vec3* newPos = new glm::vec3[cap];
            glm::vec3* newScale = new glm::vec3[cap];
            glm::mat4* newModels = new glm::mat4[cap];
            for(std::size_t i = 0; i < s; i++) {
                newMeshes[i] = meshIDs[i];
                newTextures[i] = textureIDs[i];
                newPos[i] = positions[i];
                newScale[i] = scales[i];
                newModels[i] = models[i];
            }
            clear();
            meshIDs = newMeshes;
            textureIDs = newTextures;
            positions = newPos;
            scales = newScale;
            models = newModels;
        }
        public:
        //Give Game direct access to internal arrays
        friend class Game;
        Objects() noexcept : ArrayMultimap() {
            meshIDs = new ID[this->capacity()];
            textureIDs = new ID[this->capacity()];
            positions = new glm::vec3[this->capacity()];
            scales = new glm::vec3[this->capacity()];
            models = new glm::mat4[this->capacity()];
        }

        Error<noreturn> updateModelMatrices(const std::vector<ID>& toUpdate) const noexcept {
            const std::size_t s = this->size();
            for(ID id : toUpdate) {
                const ID index = this->getIndex(id);
                if(index >= s) {
                    return fatal(std::format("Out of bounds: Object ID {} (index {}) is invalid for object array of size {}", id, index, s));
                }
                models[index] = glm::translate(identity, positions[index]) * glm::scale(identity, scales[index]);
            }
            return success();
        }
        ~Objects() noexcept {
            clear();
        }

        ID append(ID meshID, ID textureID, const glm::vec3& position, const glm::vec3& scale) noexcept {
            if(this->size() == this->capacity()) {
                realloc();
            }
            const std::size_t s = this->size();
            meshIDs[s] = meshID;
            textureIDs[s] = textureID;
            positions[s] = position;
            scales[s] = scale;
            return this->ArrayMultimap::append();
        }
    };
};
