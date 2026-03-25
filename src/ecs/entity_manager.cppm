module;

#include <queue>

export module entity_manager;
import types;
import entity;

namespace mars {
    export class EntityManager {
        std::queue<ID> mIDQueue;
        public:
        EntityManager() noexcept;
        ~EntityManager() noexcept;
        ID createEntity() noexcept;
        void destroyEntity(ID id) noexcept;
    };
}