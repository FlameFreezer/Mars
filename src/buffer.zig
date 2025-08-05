const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

const Data = @import("data.zig");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    const sizeVertexBuffer: u32 = @sizeOf(@TypeOf(Data.Vertices));
    const sizeIndexBuffer: u32 = @sizeOf(@TypeOf(Data.Indices)); 
    const size: u32 = comptime sizeVertexBuffer + sizeIndexBuffer; 
    const usage: c.VkBufferUsageFlags = c.VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | c.VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            | c.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
            | c.VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    state.buffer = try Utils.Buffer.create(state.physicalDevice, state.device, allocator, size, usage, 
        c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    state.uniformBuffer = try Utils.Buffer.create(state.physicalDevice, state.device, allocator, 
        @sizeOf(Utils.UniformBufferObject) * Utils.MAX_FRAMES_IN_FLIGHT, c.VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    if(c.vkMapMemory(state.device, state.uniformBuffer.memory, 0, 
        @sizeOf(Utils.UniformBufferObject) * Utils.MAX_FRAMES_IN_FLIGHT, 0, @ptrCast(&state.uniformBufferMapped.ptr)
    ) != c.VK_SUCCESS) {
        return error.failedToMapMemory;
    }
    state.uniformBufferMapped.len = Utils.MAX_FRAMES_IN_FLIGHT;
    for(state.uniformBufferMapped) |*uniformBufferObject| {
        uniformBufferObject.* = Utils.UniformBufferObject.default;
    }

    var stagingBuffer = try Utils.Buffer.create(state.physicalDevice, state.device, 
        allocator, size, c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    defer stagingBuffer.destroy(state.device, allocator);

    var memory: ?*anyopaque = null;

    //Map region for the Vertices
    if(c.vkMapMemory(state.device, stagingBuffer.memory, 0, sizeVertexBuffer, 0, &memory) != c.VK_SUCCESS) {
        return error.failedToMapMemory;
    }
    @memcpy(@as([*]Utils.Vertex, @ptrCast(@alignCast(memory))), &Data.Vertices);
    c.vkUnmapMemory(state.device, stagingBuffer.memory);

    //Map region for the Indices
    if(c.vkMapMemory(state.device, stagingBuffer.memory, sizeVertexBuffer, sizeIndexBuffer, 0, &memory) != c.VK_SUCCESS) {
        return error.failedToMapMemory;
    }
    @memcpy(@as([*]u32, @ptrCast(@alignCast(memory))), &Data.Indices);
    c.vkUnmapMemory(state.device, stagingBuffer.memory);

    const commandBuffer: c.VkCommandBuffer = try Utils.beginSingleTimeCommandBuffer(state.device, state.commandPool);
    c.vkCmdCopyBuffer(commandBuffer, stagingBuffer.handle, state.buffer.handle, 1, &.{.srcOffset = 0, .dstOffset = 0, .size = size});
    if(c.vkEndCommandBuffer(commandBuffer) != c.VK_SUCCESS) {
        return error.failedToEndSingleTimeCommandBuffer;
    }

    const submitInfo = c.VkSubmitInfo2{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 0,
        .signalSemaphoreInfoCount = 0,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &.{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = commandBuffer,
            .deviceMask = 0
        }
    };

    if(c.vkQueueSubmit2(state.queues.graphics, 1, &submitInfo, null) != c.VK_SUCCESS) {
        return error.failedToSubmitToQueue;
    }
    _ = c.vkQueueWaitIdle(state.queues.graphics);
    c.vkFreeCommandBuffers(state.device, state.commandPool, 1, &commandBuffer);
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    state.buffer.destroy(state.device, allocator);
    c.vkUnmapMemory(state.device, state.uniformBuffer.memory);
    state.uniformBuffer.destroy(state.device, allocator);
}


