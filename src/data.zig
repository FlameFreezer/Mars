const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const VertexAttributes: type = enum (u32) {
    COLOR = 0,
    POSITION
};

pub const Vertex = struct {
    color: [3]f32,
    position: [2]f32,

    pub fn create(inColor: [3]f32, inPosition: [2]f32) Vertex {
        return .{
            .position = inPosition,
            .color = inColor
        };
    }

    pub fn inputBindingDescription() c.VkVertexInputBindingDescription {
        return .{
            .binding = 0,
            .stride = @sizeOf(Vertex),
            .inputRate = c.VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    pub fn inputAttributeDescriptions() [@typeInfo(Vertex).@"struct".fields.len]c.VkVertexInputAttributeDescription {
        var attributes: [@typeInfo(Vertex).@"struct".fields.len]c.VkVertexInputAttributeDescription = undefined;
        attributes[@intFromEnum(VertexAttributes.COLOR)] = .{
            .location = @intFromEnum(VertexAttributes.COLOR),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        };
        attributes[@intFromEnum(VertexAttributes.POSITION)] = .{
            .location = @intFromEnum(VertexAttributes.POSITION),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32_SFLOAT,
            .offset = @offsetOf(Vertex, "position")
        };
        return attributes;
    }
};

pub const Vertices = [_]Vertex{
    .create(.{0.0,0.0,0.0}, .{0.5, -0.5}),
    .create(.{0.0,0.0,0.0}, .{0.5, 0.5}),
    .create(.{1.0,0.0,0.0}, .{-0.5, 0.5}),
    .create(.{1.0,0.0,0.0}, .{-0.5, -0.5})
};

pub const Indices = [_]u32{
    0, 1, 3, 1, 2, 3
};
