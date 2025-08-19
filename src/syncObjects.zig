const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    //  Each swapchain image gets its own presentation semaphore
    //  Each frame gets its own image acquisition semaphore
    state.semaphores = try std.heap.page_allocator.alloc(c.VkSemaphore, state.swapchainImages.len + Utils.MAX_FRAMES_IN_FLIGHT);

    //  Create standard per-frame fences
    var fenceInfo = c.VkFenceCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = c.VK_FENCE_CREATE_SIGNALED_BIT
    };
    for(&state.fences) |*fence| {
        if(c.vkCreateFence(state.device, &fenceInfo, allocator, fence) != c.VK_SUCCESS) {
            return error.failedToCreateFence;
        }
    }

    //  Create transfer operation fence, which is expected to start unsignaled
    fenceInfo.flags = 0;
    if(c.vkCreateFence(state.*.device, &fenceInfo, allocator, &state.*.transferOpFence) != c.VK_SUCCESS) {
        return error.failedToCreateFence;
    }

    //  Create semaphores
    for(state.semaphores) |*semaphore| {
        if(c.vkCreateSemaphore(state.device, &.{.sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}, allocator, semaphore) != c.VK_SUCCESS) {
            return error.failedToCreateSemaphore;
        }
     }
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    for(state.fences) |fence| {
        c.vkDestroyFence(state.device, fence, allocator);
    }
    c.vkDestroyFence(state.*.device, state.*.transferOpFence, allocator);
    for(state.semaphores) |semaphore| {
        c.vkDestroySemaphore(state.device, semaphore, allocator);
    }
    std.heap.page_allocator.free(state.semaphores);
}
