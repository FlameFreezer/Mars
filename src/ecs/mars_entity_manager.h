#pragma once

#include <queue>

#include "mars_types.h"

namespace mars {
    class EntityManager {
        std::queue<ID> mIDQueue;
        public:
        EntityManager() noexcept;
        ID createEntity() noexcept;
        void destroyEntity(ID id) noexcept;
    };
}