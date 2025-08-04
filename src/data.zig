const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

pub const cube = Utils.Cube{
    .pos = .{.x = -5.0, .y = -5.0, .z = 0.0},
    .dim = .{.w = 10.0, .h = 10.0, .l = 10.0}
};

pub const Vertices = [_]Utils.Vertex{
    .create(.{1.0,1.0,1.0}, .{5, -5, 1.0}),
    .create(.{1.0,0.0,0.0}, .{5, 5, 1.0}),
    .create(.{0.0,1.0,0.0}, .{-5, 5, 1.0}),
    .create(.{0.0,0.0,1.0}, .{-5, -5, 1.0})
};

pub const Indices = [_]u32{
    0, 1, 3, 1, 2, 3
};
