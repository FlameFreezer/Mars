module;

#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.h>

export module component;
import types;
import error;

namespace mars {
    export using ComponentT = u8;
    export enum class Component : ComponentT {
        TRANSFORM,
        PHYSICS,
        DRAW,
        COLLIDE,
        DYNAMICS,
        //KEEP THIS AT THE END OF THE ENUM
        MAX_COMPONENT
    };
    export constexpr ComponentT numComponents = static_cast<ComponentT>(Component::MAX_COMPONENT);

    export struct Transform {
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 scale = glm::vec2(1.0f);
        float angle = 0;
        float zLayer = 0;
    };
    export struct Physics {
        glm::vec2 velocity = glm::vec2(0.0f);
        glm::vec2 gravity = glm::vec2(0.0f, 1.0f);
    };
    //TODO: default mesh/texture with ID = 0
    export struct Draw {
        ID meshID = 0;
        ID textureID = 0;
    };
    export struct Dynamics {
        std::vector<ID> collisions;
        float acceleration;
        float friction;
        float drag;
        float maxSpeed;
        float jumpSpeed;
        ID floorID = -1;
    };
    export enum class BoundingShape : ComponentT {
        RECTANGLE,
        CIRCLE,
    };
    export struct Collide {
        BoundingShape boundingShape = BoundingShape::RECTANGLE;
        glm::vec2 position = glm::vec2(0.0f);
        union {
            //Only valid if shape is a circle
            float radius;
            //Only valid if shape is a rectangle
            glm::vec2 scale = glm::vec2(1.0f);
        };
        bool isSolid = false;
    };
        
    //This struct template allows accessing the type of a component at compile time just using the actual component enum member
    export template<Component c>
    struct GetComp {};
    template<> struct GetComp<Component::TRANSFORM> {using Type = Transform;};
    template<> struct GetComp<Component::PHYSICS> {using Type = Physics;};
    template<> struct GetComp<Component::DRAW> {using Type = Draw;};
    template<> struct GetComp<Component::COLLIDE> {using Type = Collide;};
    template<> struct GetComp<Component::DYNAMICS> {using Type = Dynamics;};
}
