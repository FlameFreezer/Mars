const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

pub const cube = Utils.Cube{
    .w = 10.0,
    .h = 10.0,
    .l = 10.0
};

pub const vertices = [_]Utils.Vertex{
    .create(.{1.0,1.0,1.0}, .{-5.0, 5.0, 8.0}),
    .create(.{1.0,0.0,0.0}, .{5.0, 5.0, 8.0}),
    .create(.{0.0,1.0,0.0}, .{5.0, -5.0, 8.0}),
    .create(.{0.0,0.0,1.0}, .{-5.0, -5.0, 8.0})
};

pub const indices = [_]u32{
    0, 1, 2, 0, 2, 3
};

pub const Vertices = cube.getVertices();
pub const Indices = cube.getIndices();
