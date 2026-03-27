module;

#include <glm/glm.hpp>

export module position;

namespace mars {
    //Helper class to help programmers keep transforms and collisions aligned
    export class Position {
        glm::vec2& mTransform;
        glm::vec2& mCollide;
        public:
        Position(glm::vec2& t, glm::vec2& c) noexcept : mTransform(t), mCollide(c) {}
        Position operator=(glm::vec2 rhs) noexcept {
            mTransform = rhs;
            mCollide = rhs;
            return *this;
        }
        Position operator+=(glm::vec2 rhs) noexcept {
            mTransform += rhs;
            mCollide += rhs;
            return *this;
        }
        Position operator-=(glm::vec2 rhs) noexcept {
            mTransform -= rhs;
            mCollide -= rhs;
            return *this;
        }
        Position operator*=(float rhs) noexcept {
            mTransform *= rhs;
            mCollide *= rhs;
            return *this;
        }
        Position operator/=(float rhs) noexcept {
            mTransform /= rhs;
            mCollide /= rhs;
            return *this;
        }
    };
}