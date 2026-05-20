#pragma once

#include "mars_entity.h"
#include "mars_component.h"
#include "mars_entity_manager.h"
#include "mars_types.h"
#include "error.h"

namespace mars {
    class EntityComponentSystem {
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
    using ECS = EntityComponentSystem;
}
