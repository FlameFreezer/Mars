const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

const VertexAttributes: type = enum (u32) {
    COLOR = 0,
    POSITION
};

pub const Vertex = struct {
    color: [3]f32,
    position: [3]f32,

    const numAttribs: usize = @typeInfo(@This()).@"struct".fields.len;

    pub fn create(inColor: [3]f32, inPosition: [3]f32) Vertex {
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

    pub fn inputAttributeDescriptions() [numAttribs]c.VkVertexInputAttributeDescription {
        comptime var attributes: [numAttribs]c.VkVertexInputAttributeDescription = undefined;
        comptime attributes[@intFromEnum(VertexAttributes.COLOR)] = .{
            .location = @intFromEnum(VertexAttributes.COLOR),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        };
        comptime attributes[@intFromEnum(VertexAttributes.POSITION)] = .{
            .location = @intFromEnum(VertexAttributes.POSITION),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32B32_SFLOAT,
            .offset = @offsetOf(Vertex, "position")
        };
        return attributes;
    }
};

pub const Vertices = [_]Vertex{
    .create(.{0.0,0.0,0.0}, .{0.5, -0.5, 0.0}),
    .create(.{0.0,0.0,0.0}, .{0.5, 0.5, 0.0}),
    .create(.{1.0,0.0,0.0}, .{-0.5, 0.5, 0.0}),
    .create(.{1.0,0.0,0.0}, .{-0.5, -0.5, 0.0})
};

pub const Indices = [_]u32{
    0, 1, 3, 1, 2, 3
};
