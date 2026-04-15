module;

module ecs;

namespace mars {
    EntityComponentSystem& EntityComponentSystem::get() noexcept {
        static EntityComponentSystem instance;
        return instance;
    }
    Error<Entity> EntityComponentSystem::createEntity(Signature s) noexcept {
        ID id = entityManager.createEntity();
        if(id == nullID) {
            return fatal<Entity>("Tried to create an entity, but the maximum number of entities were already created");
        }
        componentManager.reserveFor(id, s);
        return Entity(id, s);
    }

    void EntityComponentSystem::destroyEntity(Entity e) noexcept {
        if(e == nullEntity) return;
        componentManager.freeFor(e.id());
        entityManager.destroyEntity(e.id());
    }

    Entity EntityComponentSystem::entity(ID id) const noexcept {
        return {id, componentManager.getSignature(id)};
    }
};
