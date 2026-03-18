module;

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
        MESH,
        TEXTURE,
        SOLID,
        //KEEP THIS AT THE END OF THE ENUM
        MAX_COMPONENT
    };
    export constexpr ComponentT numComponents = static_cast<ComponentT>(Component::MAX_COMPONENT);

    export struct Transform {
        glm::vec3 position;
        glm::vec2 scale;
        float angle;
    };
    export struct Physics {
        glm::vec2 velocity;
        glm::vec2 gravity;
        float acceleration;
        float friction;
        float drag;
        float maxSpeed;
        float jumpSpeed;
    };
    export enum class BoundingShape : ComponentT {
        RECTANGLE,
        CIRCLE,
    };
    export struct Solid {
        BoundingShape boundingShape;
        glm::vec2 position;
        union {
            //Only valid if shape is a circle
            float radius;
            //Only valid if shape is a rectangle
            glm::vec2 scale;
        };
    };
        
    //This struct template allows accessing the type of a component at compile time just using the actual component enum member
    export template<Component c>
    struct GetComp {};
    template<> struct GetComp<Component::TRANSFORM> {using Type = Transform;};
    template<> struct GetComp<Component::PHYSICS> {using Type = Physics;};
    template<> struct GetComp<Component::MESH> {using Type = ID;};
    template<> struct GetComp<Component::TEXTURE> {using Type = ID;};
    template<> struct GetComp<Component::SOLID> {using Type = Solid;};
}
