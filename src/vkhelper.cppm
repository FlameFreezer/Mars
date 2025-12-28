module;

#include <vulkan/vulkan.h>

#include <cstdint>

export module vkhelper;
import error;

using mars::Error;
using mars::ErrorTag;

namespace vkhelper {
    Error<std::uint32_t> findPhysicalDeviceMemoryTypeIndex(VkPhysicalDevice physicalDevice, std::uint32_t availableTypes, VkMemoryPropertyFlags memProperties) noexcept {
        VkPhysicalDeviceMemoryProperties2 deviceMemProperties{};
        deviceMemProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
        vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &deviceMemProperties);
        for(std::uint32_t i = 0; i < deviceMemProperties.memoryProperties.memoryTypeCount; i++) {
            std::uint32_t const currentTypeBit = 1U << i;
            if(availableTypes & currentTypeBit and deviceMemProperties.memoryProperties.memoryTypes[i].propertyFlags & memProperties) {
                return i;
            }
        }
        return {ErrorTag::FATAL_ERROR, "Physical device does not support needed memory type"};
    }
    export Error<VkDeviceMemory> allocateDeviceMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags memProperties) noexcept {
        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
        Error<std::uint32_t> memType = findPhysicalDeviceMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, memProperties);
        if(!memType) return memType.moveError<VkDeviceMemory>();

        VkMemoryAllocateInfo const allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memType
        };
        VkDeviceMemory memory = nullptr;
        if(vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            return {ErrorTag::FATAL_ERROR, "Failed to allocate device memory"};
        }
        if(vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
            return {ErrorTag::FATAL_ERROR, "Failed to bind buffer memory"};
        }
        return memory;
    }
    export Error<VkDeviceMemory> allocateDeviceMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkImage image, VkMemoryPropertyFlags memProperties) noexcept {
        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(device, image, &memRequirements);
        Error<std::uint32_t> memType = findPhysicalDeviceMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, memProperties);
        if(!memType) return memType.moveError<VkDeviceMemory>();

        VkMemoryAllocateInfo const allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = memType
        };
        VkDeviceMemory memory = nullptr;
        if(vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            return {ErrorTag::FATAL_ERROR, "Failed to allocate device memory"};
        }
        if(vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
            return {ErrorTag::FATAL_ERROR, "Failed to bind image memory"};
        }
        return memory;
    }
}