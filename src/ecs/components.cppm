module;

#include <vector>
#include <utility>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

export module components;
import types;
import error;
import room;

namespace mars {
    export using ComponentT = u8;
    export enum class Component : ComponentT {
        transform,
        physics,
        draw,
        collide,
        dynamics,
        ledgeGrab,
        //KEEP THIS AT THE END OF THE ENUM
        maxComponent
    };
    export constexpr ComponentT numComponents = std::to_underlying(Component::maxComponent);
    static_assert(numComponents < 1U << 31);

    export struct Transform {
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 scale = glm::vec2(1.0f);
        float angle = 0;
        float zLayer = 0;
    };
    export struct Physics {
        glm::vec2 velocity = glm::vec2(0.0f);
        glm::vec2 gravity = glm::vec2(0.0f, 1.0f);
        const Room* room = nullptr;
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
        ID floorID = nullID;
        ID wallID = nullID;
    };
    export struct LedgeGrab {
        ID ledgeID = nullID;
        float ledgeGrabRange = 0.5f;
        float shoulderHeight = 0.25f;
        float waistHeight = 0.75f;
    };
    export enum class BoundingShape : u8 {
        rectangle,
        circle,
    };
    export struct Collide {
        BoundingShape boundingShape = BoundingShape::rectangle;
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
    template<> struct GetComp<Component::transform> {using Type = Transform;};
    template<> struct GetComp<Component::physics> {using Type = Physics;};
    template<> struct GetComp<Component::draw> {using Type = Draw;};
    template<> struct GetComp<Component::collide> {using Type = Collide;};
    template<> struct GetComp<Component::dynamics> {using Type = Dynamics;};
    template<> struct GetComp<Component::ledgeGrab> {using Type = LedgeGrab;};
}
