module;

module ecs;

#define ALLOC_SYSTEM(component) mSystems[(component)] = new ComponentSystem<typename GetComp<(component)>::Type>()

namespace mars {
    ComponentSystemParent*& ComponentSystems::operator[](Component c) noexcept {
        return mSystems[static_cast<ComponentT>(c)];
    }

    const ComponentSystemParent* const& ComponentSystems::operator[](Component c) const noexcept {
        return mSystems[static_cast<ComponentT>(c)];
    }

    EntityManager::EntityManager() noexcept {
        for(ID i = 0; i < maxEntities; i++) {
            mIDQueue.push(i);
            mEntities[i] = nullEntity;
        }
        ALLOC_SYSTEM(Component::TRANSFORM);
        ALLOC_SYSTEM(Component::PHYSICS);
        ALLOC_SYSTEM(Component::MESH);
        ALLOC_SYSTEM(Component::TEXTURE);
        ALLOC_SYSTEM(Component::SOLID);
    }

    EntityManager::~EntityManager() noexcept {
        for(ComponentT i = 0; i < numComponents; i++) {
            delete mSystems.mSystems[i];
        }
    }

    Error<Entity> EntityManager::createEntity(Signature sig) noexcept {
        if(mIDQueue.empty()) {
            return fatal<Entity>("Tried to create an entity, but the maximum number of entities were made!");
        }
        const ID id = mIDQueue.front();
        mIDQueue.pop();
        //Number of the current bit we are checking in the signature
        //Corresponds to the component number for this bit
        ComponentT bitNum = 0;
        //For each bit in the signature
        for(SignatureT x = 1; x < (1 << 31); x <<= 1) {             
            if(sig.getBits() & x) {
                mSystems[static_cast<Component>(bitNum)]->reserve(id);
            }
            bitNum++;
        }
        Entity e(id, sig);
        mEntities[id] = e;
        return e;
    }

    void EntityManager::destroyEntity(Entity e) noexcept {
        ID id = e.id();        
        mIDQueue.push(id);
        Signature sig = e.signature();
        ComponentT bitNum = 0;
        for(SignatureT x = 1; x < (1 << 31); x <<= 1) {
            if(sig.getBits() & x) {
                mSystems[static_cast<Component>(bitNum++)]->erase(id);
            }
        }
        mEntities[id] = nullEntity;
    }

    Entity EntityManager::getEntity(ID id) const noexcept {
        return mEntities[id];
    }
};
