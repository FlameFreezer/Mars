module;

#include <queue>


export module ecs;
export import entity;
export import component_system;
export import component;
import types;
import error;

namespace mars {
    export class ComponentSystems {
        ComponentSystemParent* mSystems[numComponents];
        public:
        friend class EntityManager;
        ComponentSystemParent*& operator[](Components c) noexcept;
        const ComponentSystemParent* const& operator[](Components c) const noexcept;
    };

    export class EntityManager {
        std::queue<ID> mIDQueue;
        void reserveFor(ID id, Components c) noexcept;
        public:
        ComponentSystems systems;
        EntityManager() noexcept;
        ~EntityManager() noexcept;
        Entity createEntity(Signature s) noexcept;
        void destroyEntity(Entity e) noexcept;
    };
}
