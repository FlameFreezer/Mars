#include "mars_renderer_ecs.h"

#include <vulkan/vulkan.h>

#include "mars_constants.h"

namespace mars {
    RendererEntityManager::RendererEntityManager() noexcept {
        for(ID i = 0; i < maxMeshes; i++) {
            mMeshIDQueue.push(i);
        }
    }

    RendererEntityManager::~RendererEntityManager() noexcept {
        delete sysMesh;
    }

    ID RendererEntityManager::insertMesh(VkBuffer handle, VkDeviceMemory memory, VkDeviceSize indexOffset, u32 numIndices) noexcept {
        const ID id = mMeshIDQueue.front();
        mMeshIDQueue.pop();
        sysMesh->insert(id, handle, memory, indexOffset, numIndices);
        return id;
    }

    void RendererEntityManager::eraseMesh(ID id) noexcept {
        sysMesh->erase(id);
        mMeshIDQueue.push(id);
    }
}