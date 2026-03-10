module;

#include <vulkan/vulkan.h>

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
        mSystems[Components::TRANSFORM] = new ComponentSystem<Transform>();
        mSystems[Components::PHYSICS] = new ComponentSystem<Physics>();
        mSystems[Components::MESH] = new ComponentSystem<ID>();
        mSystems[Components::TEXTURE] = new ComponentSystem<ID>();
    }

    EntityManager::~EntityManager() noexcept {
        for(u8 i = 0; i < numComponents; i++) {
            delete mSystems[static_cast<Components>(i)];
        }
    }

    Entity EntityManager::createEntity(Signature sig) noexcept {
        const ID id = mIDQueue.front();
        mIDQueue.pop();
        //Number of the current bit we are checking in the signature
        //Corresponds to the component number for this bit
        u8 bitNum = 0;
        //For each bit in the signature
        for(u32 x = 1; x < (1 << 31); x <<= 1) {             
            if(sig.getBits() & x) {
                mSystems[static_cast<Components>(bitNum++)]->reserve(id);
            }
        }
        return Entity(id, sig);
    }

    void EntityManager::destroyEntity(Entity e) noexcept {
        ID id = e.id();        
        mIDQueue.push(id);
        Signature sig = e.signature();
        u8 bitNum = 0;
        for(u32 x = 1; x < (1 << 31); x <<= 1) {
            if(sig.getBits() & x) {
                mSystems[static_cast<Components>(bitNum++)]->erase(id);
            }
        }
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
