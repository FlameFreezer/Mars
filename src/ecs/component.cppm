module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.h>

export module component;
import types;

namespace mars {
    export enum class Components : u8 {
        TRANSFORM,
        PHYSICS,
        MESH,
        TEXTURE,
        //KEEP THIS AT THE END OF THE ENUM
        MAX_COMPONENT
    };
    export constexpr u8 numComponents = static_cast<u8>(Components::MAX_COMPONENT);

    export struct Transform {
        glm::vec3 position;
        glm::vec2 scale;
        float angle;
    };
    export struct Physics {
        glm::vec2 velocity;
        glm::vec2 acceleration;
    };
    //The Mesh and Texture component systems are internal to the renderer and are managed by their own entity manager
    export constexpr u64 maxMeshes = 1000;
    export struct Mesh {
        VkBuffer handles[maxMeshes];
        VkDeviceMemory memories[maxMeshes];
        struct {
            VkDeviceSize indexOffset;
            u32 numIndices;
        } sizes[maxMeshes];
    };
    export constexpr u64 maxTextures = 1000;
    export struct Texture {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
    };

    //This struct template allows accessing the type of a component at compile time just using the actual component enum member
    export template<Components c>
    struct GetComp {};
    template<> struct GetComp<Components::TRANSFORM> {using Type = Transform;};
    template<> struct GetComp<Components::PHYSICS> {using Type = Physics;};
    template<> struct GetComp<Components::MESH> {using Type = ID;};
    template<> struct GetComp<Components::TEXTURE> {using Type = ID;};
}
