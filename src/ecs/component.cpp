module;

#include <glm/glm.hpp>

module component;

namespace mars {
    template<ComponentT c>
    void allocSystem(ComponentSystemParent* systems[]) noexcept {
        //Allocate the system
        systems[c] = new ComponentSystem<typename GetComp<static_cast<Component>(c)>::Type>();
        //Reserve space for the null entity
        systems[c]->reserve(nullID);
        //Allocate the next system
        allocSystem<c + 1>(systems);
    }
    //Base Case: do nothing once all systems have been allocated
    template<> void allocSystem<numComponents>(ComponentSystemParent**) noexcept {}

    void allocSystems(ComponentSystemParent* systems[]) noexcept {
        allocSystem<0>(systems);
    }

    ComponentManager::ComponentManager() noexcept {
        allocSystems(mSystems);
    }
    ComponentManager::~ComponentManager() noexcept {
        for(ComponentT i = 0; i < numComponents; i++) {
            delete mSystems[i];
        }
        delete[] mSignatures;
    }

    void ComponentManager::reserveFor(ID id, Signature s) noexcept {
        SignatureT bits = s.getBits();
        ComponentT bitNum = 0;
        for(SignatureT i = 1; i != 0; i <<= 1) {
            if(bits & i) {
                mSystems[bitNum]->reserve(id);
            }
            ++bitNum;
        }
        mSignatures[id] = s;
    }

    void ComponentManager::freeFor(ID id) noexcept {
        Signature s = mSignatures[id];
        mSignatures[id] = Signature();
        SignatureT bits = s.getBits();
        ComponentT bitNum = 0;
        for(SignatureT i = 1; i != 0; i <<= 1) {
            if(bits & i) {
                mSystems[bitNum]->erase(id);
            }
            ++bitNum;
        }
    }

    Signature ComponentManager::getSignature(ID id) const noexcept {
        return mSignatures[id];
    }

    Position ComponentManager::position(Entity e) noexcept {
        return {
            transform(e).position, collide(e).position
        };
    }
}