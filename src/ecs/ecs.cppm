module;

#include <queue>

#include <vulkan/vulkan.h>

export module ecs;
export import entity;
export import component_system;
export import component;
import types;
import error;

namespace mars {
    //Simple wrapper over the systems to allow easily indexing them with members of the Components enum
    export class ComponentSystems {
        ComponentSystemParent* mSystems[numComponents];
        public:
        friend class EntityManager;
        ComponentSystemParent*& operator[](Components c) noexcept;
        const ComponentSystemParent* const& operator[](Components c) const noexcept;
    };

    export class EntityManager {
        std::queue<ID> mIDQueue;
        ComponentSystems mSystems;
        public:
        EntityManager() noexcept;
        ~EntityManager() noexcept;
        Entity createEntity(Signature s) noexcept;
        void destroyEntity(Entity e) noexcept;
        //GetComp is a struct template defined in components.cppm which has a member Type
        //This member matches a component enum member to the type for its component
        template<Components c>
        ComponentSystem<typename GetComp<c>::Type>& system() noexcept {
            //Using GetComp::Type to get the type of the component in order to downcast the parent pointer safely
            return *reinterpret_cast<ComponentSystem<typename GetComp<c>::Type>*>(mSystems[c]);
        }
        template<Components c>
        const ComponentSystem<typename GetComp<c>::Type>& system() const noexcept {
            //Using GetComp::Type to get the type of the component in order to downcast the parent pointer safely
            return *reinterpret_cast<const ComponentSystem<typename GetComp<c>::Type>*>(mSystems[c]);
        }

    };
    //This one manages the entities used internally by the renderer
    export class RendererEntityManager {
        std::queue<ID> mMeshIDQueue;
        std::queue<ID> mTextureIDQueue;
        public:
        RendererEntityManager() noexcept;
        ComponentSystem<Mesh> sysMesh;
        ComponentSystem<Texture> sysTexture;
        ID insertMesh(VkBuffer handle, VkDeviceMemory memory, VkDeviceSize indexOffset, u32 numIndices) noexcept;
        ID insertTexture(const Texture& t) noexcept;
        void eraseMesh(ID id) noexcept;
        void eraseTexture(ID id) noexcept;
    };
}
