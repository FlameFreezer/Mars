#include "renderer/mars_gpubuffer.h"

#include "renderer/mars_vkhelper.h"

namespace mars {
    VkDevice GPUBuffer::device{};
    VkPhysicalDevice GPUBuffer::physicalDevice{};

    GPUBuffer::GPUBuffer(GPUBuffer&& other) noexcept : handle(other.handle), memory(other.memory), size(other.size) {
        other.handle = nullptr;
        other.memory = nullptr;
        other.size = 0;
    }

    GPUBuffer::~GPUBuffer() noexcept {
        destroy();
    }

    GPUBuffer& GPUBuffer::operator=(GPUBuffer&& other) noexcept {
        if (this != &other) {
            destroy();
            handle = other.handle;
            memory = other.memory;
            size = other.size;

            other.handle = nullptr;
            other.memory = nullptr;
            other.size = 0;
        }

        return *this;
    }

    void GPUBuffer::attachDevice(VkDevice device, VkPhysicalDevice physicalDevice) noexcept {
        GPUBuffer::device = device;
        GPUBuffer::physicalDevice = physicalDevice;
    }
    void GPUBuffer::destroy() noexcept {
        vkDeviceWaitIdle(device);
        if (handle != nullptr) {
            vkDestroyBuffer(device, handle, nullptr);
            handle = nullptr;
        }
        if (memory != nullptr) {
            vkFreeMemory(device, memory, nullptr);
            memory = nullptr;
        }
        size = 0;
    }
    Error<GPUBuffer> GPUBuffer::make(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memProperties) noexcept {
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

}