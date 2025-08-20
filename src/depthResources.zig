const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const candidateDepthFormats = [_]c.VkFormat{
    c.VK_FORMAT_D32_SFLOAT, c.VK_FORMAT_D32_SFLOAT_S8_UINT, c.VK_FORMAT_D24_UNORM_S8_UINT
};

pub fn init(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) !void {
    const depthFormatIndex = try chooseDepthFormat(state.physicalDevice);
    const depthImageInfo = c.VkImageCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = c.VK_IMAGE_TYPE_2D,
        .format = candidateDepthFormats[depthFormatIndex],
        .tiling = c.VK_IMAGE_TILING_OPTIMAL,
        .extent = .{
            .width = state.swapchainExtent.width,
            .height = state.swapchainExtent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = c.VK_SAMPLE_COUNT_1_BIT,
        .usage = c.VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE
    };
    if(c.vkCreateImage(state.device, &depthImageInfo, allocator, &state.depthImage) != c.VK_SUCCESS) {
        return error.failedToCreateDepthImage;
    }

    var memRequirements: c.VkMemoryRequirements = undefined;
    _ = c.vkGetImageMemoryRequirements(state.device, state.depthImage, &memRequirements);

    const allocInfo = c.VkMemoryAllocateInfo{
        .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = try Utils.findPhysicalDeviceMemoryTypeIndex(state.physicalDevice, memRequirements.memoryTypeBits, c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    };

    if(c.vkAllocateMemory(state.device, &allocInfo, allocator, &state.depthImageMemory) != c.VK_SUCCESS) {
        return error.failedToAllocateMemory;
    }
    _ = c.vkBindImageMemory(state.device, state.depthImage, state.depthImageMemory, 0);

    const depthImageViewInfo = c.VkImageViewCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = state.depthImage,
        .viewType = c.VK_IMAGE_VIEW_TYPE_2D,
        .format = candidateDepthFormats[depthFormatIndex],
        .components = c.VkComponentMapping{
            .r = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = c.VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = c.VkImageSubresourceRange{
            .aspectMask = c.VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    if(c.vkCreateImageView(state.device, &depthImageViewInfo, allocator, &state.depthImageView) != c.VK_SUCCESS) {
        return error.failedToCreateDepthImageView;
    }
}

pub fn destroy(state: *const Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroyImageView(state.device, state.depthImageView, allocator);
    c.vkDestroyImage(state.device, state.depthImage, allocator);
    c.vkFreeMemory(state.device, state.depthImageMemory, allocator);
}

fn chooseDepthFormat(physicalDevice: c.VkPhysicalDevice) !usize {
    for(0..candidateDepthFormats.len) |i| {
        var formatProperties: c.VkFormatProperties = undefined;
        _ = c.vkGetPhysicalDeviceFormatProperties(physicalDevice, candidateDepthFormats[i], &formatProperties);
        if(formatProperties.optimalTilingFeatures & c.VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL != 0) {
            return i;
        }
    }
    return error.failedToFindSuitableDepthFormat;
}