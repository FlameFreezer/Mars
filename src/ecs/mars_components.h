#pragma once

#include <vector>
#include <utility>

#include <glm/glm.hpp>

#include "mars_types.h"
#include "mars_room.h"

namespace mars {
    using ComponentT = u8;
    enum class Component : ComponentT {
        draw,
        physics,
        collide,
        dynamics,
        //KEEP THIS AT THE END OF THE ENUM
        maxComponent
    };
    constexpr ComponentT numComponents = std::to_underlying(Component::maxComponent);
    static_assert(numComponents < 1U << 31);

    //TODO: default mesh/texture with ID = 0
    struct Draw {
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 scale = glm::vec2(1.0f);
        float angle = 0;
        float zLayer = 0;
        ID meshID = 0;
        ID textureID = 0;
    };
    struct Physics {
        glm::vec2 velocity = glm::vec2(0.0f);
        glm::vec2 gravity = glm::vec2(0.0f, 1.0f);
        const Room* room = nullptr;
    };
    struct Dynamics {
        std::vector<ID> collisions;
        ID floorID = nullID;
        ID wallID = nullID;
    };
    enum class BoundingShape : u8 {
        rectangle,
        circle,
    };
    struct Collide {
        glm::vec2 position = glm::vec2(0.0f);
        BoundingShape boundingShape = BoundingShape::rectangle;
        union {
            //Only valid if shape is a circle
            float radius;
            //Only valid if shape is a rectangle
            glm::vec2 scale = glm::vec2(1.0f);
        };
        bool isSolid = false;
    };
        
    //This struct template allows accessing the type of a component at compile time just using the actual component enum member
    template<Component c>
    struct GetComp {};
    template<> struct GetComp<Component::draw> {using Type = Draw;};
    template<> struct GetComp<Component::physics> {using Type = Physics;};
    template<> struct GetComp<Component::collide> {using Type = Collide;};
    template<> struct GetComp<Component::dynamics> {using Type = Dynamics;};
}
