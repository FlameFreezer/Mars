const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const candidateDepthFormats = [_]c.VkFormat{
    c.VK_FORMAT_D32_SFLOAT, c.VK_FORMAT_D32_SFLOAT_S8_UINT, c.VK_FORMAT_D24_UNORM_S8_UINT
};

pub fn init(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) !void {
    const depthFormat = candidateDepthFormats[try chooseDepthFormat(state.physicalDevice)];
    state.depthImage = try Utils.Image.create(state.physicalDevice, state.device, allocator, 
        depthFormat, state.swapchainExtent, c.VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, c.VK_IMAGE_ASPECT_DEPTH_BIT);
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