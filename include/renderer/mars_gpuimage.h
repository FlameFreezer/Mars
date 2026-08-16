#pragma once

#include <vulkan/vulkan.h>

#include <error.h>

namespace mars {
    struct GPUImage {
        static VkDevice device;
        static VkPhysicalDevice physicalDevice;

        VkImage handle{};
        VkImageView view{};
        VkDeviceMemory memory{};
        u32 width = 0;
        u32 height = 0;

        GPUImage() noexcept = default;
        GPUImage(GPUImage&& other) noexcept;
        ~GPUImage() noexcept;

        void destroy() noexcept;

        GPUImage& operator=(GPUImage&& other) noexcept;

        static void attachDevice(VkDevice device, VkPhysicalDevice physicalDevice) noexcept;

        static Error<GPUImage> make(
                VkExtent3D extent,
                VkSampleCountFlagBits sampleCount, 
                VkImageTiling tiling, 
                VkImageUsageFlags usage, 
                VkMemoryPropertyFlags memProperties,
                VkFormat format,
                VkImageAspectFlags aspect
            ) noexcept; 
    };
}
