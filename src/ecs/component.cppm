module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module component;
import types;

namespace mars {
    export enum class Components : u8 {
        TRANSFORM = 0,
        PHYSICS,
        //KEEP THIS AT THE END OF THE ENUM
        MAX_COMPONENT
    };
    export constexpr u8 numComponents = static_cast<u8>(Components::MAX_COMPONENT);

    export struct Transform {
        glm::vec2 position;
        glm::vec2 scale;
        float angle;
    };
    export struct Physics {
        glm::vec2 velocity;
        glm::vec2 acceleration;
    };

    export class Signature {
        u32 mBits;
        public:
        explicit Signature(std::initializer_list<Components> comps) noexcept;
        Signature(const Signature& s) noexcept;
        bool has(std::initializer_list<Components> comps) const noexcept;
        bool has(Components comp) const noexcept;
        u32 getBits() const noexcept;
    };
}
