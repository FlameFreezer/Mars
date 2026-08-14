#pragma once

#include <queue>
#include <utility>

#include "mars_entity.h"
#include "mars_signature.h"
#include <mars_types.h>
#include <error.h>
#include "mars_component_system.h"

namespace mars {
    class EntityComponentSystem {
	public:
        ~EntityComponentSystem() noexcept;
        EntityComponentSystem(const EntityComponentSystem&) = delete;
        EntityComponentSystem(EntityComponentSystem&&) = delete;
        EntityComponentSystem& operator=(const EntityComponentSystem&) = delete;
        EntityComponentSystem& operator=(EntityComponentSystem&&) = delete;

        static EntityComponentSystem& get() noexcept;
        Error<Entity> createEntity(Signature s) noexcept;
        void destroyEntity(Entity e) noexcept;
        Entity entityFromID(ID id) const noexcept;

        template<Component c>
        ComponentSystem<typename GetComp<c>::Type>& system() noexcept {
            return *reinterpret_cast<ComponentSystem<typename GetComp<c>::Type>*>(mComponentSystems[std::to_underlying(c)]);
        }
        template<Component c>
        const ComponentSystem<typename GetComp<c>::Type>& system() const noexcept {
            return *reinterpret_cast<const ComponentSystem<typename GetComp<c>::Type>*>(mComponentSystems[std::to_underlying(c)]);
        }

        template<Component c>
        void initSystem() noexcept {
            mComponentSystems[std::to_underlying(c)] = new ComponentSystem<typename GetComp<c>::Type>();
            mComponentSystems[std::to_underlying(c)]->reserve(nullID);
        }
    private:
        std::queue<ID> mIDs;
        Signature mSignatures[maxEntities];
        ComponentSystemParent* mComponentSystems[numComponents];
        EntityComponentSystem() noexcept;
    };
    using ECS = EntityComponentSystem;
}

#define MARS_COMPONENT(comp, id) mars::EntityComponentSystem::get().system<comp::component>()[id]
#define MARS_SYSTEM(comp) mars::EntityComponentSystem::get().system<comp::component>()
