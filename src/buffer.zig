const c = @import("c");
const std = @import("std");

const Utils = @import("utils.zig");
const Data = @import("data.zig");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    const size: u32 = @sizeOf(@TypeOf(Data.Vertices)) + @sizeOf(@TypeOf(Data.Indices)); 
    const usage: c.VkBufferUsageFlags = c.VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | c.VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            | c.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    state.buffer = try createBuffer(state.physicalDevice, state.device, allocator, size, usage, 
        c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    var stagingBuffer: Utils.Buffer = try createBuffer(state.physicalDevice, state.device, 
        allocator, size, c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    defer stagingBuffer.Destroy(state.device, allocator);

    //var memory: ?*anyopaque = null;
    var memory: ?[*]Data.Vertex = null;

    //Map region for the Vertices
    const vertexStagingSize: u32 = @sizeOf(@TypeOf(Data.Vertices));
    if(c.vkMapMemory(state.device, stagingBuffer.memory, 0, vertexStagingSize, 0, @ptrCast(&memory)) != c.VK_SUCCESS) {
        return error.failed_to_map_memory;
    }
    //@memcpy(@as([*]Data.Vertex, @ptrCast(@alignCast(memory))), &Data.Vertices);
    @memcpy(memory.?, &Data.Vertices);
    std.debug.assert(memory.?[0].color[0] == Data.Vertices[0].color[0]);
    c.vkUnmapMemory(state.device, stagingBuffer.memory);

    //Map region for the Indices
    var mem2: ?[*]u32 = null;
    const indexStagingSize: u32 = @sizeOf(@TypeOf(Data.Indices));
    if(c.vkMapMemory(state.device, stagingBuffer.memory, vertexStagingSize, indexStagingSize, 0, @ptrCast(&mem2)) != c.VK_SUCCESS) {
        return error.failed_to_map_memory;
    }
    //@memcpy(@as([*]u32, @ptrCast(@alignCast(memory))), &Data.Indices);
    @memcpy(mem2.?, &Data.Indices);
    std.debug.assert(mem2.?[0] == Data.Indices[0]);
    c.vkUnmapMemory(state.device, stagingBuffer.memory);

    const commandBuffer: c.VkCommandBuffer = try Utils.beginSingleTimeCommandBuffer(state.device, state.commandPool);
    c.vkCmdCopyBuffer(commandBuffer, stagingBuffer.handle, state.buffer.handle, 1, &.{.srcOffset = 0, .dstOffset = 0, .size = size});
    if(c.vkEndCommandBuffer(commandBuffer) != c.VK_SUCCESS) {
        return error.failed_to_end_single_time_command_buffer;
    }

    var submitInfo = c.VkSubmitInfo2{
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
        return error.failed_to_submit_to_queue;
    }
    _ = c.vkQueueWaitIdle(state.queues.graphics);
    c.vkFreeCommandBuffers(state.device, state.commandPool, 1, &commandBuffer);
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    state.buffer.Destroy(state.device, allocator);
}

fn findPhysicalDeviceMemoryTypeIndex(physical: c.VkPhysicalDevice, availableTypes: u32, 
    properties: c.VkMemoryPropertyFlags
) !u32 {
    var deviceMemoryProperties = c.VkPhysicalDeviceMemoryProperties2{
        .sType = c.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
    };
    c.vkGetPhysicalDeviceMemoryProperties2(physical, &deviceMemoryProperties);

    for(deviceMemoryProperties.memoryProperties.memoryTypes, 0..) |memType, i| {
        const currentTypeBit: u32 = @as(u32, @as(u32, 1) << @intCast(i));
        if(memType.propertyFlags & properties == properties and availableTypes & currentTypeBit != 0) {
            return @intCast(i);
        }
    }

    return error.failed_to_find_memory_type;
}

fn createBuffer(physical: c.VkPhysicalDevice, device: c.VkDevice, 
    allocator: ?*c.VkAllocationCallbacks, size: u32, usage: c.VkBufferUsageFlags, 
    properties: c.VkMemoryPropertyFlags) !Utils.Buffer 
{
    var result: Utils.Buffer = undefined;
    const bufferInfo: c.VkBufferCreateInfo = .{
        .sType = c.VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE
    };
    if(c.vkCreateBuffer(device, &bufferInfo, allocator, &result.handle) != c.VK_SUCCESS) {
        return error.failed_to_create_buffer;
    }

    var memoryRequirements: c.VkMemoryRequirements = undefined;
    c.vkGetBufferMemoryRequirements(device, result.handle, &memoryRequirements);

    const memoryAllocationInfo = c.VkMemoryAllocateInfo{
        .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = try findPhysicalDeviceMemoryTypeIndex(physical, memoryRequirements.memoryTypeBits, properties)
    };
    if(c.vkAllocateMemory(device, &memoryAllocationInfo, allocator, &result.memory) != c.VK_SUCCESS) {
        return error.failed_to_allocate_memory;
    }
    if(c.vkBindBufferMemory(device, result.handle, result.memory, 0) != c.VK_SUCCESS) {
        return error.failed_to_bind_buffer_memory;
    }
    return result;
}
