const std = @import("std");
pub const c = @import("c");
const buildOpts = @import("buildOpts");

const Utils = @import("utils.zig");
pub const Math = @import("math.zig");

const Self = @This();

state: Utils.State,

pub fn createMesh(self: *Self, vertices: []const Utils.Vertex, indices: []const u32) !u64 {
    const mesh = try Utils.Mesh.create(&self.state.GPU, vertices, indices, null);
    try self.state.GPU.meshes.put(mesh.id, mesh);
    return mesh.id;
}

pub fn loadMeshes(self: *Self) !void {
    if(self.state.GPU.notFlagSet(Utils.Flags.BEGAN_MESH_LOADING)) return;

    if(c.vkEndCommandBuffer(self.state.GPU.transferCommandBuffer) != c.VK_SUCCESS) {
        return error.failedToEndCommandBuffer;
    }
    const submitInfo = c.VkSubmitInfo2{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 0,
        .signalSemaphoreInfoCount = 0,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &c.VkCommandBufferSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = self.state.GPU.transferCommandBuffer
        }
    };
    if(c.vkQueueSubmit2(self.state.GPU.queues.transferQueue, 1, &submitInfo, self.state.GPU.transferOpFence) != c.VK_SUCCESS) {
        return error.failedToSubmitToQueue;
    }
}

pub usingnamespace Utils;
pub usingnamespace @import("draw.zig");