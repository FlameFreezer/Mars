module;

#include <queue>

module entity_manager;

namespace mars {
    EntityManager::EntityManager() noexcept {
        for(ID i = 1; i < maxEntities; i++) {
            mIDQueue.push(i);
        }
    }
    EntityManager::~EntityManager() noexcept {
    }
    ID EntityManager::createEntity() noexcept {
        if(mIDQueue.empty()) return nullID;
        ID id = mIDQueue.front();
        mIDQueue.pop();
        return id;
    }
    void EntityManager::destroyEntity(ID id) noexcept {
        mIDQueue.push(id);
    }
}