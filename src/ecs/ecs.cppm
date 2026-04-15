module;

export module ecs;
export import entity;
export import component;
export import entity_manager;
import types;
import error;

namespace mars {
    export class EntityComponentSystem {
        EntityComponentSystem() noexcept = default;
        EntityComponentSystem(const EntityComponentSystem& other) = delete;
        EntityComponentSystem(EntityComponentSystem&& other) = delete;
        public:
        EntityManager entityManager;
        ComponentManager componentManager;
        static EntityComponentSystem& get() noexcept;
        Error<Entity> createEntity(Signature s) noexcept;
        void destroyEntity(Entity e) noexcept;
        Entity entity(ID id) const noexcept;
    };
    export using ECS = EntityComponentSystem;
}
