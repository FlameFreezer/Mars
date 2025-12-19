module;

#include <vulkan/vulkan.h>

#include <cstdint>

export module gpubuffer;
import error;
import vkhelper;

namespace mars {
    export struct GPUBuffer {
        VkBuffer handle;
        VkDeviceMemory memory;

        GPUBuffer() noexcept : handle(nullptr), memory(nullptr)  {}
        GPUBuffer(GPUBuffer const& other) noexcept = delete;
        GPUBuffer(GPUBuffer&& other) noexcept : handle(other.handle), memory(other.memory) {
            other.handle = nullptr;
            other.memory = nullptr;
        }
        virtual void destroy(VkDevice device) {
            vkDestroyBuffer(device, handle, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
        static Error<GPUBuffer> make(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties) noexcept {
            GPUBuffer buffer;
            VkBufferCreateInfo const bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = size,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr
            };
            if(vkCreateBuffer(device, &bufferInfo, nullptr, &buffer.handle) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create VkBuffer while initializing GPUBuffer"};
            }
            VkMemoryRequirements memRequirements{};
            vkGetBufferMemoryRequirements(device, buffer.handle, &memRequirements);
            Error<std::uint32_t> memType = findPhysicalDeviceMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, memProperties);
            if(!memType.okay()) return memType.moveError<GPUBuffer>();

            VkMemoryAllocateInfo const allocInfo = {
        		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        		.pNext = nullptr,
        		.allocationSize = memRequirements.size,
        		.memoryTypeIndex = memType.data()
            };
            if(vkAllocateMemory(device, &allocInfo, nullptr, &buffer.memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to allocate device memory while initializing GPUBuffer"};
            }
            if(vkBindBufferMemory(device, buffer.handle, buffer.memory, 0) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to bind buffer memory while initializing GPUBuffer!"};
            }
            return buffer;
        }
        GPUBuffer& operator=(GPUBuffer&& rhs) {
            if(this != &rhs) {
                handle = rhs.handle;
                memory = rhs.memory;
                rhs.handle = nullptr;
                rhs.memory = nullptr;
            }
            return *this;
        }
        GPUBuffer& operator=(GPUBuffer const& rhs) = delete;
    };
    export template<class T>
    struct UniformBuffer : public GPUBuffer {
        T* mappedMemory;
        UniformBuffer() noexcept : mappedMemory(nullptr) {}
        static Error<UniformBuffer> make(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size) noexcept {
            Error<GPUBuffer> buffer = GPUBuffer::make(
                device, 
                physicalDevice, 
                size, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ); 
            if(!buffer.okay()) return buffer.moveError<UniformBuffer>();

            UniformBuffer result;
            *reinterpret_cast<GPUBuffer*>(&result) = buffer.moveData();

            if(vkMapMemory(device, result.memory, 0, size, 0, reinterpret_cast<void**>(&result.mappedMemory)) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map device memory to host while creating uniform buffer"};
            }
            return result;
        }

        void destroy(VkDevice device) override {
            vkUnmapMemory(device, memory);
            this->GPUBuffer::destroy(device);
        }
    };
}