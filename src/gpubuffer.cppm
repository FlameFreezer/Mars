module;

#include <vulkan/vulkan.h>

export module gpubuffer;
import error;
import vkhelper;

namespace mars {
    export struct GPUBuffer {
        VkBuffer handle;
        VkDeviceMemory memory;

        void destroy(VkDevice device) {
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
            Error<VkDeviceMemory> mem = vkhelper::allocateDeviceMemory(device, physicalDevice, buffer.handle, memProperties);
            if(!mem) return mem.moveError<GPUBuffer>();
            buffer.memory = mem;
            return buffer;
        }
    };
    export template<class T>
    struct UniformBuffer {
        GPUBuffer buffer;
        T* mappedMemory;

        void destroy(VkDevice device) noexcept {
            vkUnmapMemory(device, buffer.memory);
            buffer.destroy(device);
        }
        static Error<UniformBuffer> make(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size) noexcept {
            Error<GPUBuffer> buffer = GPUBuffer::make(
                device, 
                physicalDevice, 
                size, 
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ); 
            if(!buffer) return buffer.moveError<UniformBuffer>();

            UniformBuffer result{buffer.moveData()};

            if(vkMapMemory(device, result.buffer.memory, 0, size, 0, reinterpret_cast<void**>(&result.mappedMemory)) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map device memory to host while creating uniform buffer"};
            }
            return result;
        }
    };
}