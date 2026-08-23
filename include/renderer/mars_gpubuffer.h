#pragma once

#include <vulkan/vulkan.h>

#include <error.h>

namespace mars {
    struct GPUBuffer {
        static VkDevice device;
        static VkPhysicalDevice physicalDevice;
        VkBuffer handle{};
        VkDeviceMemory memory{};
        VkDeviceSize size{};

        GPUBuffer() noexcept = default;
        GPUBuffer(GPUBuffer&& other) noexcept;
        ~GPUBuffer() noexcept;

        static void attachDevice(VkDevice device, VkPhysicalDevice physicalDevice) noexcept;

        void destroy() noexcept;

        static Error<GPUBuffer> make(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties) noexcept;

        GPUBuffer& operator=(GPUBuffer&& other) noexcept;
    };
    template<class T>
    struct UniformBuffer {
        GPUBuffer buffer{};
        T* mappedMemory{};

        UniformBuffer() noexcept = default;
        UniformBuffer(UniformBuffer&& other) noexcept : buffer(std::move(other.buffer)), mappedMemory(other.mappedMemory) {
            other.mappedMemory = nullptr;
        }
        ~UniformBuffer() noexcept {
            destroy();
        }

        UniformBuffer& operator=(UniformBuffer&& other) noexcept {
            if (this != &other) {
                buffer = std::move(other.buffer);
                mappedMemory = other.mappedMemory;

                other.mappedMemory = nullptr;
            }
            return *this;
        }

        void destroy() noexcept {
            if (buffer.memory != nullptr && mappedMemory != nullptr) {
				vkUnmapMemory(buffer.device, buffer.memory);
                mappedMemory = nullptr;
            }
            buffer.destroy();
        }
        static Error<UniformBuffer> make(VkDeviceSize size, VkBufferUsageFlags2 usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) noexcept {
            Error<GPUBuffer> buffer = GPUBuffer::make(
                size, 
                usage, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            ); 
            if (!buffer.okay()) {
                APPEND_SOURCE_INFO(buffer);
                return MOVE_ERROR(buffer);
            }

            UniformBuffer result;
            result.buffer = buffer.moveValue();

            if(vkMapMemory(result.buffer.device, result.buffer.memory, 0, size, 0, reinterpret_cast<void**>(&result.mappedMemory)) != VK_SUCCESS) {
                FATAL("Failed to map device memory to host while creating uniform buffer");
            }
            return result;
        }
    };
}
