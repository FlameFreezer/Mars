module;

#include <vulkan/vulkan.h>

export module gpuimage;
import error;
import vkhelper;

namespace mars {
    export struct GPUImage {
        VkImage handle;
        VkImageView view;
        VkDeviceMemory memory;

        void destroy(VkDevice device) noexcept {
            vkDestroyImageView(device, view, nullptr);
            vkDestroyImage(device, handle, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
        static Error<GPUImage> make(
                VkDevice device, 
                VkPhysicalDevice physicalDevice, 
                VkExtent3D extent,
                VkSampleCountFlagBits sampleCount, 
                VkImageTiling tiling, 
                VkImageUsageFlags usage, 
                VkMemoryPropertyFlags memProperties,
                VkFormat format,
                VkImageAspectFlags aspect
            ) noexcept {
            GPUImage result{};
            VkImageCreateInfo const imageInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = format,
                .extent = extent,
                .mipLevels = 1, 
                .arrayLayers = 1,
                .samples = sampleCount,
                .tiling = tiling,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            };
            if(vkCreateImage(device, &imageInfo, nullptr, &result.handle) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create image"};
            }

            Error<VkDeviceMemory> mem = vkhelper::allocateDeviceMemory(device, physicalDevice, result.handle, memProperties);
            if(!mem) return mem.moveError<GPUImage>();
            result.memory = mem;

            VkImageViewCreateInfo const viewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = result.handle,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = aspect,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            if(vkCreateImageView(device, &viewInfo, nullptr, &result.view) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create image view"};
            }

            return result;
        }
    };
}