module;

#include <vulkan/vulkan.h>

#include <cstdint>

export module vkhelper;
import error;

namespace mars {
    export Error<std::uint32_t> findPhysicalDeviceMemoryTypeIndex(VkPhysicalDevice physicalDevice, std::uint32_t availableTypes, VkMemoryPropertyFlags memProperties) noexcept {
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
}