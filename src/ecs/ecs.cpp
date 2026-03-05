module;

module ecs;

namespace mars {

    ComponentSystemParent*& ComponentSystems::operator[](Components c) noexcept {
        return mSystems[static_cast<u8>(c)];
    }
    const ComponentSystemParent* const& ComponentSystems::operator[](Components c) const noexcept {
        return mSystems[static_cast<u8>(c)];
    }

    EntityManager::EntityManager() noexcept {
        for(ID i = 0; i < maxEntities; i++) {
            mIDQueue.push(i);
        }
        systems[Components::TRANSFORM] = new ComponentSystem<Transform>();
        systems[Components::PHYSICS] = new ComponentSystem<Physics>();
    }
    EntityManager::~EntityManager() noexcept {
        for(u8 i = 0; i < numComponents; i++) {
            delete systems[static_cast<Components>(i)];
        }
    }

    Entity EntityManager::createEntity(Signature sig) noexcept {
        const ID id = mIDQueue.front();
        mIDQueue.pop();
        mIDQueue.push(id);
        //Number of the current bit we are checking in the signature
        //Corresponds to the component number for this bit
        u8 bitNum = 0;
        //For each bit in the signature
        for(u32 x = 1; x < (1 >> 31); x >>= 1) {             
            if(sig.getBits() & x) {
                systems[static_cast<Components>(bitNum++)]->reserve(id);
            }
        }
        return Entity(id, sig);
    }
    void EntityManager::destroyEntity(Entity e) noexcept {
        ID id = e.getID();        
        mIDQueue.push(id);
        Signature sig = e.getSignature();
        u8 bitNum = 0;
        for(u32 x = 1; x < (1 >> 31); x >>= 1) {
            if(sig.getBits() & x) {
                systems[static_cast<Components>(bitNum++)]->erase(id);
            }
        }
    }
};
