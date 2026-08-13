#pragma once

#include <queue>

#include <vulkan/vulkan.h>

#include "mars_constants.h"
#include "mars_types.h"
#include "mars_component_system.h"

namespace mars {
    //The Mesh and Texture component systems are internal to the renderer and are managed by their own entity manager
    struct Mesh {
        VkBuffer handles[maxMeshes] = {};
        VkDeviceMemory memories[maxMeshes] = {};
        struct {
            VkDeviceSize indexOffset = 0;
            u32 numIndices = 0;
        } sizes[maxMeshes];
    };

    template<>
    class ComponentSystem<Mesh> : public ComponentSystemParent {
        //Remember, Mesh is a struct of arrays
        Mesh mData {};
        public:
        //Getter for array of vertex buffer handles
        VkBuffer* handles() noexcept {
            return mData.handles;
        }
        const VkBuffer* handles() const noexcept {
            return mData.handles;
        }
        //Getter for array of device memories
        VkDeviceMemory* memories() noexcept {
            return mData.memories;
        }
        const VkDeviceMemory* memories() const noexcept {
            return mData.memories;
        }
        //Getter for the offset of the index buffer within the mesh associated with id
        VkDeviceSize getIndexOffset(ID id) const noexcept {
            return mData.sizes[mIndices[id]].indexOffset;
        }
        //Getter for the number of indices in the index buffer within the mesh associated with id
        u32 getNumIndices(ID id) const noexcept {
            return mData.sizes[mIndices[id]].numIndices;
        }
        void insert(ID id, VkBuffer handle, VkDeviceMemory memory, VkDeviceSize indexOffset, u32 numIndices) noexcept {
            u64 index = reserve(id);
            mData.handles[index] = handle;
            mData.memories[index] = memory;
            mData.sizes[index] = {indexOffset, numIndices};
        }
        //Does no deallocation of data - make sure to call vulkan functions first
        void erase(ID id) noexcept {
            u64 index = mIndices[id];
            mData.handles[index] = mData.handles[mSize];
            mData.memories[index] = mData.memories[mSize];
            mData.sizes[index] = mData.sizes[mSize];
            swapErase(id);
        }
        //Deallocates all internal data
        void destroySystem(VkDevice device) noexcept {
            for(u64 i = 0; i < mSize; i++) {
                vkDestroyBuffer(device, mData.handles[i], nullptr);
            }
            for(u64 i = 0; i < mSize; i++) {
                vkFreeMemory(device, mData.memories[i], nullptr);
            }
            mSize = 0;
        }
    };

    class RendererEntityManager {
        std::queue<ID> mMeshIDQueue;
        public:
        RendererEntityManager() noexcept;
        ~RendererEntityManager() noexcept;
        ComponentSystem<Mesh>* sysMesh = new ComponentSystem<Mesh>;
        ID insertMesh(VkBuffer handle, VkDeviceMemory memory, VkDeviceSize indexOffset, u32 numIndices) noexcept;
        void eraseMesh(ID id) noexcept;
        void eraseTexture(ID id) noexcept;
    };
}