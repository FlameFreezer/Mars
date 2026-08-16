#include "renderer/mars_gpuimage.h"

#include "renderer/mars_vkhelper.h"

namespace mars {
    VkDevice GPUImage::device{};
    VkPhysicalDevice GPUImage::physicalDevice{};

        GPUImage::GPUImage(GPUImage&& other) noexcept : handle(other.handle), view(other.view), memory(other.memory), width(other.width), height(other.height) {
            other.handle = nullptr;
            other.memory = nullptr;
            other.view = nullptr;
            other.width = 0;
            other.height = 0;
        }
        GPUImage::~GPUImage() noexcept {
            destroy();
        }

        GPUImage& GPUImage::operator=(GPUImage&& other) noexcept {
            if (this != &other) {
                destroy();
                handle = other.handle;
                memory = other.memory;
                view = other.view;
                width = other.width;
                height = other.height;

                other.handle = nullptr;
                other.memory = nullptr;
                other.view = nullptr;
                other.width = 0;
                other.height = 0;
            }
            return *this;
        }

        void GPUImage::destroy() noexcept {
            vkDeviceWaitIdle(device);
            if(view) {
                vkDestroyImageView(device, view, nullptr);
                view = nullptr;
            }
            if (handle) {
                vkDestroyImage(device, handle, nullptr);
                handle = nullptr;
            }
            if (memory) {
                vkFreeMemory(device, memory, nullptr);
                memory = nullptr;
            }
        }
        void GPUImage::attachDevice(VkDevice device, VkPhysicalDevice physicalDevice) noexcept {
            GPUImage::device = device;
            GPUImage::physicalDevice = physicalDevice;
        }
        Error<GPUImage> GPUImage::make(
                VkExtent3D extent,
                VkSampleCountFlagBits sampleCount, 
                VkImageTiling tiling, 
                VkImageUsageFlags usage, 
                VkMemoryPropertyFlags memProperties,
                VkFormat format,
                VkImageAspectFlags aspect
            ) noexcept 
        {
            GPUImage result{};
            result.width = extent.width;
            result.height = extent.height;
            const VkImageCreateInfo imageInfo = {
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
                FATAL("Failed to create image");
            }

            TRY_ASSIGN(result.memory, vkhelper::allocateDeviceMemory(device, physicalDevice, result.handle, memProperties));

            const VkImageViewCreateInfo viewInfo = {
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
                FATAL("Failed to create image view");
            }

            return result;
        }

}