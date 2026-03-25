module;

export module ecs;
export import entity;
export import component;
export import entity_manager;
import types;
import error;

namespace mars {
    export class EntityComponentSystem {
        public:
        EntityManager entityManager;
        ComponentManager componentManager;
        Error<Entity> createEntity(Signature s) noexcept;
        void destroyEntity(Entity e) noexcept;
        Entity entity(ID id) const noexcept;
    };

}