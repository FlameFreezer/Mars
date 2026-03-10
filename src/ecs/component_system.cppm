module;

#include <format>

#include <vulkan/vulkan.h>

export module component_system;
import entity;
import component;
import types;
import error;
import gpubuffer;

namespace mars {
    //Abstract parent class for component systems - to automate initialization and deinitialization
    // within the EntityManager
    export class ComponentSystemParent {
        protected:
        ID mIDs[maxEntities];
        u64 mIndices[maxEntities];
        u64 mSize;
        void swapErase(ID id) noexcept {
            //Swap the index of the data at id for the data at end
            mIndices[mIDs[mSize]] = mIndices[id];
            //Decrement size
            --mSize;
        }
        public:
        ComponentSystemParent() noexcept : mSize(0) {}
        virtual ~ComponentSystemParent() noexcept {}
        u64 size() const noexcept {
            return mSize;
        }
        //Writes the ID to correct spaces of constituent arrays, then increments size
        u64 reserve(ID id) noexcept {
            mIndices[id] = mSize;
            mIDs[mSize] = id;
            return mSize++;
        }
        //Gets the internal index associated with the ID
        u64 index(ID id) const noexcept {
            return mIndices[id];
        }
        //Gets the array of IDs
        ID* getIDs() noexcept {
            return mIDs;
        }
        const ID* getIDs() const noexcept {
            return mIDs;
        }
        //Abstract function which should swap the data at id with the data at the end, then calls
        // swapErase
        virtual void erase(ID id) noexcept = 0;

    };
    export template<typename C>
    class ComponentSystem : public ComponentSystemParent {
        C mData[maxEntities];
        public:
        //Safety checked random access of data
        Error<const C*> at(ID id) const noexcept {
            if(id >= maxEntities) {
                return {ErrorTag::FATAL_ERROR, std::format("Invalid ID {} passed to function \"at\"", id)};
            }
            return &mData[mIndices[id]];
        }
        Error<C*> at(ID id) noexcept {
            if(id >= maxEntities) {
                return {ErrorTag::FATAL_ERROR, std::format("Invalid ID {} passed to function \"at\"", id)};
            }
            return &mData[mIndices[id]];
        }
        //Unchecked random access of data
        const C& operator[](ID id) const noexcept {
            return mData[mIndices[id]];
        }
        C& operator[](ID id) noexcept {
            return mData[mIndices[id]];
        }
        //Getter for internal data array
        C* data() noexcept {
            return mData;
        }
        const C* data() const noexcept {
            return mData;
        }
        void insert(ID id, const C& comp) noexcept {
            u64 index = reserve(id);
            mData[index] = comp;
        }
        void erase(ID id) noexcept {
            //Swap the data at id for the data at the end
            mData[mIndices[id]] = mData[mSize];
            swapErase(id);
        }
    };
    template<>
    class ComponentSystem<Mesh> : public ComponentSystemParent {
        //Remember, Mesh is a struct of arrays
        Mesh mData;
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
}
