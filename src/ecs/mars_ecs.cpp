#include <ecs/mars_ecs.h>

#include <mars_types.h>
#include <mars_constants.h>
#include <error.h>

namespace mars {
    template<ComponentT c>
    void allocSystem(ComponentSystemParent* systems[]) noexcept {
        systems[c] = new ComponentSystem<typename GetComp<static_cast<Component>(c)>::Type>();
        systems[c]->reserve(nullID);
        allocSystem<c + 1>(systems);
    }
    // Base Case: do nothing once all systems have been allocated. We stop at the first user
    // component since these should be manually initialized by the user
    template<> void allocSystem<std::to_underlying(Component::USER_COMP_0)>(ComponentSystemParent**) noexcept {}

    void allocSystems(ComponentSystemParent* systems[]) noexcept {
        allocSystem<0>(systems);
    }

    EntityComponentSystem::EntityComponentSystem() noexcept {
        //Start at ID 1 to skip the nullID at index 0
        for(ID i = 1; i < maxEntities; i++) {
            mIDs.push(i);
        }
        mSignatures[nullID] = nullSignature;
        for (ComponentT i = 0; i < numComponents; i++) mComponentSystems[i] = nullptr;
        allocSystems(mComponentSystems);
    }
    EntityComponentSystem::~EntityComponentSystem() noexcept {
        for(ComponentT i = 0; i < numComponents; i++) {
            if (mComponentSystems[i]) {
                delete mComponentSystems[i];
            }
        }
    }
    Error<Entity> EntityComponentSystem::createEntity(Signature s) noexcept {
        if (mIDs.empty()) {
            FATAL("Tried to create an entity, but the maximum number of entities were already created");
        }
        const ID id = mIDs.front();
        mIDs.pop();

        const SignatureT bits = s.getBits();
        ComponentT bitNum = 0;
        for(SignatureT i = 1; i != 0; i <<= 1) {
            if(bits & i) {
                // Alert the user if they tried to use a custom component that they didn't initialize
                if (!mComponentSystems[bitNum]) {
                    FATAL(std::format("Component System with number {} was not initialized", bitNum));
                }
                mComponentSystems[bitNum]->reserve(id);
            }
            ++bitNum;
        }

        mSignatures[id] = s;
        return Entity{id, s};
    }

    void EntityComponentSystem::destroyEntity(Entity e) noexcept {
        if(e == nullEntity) return;
        const SignatureT bits = e.signature().getBits();
        ComponentT bitNum = 0;
        for(SignatureT i = 1; i != 0; i <<= 1) {
            if(bits & i) {
                mComponentSystems[bitNum]->erase(e.id());
            }
            ++bitNum;
        }

        mIDs.push(e.id());
    }

    Entity EntityComponentSystem::entityFromID(ID id) const noexcept {
        return Entity{id, mSignatures[id]};
    }
};
