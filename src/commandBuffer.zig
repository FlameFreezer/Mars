const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    const queueFamilyIndices = try Utils.findQueueFamilyIndices(state.physicalDevice, state.surface);
    const commandPoolInfo = c.VkCommandPoolCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = c.VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsIndex.?
    };

    if(c.vkCreateCommandPool(state.device, &commandPoolInfo, allocator, &state.commandPool) != c.VK_SUCCESS) {
        return error.failedToCreateCommandPool;
    }

    const commandBufferInfo = c.VkCommandBufferAllocateInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = state.commandBuffers.len,
        .commandPool = state.commandPool,
        .level = c.VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };

    if(c.vkAllocateCommandBuffers(state.device, &commandBufferInfo, &state.commandBuffers[0]) != c.VK_SUCCESS) {
        return error.failedToAllocateCommandBuffers;
    }

}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkFreeCommandBuffers(state.device, state.commandPool, state.commandBuffers.len, &state.commandBuffers[0]);
    c.vkDestroyCommandPool(state.device, state.commandPool, allocator);
}
