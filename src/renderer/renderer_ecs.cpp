module;

#include <vulkan/vulkan.h>

module renderer_ecs;

namespace mars {
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
}