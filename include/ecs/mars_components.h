#pragma once

#include <memory>
#include <utility>

#include <glm/glm.hpp>

#include <assetmanager/mars_texture.h>
#include <mars_types.h>
#include <mars_room.h>

namespace mars {
    using ComponentT = u8;
    enum class Component : ComponentT {
        DRAW,
        PHYSICS,
        COLLIDE,
        USER_COMP_0,
        USER_COMP_1,
        USER_COMP_2,
        USER_COMP_3,
        USER_COMP_4,
        USER_COMP_5,
        //KEEP THIS AT THE END OF THE ENUM
        MAX_COMPONENT
    };
    constexpr ComponentT numComponents = std::to_underlying(Component::MAX_COMPONENT);

    struct Draw {
        static constexpr Component component = Component::DRAW;
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 scale = glm::vec2(1.0f);
        float angle = 0;
        float zLayer = 0;
        std::shared_ptr<Texture> texture;
        u64 spriteIndex = 0;
    };
    struct Physics {
        static constexpr Component component = Component::PHYSICS;
        glm::vec2 velocity = glm::vec2(0.0f);
        glm::vec2 gravity = glm::vec2(0.0f, 1.0f);
        const Room* room = nullptr;
        bool doUpdate = true;
    };
    enum class BoundingShape : u8 {
        rectangle,
        circle,
    };
    struct Collide {
        static constexpr Component component = Component::COLLIDE;
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
    template<> struct GetComp<Component::DRAW> {using Type = Draw;};
    template<> struct GetComp<Component::PHYSICS> {using Type = Physics;};
    template<> struct GetComp<Component::COLLIDE> {using Type = Collide;};
}
