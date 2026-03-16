module;

#include <vulkan/vulkan.h>

module ecs;

namespace mars {
    ComponentSystemParent*& ComponentSystems::operator[](Component c) noexcept {
        return mSystems[static_cast<u8>(c)];
    }

    const ComponentSystemParent* const& ComponentSystems::operator[](Component c) const noexcept {
        return mSystems[static_cast<u8>(c)];
    }

    template<Component c>
    void EntityManager::allocSystem() noexcept {
        mSystems[c] = new ComponentSystem<typename GetComp<c>::Type>();
    }

    EntityManager::EntityManager() noexcept {
        for(ID i = 0; i < maxEntities; i++) {
            mIDQueue.push(i);
            mEntities[i] = nullEntity;
        }
        allocSystem<Component::TRANSFORM>();
        allocSystem<Component::PHYSICS>();
        allocSystem<Component::MESH>();
        allocSystem<Component::TEXTURE>();
        allocSystem<Component::SOLID>();
    }

    EntityManager::~EntityManager() noexcept {
        for(u8 i = 0; i < numComponents; i++) {
            delete mSystems[static_cast<Component>(i)];
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
        u8 bitNum = 0;
        //For each bit in the signature
        for(u32 x = 1; x < (1 << 31); x <<= 1) {             
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
        u8 bitNum = 0;
        for(u32 x = 1; x < (1 << 31); x <<= 1) {
            if(sig.getBits() & x) {
                mSystems[static_cast<Component>(bitNum++)]->erase(id);
            }
        }
        mEntities[id] = nullEntity;
    }

    Entity EntityManager::getEntity(ID id) const noexcept {
        return mEntities[id];
    }

    RendererEntityManager::RendererEntityManager() noexcept {
        for(ID i = 0; i < maxMeshes; i++) {
            mMeshIDQueue.push(i);
        }
        for(ID i = 0; i < maxTextures; i++) {
            mTextureIDQueue.push(i);
        }
    }

    ID RendererEntityManager::insertMesh(VkBuffer handle, VkDeviceMemory memory, VkDeviceSize indexOffset, u32 numIndices) noexcept {
        const ID id = mMeshIDQueue.front();
        mMeshIDQueue.pop();
        sysMesh.insert(id, handle, memory, indexOffset, numIndices);
        return id;
    }
    ID RendererEntityManager::insertTexture(const Texture& t) noexcept {
        const ID id = mTextureIDQueue.front();
        mTextureIDQueue.pop();
        sysTexture.insert(id, t);
        return id;
    }

    void RendererEntityManager::eraseMesh(ID id) noexcept {
        sysMesh.erase(id);
        mMeshIDQueue.push(id);
    }
    void RendererEntityManager::eraseTexture(ID id) noexcept {
        sysTexture.erase(id);
        mTextureIDQueue.push(id);
    }
};
