#pragma once

#include <cstdint>
#include <string>

#include <vulkan/vulkan.h>

#include "error.h"

namespace vkhelper {
    Error<VkDeviceMemory> allocateDeviceMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags memProperties) noexcept;
    Error<VkDeviceMemory> allocateDeviceMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkImage image, VkMemoryPropertyFlags memProperties) noexcept;
    std::string messageSeverityToString(VkDebugUtilsMessageSeverityFlagBitsEXT severity) noexcept;
}
