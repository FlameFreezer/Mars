#pragma once

#include <vulkan/vulkan.h>

#include "error.h"
#include "mars_vkhelper.h"

namespace mars {
    struct GPUBuffer {
        VkBuffer handle{};
        VkDeviceMemory memory{};
        VkDeviceSize size{};

        void destroy(VkDevice device) {
            if (handle) {
				vkDestroyBuffer(device, handle, nullptr);
            }
            if (memory) {
				vkFreeMemory(device, memory, nullptr);
            }
        }
        static Error<GPUBuffer> make(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties) noexcept {
            GPUBuffer buffer;
            buffer.size = size;
            const VkBufferCreateInfo bufferInfo = {
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
                FATAL("Failed to create VkBuffer while initializing GPUBuffer");
            }
            TRY_ASSIGN(buffer.memory, vkhelper::allocateDeviceMemory(device, physicalDevice, buffer.handle, memProperties));
            return buffer;
        }
    };
    template<class T>
    struct UniformBuffer {
        GPUBuffer buffer{};
        T* mappedMemory{};

        void destroy(VkDevice device) noexcept {
            if (buffer.memory) {
				vkUnmapMemory(device, buffer.memory);
            }
            buffer.destroy(device);
        }
        static Error<UniformBuffer> make(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags2 usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) noexcept {
            Error<GPUBuffer> buffer = GPUBuffer::make(
                device, 
                physicalDevice, 
                size, 
                usage, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ); 
            if (!buffer.okay()) {
                APPEND_SOURCE_INFO(buffer);
                return MOVE_ERROR(buffer);
            }

            UniformBuffer result{buffer.moveValue()};

            if(vkMapMemory(device, result.buffer.memory, 0, size, 0, reinterpret_cast<void**>(&result.mappedMemory)) != VK_SUCCESS) {
                FATAL("Failed to map device memory to host while creating uniform buffer");
            }
            return result;
        }
    };
}
