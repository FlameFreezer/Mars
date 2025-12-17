module;

#include <vulkan/vulkan.h>

#include <cstdint>

export module gpubuffer;
import error;

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
        void destroy(VkDevice device) {
            vkDestroyBuffer(device, handle, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
        static Error<std::uint32_t> findPhysicalDeviceMemoryTypeIndex(VkPhysicalDevice physicalDevice, std::uint32_t availableTypes, VkMemoryPropertyFlags memProperties) noexcept {
            VkPhysicalDeviceMemoryProperties2 deviceMemProperties{};
            deviceMemProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
            vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &deviceMemProperties);
            for(std::uint32_t i = 0; i < deviceMemProperties.memoryProperties.memoryTypeCount; i++) {
                std::uint32_t const currentTypeBit = 1U << i;
                if(availableTypes & currentTypeBit and deviceMemProperties.memoryProperties.memoryTypes[i].propertyFlags & memProperties) {
                    return i;
                }
            }
            return {ErrorTag::FATAL_ERROR, "Physical Device does not support needed memory type"};
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
        		.memoryTypeIndex = memType.getData()
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
}