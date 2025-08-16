const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn init(state: *const Utils.State, vertices: []const Utils.Vertex, indices: []const u32, 
    allocator: ?*c.VkAllocationCallbacks) 
!Utils.Mesh {
    const commandBuffer = try Utils.beginSingleTimeCommandBuffer(state.device, state.commandPool);

    var stagingBuffer: Utils.Buffer = undefined;

    const resultMesh = try createMesh(state, &stagingBuffer, commandBuffer, vertices, indices);

    if(c.vkEndCommandBuffer(commandBuffer) != c.VK_SUCCESS) {
        return error.failedToEndCommandBuffer;
    }

    const submitInfo = c.VkSubmitInfo2{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 0,
        .signalSemaphoreInfoCount = 0,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &.{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = commandBuffer
        }
    };
    if(c.vkQueueSubmit2(state.queues.graphics, 1, &submitInfo, null) != c.VK_SUCCESS) {
        return error.failedToSubmitToQueue;
    }

    _ = c.vkQueueWaitIdle(state.queues.graphics);

    c.vkFreeCommandBuffers(state.device, state.commandPool, 1, &commandBuffer);
    stagingBuffer.destroy(state.device, allocator);
    return resultMesh;
}

pub fn createMesh(state: *const Utils.State, stagingBuffer: *Utils.Buffer, 
    commandBuffer: c.VkCommandBuffer, vertices: []const Utils.Vertex, indices: []const u32) 
!Utils.Mesh {
    var result: Utils.Mesh = undefined;
    result.verticesSize = @as(u32, @intCast(vertices.len)) * @sizeOf(Utils.Vertex);
    result.indicesSize = @as(u32, @intCast(indices.len)) * @sizeOf(u32);

    result.buffer = try Utils.Buffer.create(state.physicalDevice, state.device, null, 
        result.verticesSize + result.indicesSize, 
        c.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
            | c.VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
            | c.VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    stagingBuffer.* = try Utils.Buffer.create(state.physicalDevice, state.device, null,
        result.verticesSize + result.indicesSize, 
        c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    var memory: ?*anyopaque = null;
    if(c.vkMapMemory(state.device, stagingBuffer.memory, 0, result.verticesSize, 0, &memory) != c.VK_SUCCESS) {
        return error.failedToMapMemory;
    }
    @memcpy(@as([*]Utils.Vertex, @ptrCast(@alignCast(memory))), vertices);
    c.vkUnmapMemory(state.device, stagingBuffer.memory);

    if(c.vkMapMemory(state.device, stagingBuffer.memory, result.verticesSize, result.indicesSize, 0, &memory) != c.VK_SUCCESS) {
        return error.failedToMapMemory;
    }
    @memcpy(@as([*]u32, @ptrCast(@alignCast(memory))), indices);
    c.vkUnmapMemory(state.device, stagingBuffer.memory);

    c.vkCmdCopyBuffer(commandBuffer, stagingBuffer.handle, result.buffer.handle, 1, 
        &.{
            .srcOffset = 0, 
            .dstOffset = 0, 
            .size = result.verticesSize + result.indicesSize
        }
    );

    return result;
}

pub const cubeVertices = makeVertices:{
    const white = [3]f32{1.0, 1.0, 1.0};
    const red = [3]f32{1.0, 0.0, 0.0};
    const green = [3]f32{0.0, 1.0, 0.0};
    const blue = [3]f32{0.0, 0.0, 1.0};
    const yellow = [3]f32{1.0, 1.0, 0.0};
    const purple = [3]f32{1.0, 0.0, 1.0};

    const frontFace = [4][3]f32{
        .{0.0, 0.0, 0.0},
        .{1.0, 0.0, 0.0},
        .{1.0, -1.0, 0.0},
        .{0.0, -1.0, 0.0}
    };
    const backFace = [4][3]f32{
        .{0.0, 0.0, 1.0},
        .{1.0, 0.0, 1.0},
        .{1.0, -1.0, 1.0},
        .{0.0, -1.0, 1.0}
    };

    break:makeVertices [_]Utils.Vertex{
        //FRONT FACE
        .create(white,frontFace[0]),
        .create(white,frontFace[1]),
        .create(white,frontFace[2]),
        .create(white,frontFace[3]),
        //BACK FACE
        .create(purple,backFace[0]),
        .create(purple,backFace[1]),
        .create(purple,backFace[2]),
        .create(purple,backFace[3]),
        //RIGHT FACE
        .create(yellow,frontFace[1]),
        .create(yellow,backFace[1]),
        .create(yellow,backFace[2]),
        .create(yellow,frontFace[2]),
        //LEFT FACE
        .create(green,frontFace[0]),
        .create(green,backFace[0]),
        .create(green,backFace[3]),
        .create(green,frontFace[3]),
        //TOP FACE
        .create(red,backFace[0]),
        .create(red,backFace[1]),
        .create(red,frontFace[1]),
        .create(red,frontFace[0]),
        //BOTTOM FACE
        .create(blue,backFace[3]),
        .create(blue,backFace[2]),
        .create(blue,frontFace[2]),
        .create(blue,frontFace[3]),
    };
};

pub const cubeIndices = [_]u32{
    //FRONT FACE
    0, 1, 2,
    0, 2, 3,
    //BACK FACE
    4, 5, 6,
    4, 6, 7,
    //RIGHT FACE
    8, 9, 10,
    8, 10, 11,
    //LEFT FACE
    12, 13, 14, 
    12, 14, 15,
    //TOP FACE
    16, 17, 18,
    16, 18, 19,
    //BOTTOM FACE
    20, 21, 22,
    20, 22, 23
};