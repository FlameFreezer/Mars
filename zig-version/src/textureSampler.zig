const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn init(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) !void {
    const samplerInfo = c.VkSamplerCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = c.VK_FILTER_LINEAR,
        .minFilter = c.VK_FILTER_LINEAR,
        .addressModeU = c.VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = c.VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = c.VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = c.VK_FALSE,
        .unnormalizedCoordinates = c.VK_FALSE,
        .mipLodBias = 0.0,
        .compareEnable = c.VK_FALSE,
        .borderColor = c.VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    };
    if(c.vkCreateSampler(state.device, &samplerInfo, allocator, &state.textureSampler) != c.VK_SUCCESS) {
        return error.failedToCreateTextureSampler;
    }
}

pub fn destroy(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroySampler(state.device, state.textureSampler, allocator);
}