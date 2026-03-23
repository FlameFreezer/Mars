module;

#include <queue>

#include <vulkan/vulkan.h>

export module ecs;
export import entity;
export import component_system;
export import component;
import types;
import error;

#define DEFINE_COMPONENT_GETTER(component) const GetComp<Component::component>::Type& component(Entity e) const noexcept{\
    return system<Component::component>()[e];\
}\
GetComp<Component::component>::Type& component(Entity e) noexcept {\
    return system<Component::component>()[e];\
}\

namespace mars {
    //Simple wrapper over the systems to allow easily indexing them with members of the Component enum
    export class ComponentSystems {
        ComponentSystemParent* mSystems[numComponents];
        public:
        friend class EntityManager;
        ComponentSystemParent*& operator[](Component c) noexcept;
        const ComponentSystemParent* const& operator[](Component c) const noexcept;
    };

    export class PhysicsManager {
        ComponentSystem<Physics>& sysPhysics;
        ComponentSystem<Dynamics>& sysDynamics;
        ComponentSystem<Transform>& sysTransform;
        ComponentSystem<Collide>& sysCollide;
        void applyPhysics(float deltaTime) noexcept;
        void getCollisions() noexcept;
        void resolveCollisions() noexcept;
        public:
        PhysicsManager(EntityManager& em) noexcept;
        bool checkCollision(ID id1, ID id2) const noexcept;
        bool checkCollision(Entity e1, Entity e2) const noexcept;
        void doPhysicsStep(float deltaTime) noexcept;
    };

    export class EntityManager {
        std::queue<ID> mIDQueue;
        ComponentSystems mSystems;
        Entity* mEntities = new Entity[maxEntities];
        void createNullEntity() noexcept;
        public:
        EntityManager() noexcept;
        ~EntityManager() noexcept;
        Error<Entity> createEntity(Signature s) noexcept;
        void destroyEntity(Entity e) noexcept;
        Entity getEntity(ID id) const noexcept;
        //GetComp is a struct template defined in components.cppm which has a member Type
        //This member matches a component enum member to the type for its component
        template<Component c>
        ComponentSystem<typename GetComp<c>::Type>& system() noexcept {
            //Using GetComp::Type to get the type of the component in order to downcast the parent pointer safely
            return *reinterpret_cast<ComponentSystem<typename GetComp<c>::Type>*>(mSystems[c]);
        }
        template<Component c>
        const ComponentSystem<typename GetComp<c>::Type>& system() const noexcept {
            //Using GetComp::Type to get the type of the component in order to downcast the parent pointer safely
            return *reinterpret_cast<const ComponentSystem<typename GetComp<c>::Type>*>(mSystems[c]);
        }
        DEFINE_COMPONENT_GETTER(transform)
        DEFINE_COMPONENT_GETTER(physics)
        DEFINE_COMPONENT_GETTER(collide)
        DEFINE_COMPONENT_GETTER(draw)
        DEFINE_COMPONENT_GETTER(dynamics)
    };
}