module;

#include <cstring>

#include <vulkan/vulkan.h>

export module maps;
export import multimap;

namespace mars {
    export struct BufferSizes {
        VkDeviceSize vertices;
        VkDeviceSize indices;
    };

    export class VertexBuffers : public ArrayMultimap {
        void realloc() noexcept {
            this->ArrayMultimap::realloc();
            const std::size_t cap = this->capacity();
            const std::size_t s = this->size();

            VkBuffer* newHandles = new VkBuffer[cap];
            VkDeviceMemory* newMemories = new VkDeviceMemory[cap];
            BufferSizes* newSizes = new BufferSizes[cap];

            std::memcpy(newHandles, handles, s);
            std::memcpy(newMemories, memories, s);
            std::memcpy(newSizes, sizes, s);

            delete[] handles;
            delete[] memories;
            delete[] sizes;

            handles = newHandles;
            memories = newMemories;
            sizes = newSizes;
        }

        public:
        VkBuffer* handles;
        VkDeviceMemory* memories;
        BufferSizes* sizes;

        VertexBuffers() noexcept : ArrayMultimap() {
            handles = new VkBuffer[this->capacity()];
            memories = new VkDeviceMemory[this->capacity()];
            sizes = new BufferSizes[this->capacity()];
        }

        VertexBuffers(std::size_t cap) noexcept : ArrayMultimap(cap) {
            handles = new VkBuffer[this->capacity()];
            memories = new VkDeviceMemory[this->capacity()];
            sizes = new BufferSizes[this->capacity()];
        }

        ~VertexBuffers() noexcept {
            delete[] handles;
            delete[] memories;
            delete[] sizes;
        }

        ID append(VkBuffer h, VkDeviceMemory m, BufferSizes s) noexcept {
            if(this->size() == this->capacity()) {
                realloc();
            }
            handles[this->size()] = h;
            memories[this->size()] = m;
            sizes[this->size()] = s;
            return this->ArrayMultimap::append();
        }
        //Removes the object from the map with id. Does not perform any memory cleanup or destruction
        void remove(ID id) noexcept {
            const std::size_t index = this->getIndex(id);
            //Swap element at end of array to index
            const std::size_t back = this->size() - 1;
            handles[index] = handles[back];
            memories[index] = memories[back];
            sizes[index] = sizes[back];
            //Fix the ID in the backing array of indices and reduce size
            this->setIndexForID(id, index);
        }
    };

    export class Textures : public ArrayMultimap {
        void realloc() noexcept {
            //Reallocate backing multimap
            this->ArrayMultimap::realloc();
            const std::size_t cap = this->capacity();
            const std::size_t s = this->size();
            //Allocate arrays
            VkImage* newHandles = new VkImage[cap];
            VkDeviceMemory* newMemories = new VkDeviceMemory[cap];
            VkImageView* newViews = new VkImageView[cap];
            //Copy data to new arrays
            std::memcpy(newHandles, handles, s);
            std::memcpy(newMemories, memories, s);
            std::memcpy(newViews, views, s);
            //Delete old arrays
            delete[] handles;
            delete[] memories;
            delete[] views;
            //Reassign arrays
            handles = newHandles;
            memories = newMemories;
            views = newViews;
        }
        public:
        VkImage* handles;
        VkDeviceMemory* memories;
        VkImageView* views;

        Textures() noexcept : ArrayMultimap() {
            handles = new VkImage[this->capacity()];
            memories = new VkDeviceMemory[this->capacity()];
            views = new VkImageView[this->capacity()];
        }

        Textures(std::size_t cap) noexcept : ArrayMultimap(cap) {
            handles = new VkImage[this->capacity()];
            memories = new VkDeviceMemory[this->capacity()];
            views = new VkImageView[this->capacity()];
        }

        ~Textures() noexcept {
            delete[] handles;
            delete[] memories;
            delete[] views;
        }

        ID append(VkImage h, VkDeviceMemory m, VkImageView v) noexcept {
            if(this->size() == this->capacity()) {
                realloc();
            }
            handles[this->size()] = h;
            memories[this->size()] = m;
            views[this->size()] = v;
            return this->ArrayMultimap::append();
        }
    };
}
