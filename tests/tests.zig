const std = @import("std");
const c = @import("c");
const Utils = @import("Utils");

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualSlices = std.testing.expectEqualSlices;

fn expectApproxEqSlices(T: type, expected: []const T, actual: []const T, tolerance: T) !void {
    var err: ?anyerror = null;
    for(0..expected.len) |i| {
        std.testing.expectApproxEqRel(expected[i], actual[i], tolerance) catch |e| {
            err = e;
            std.debug.print("^Index: {d}\n", .{i});
        };
    }
    if(err) |e| return e;
}

test "control" {
    try expect(true);
}

test "vector initialization" {
    const v1 = Utils.Vec(3).default;
    try expectEqualSlices(f32, &.{0.0, 0.0, 0.0}, &v1.arr);
    const v2 = Utils.Vec(3).init(.{1.0, -3.67, 3.14159});
    try expectEqualSlices(f32, &.{1.0, -3.67, 3.14159}, &v2.arr);
    const v3 = Utils.Vec(2).init(.{0.0, 7.7});
    try expectEqualSlices(f32, &.{0.0, 7.7}, &v3.arr);
}

test "vector addition" {
    var v1 = Utils.Vec(3).init(.{1.0, 1.0, 1.0});
    const v2 = Utils.Vec(3).init(.{-1.0, 0.0, 4.32});
    const vSum = v1.add(&v2);
    try expectEqualSlices(f32, &.{0.0, 1.0, 5.32}, &vSum.arr);
    _ = v1.addInto(&v2);
    try expectEqualSlices(f32, &.{0.0, 1.0, 5.32}, &v1.arr);
    const v3 = Utils.Vec(3).init(.{1.0, 1.0, 1.0});
    _ = v1.addInto(&v3).addInto(&v3).addInto(&v3);
    try expectEqualSlices(f32, &.{3.0, 4.0, 8.32}, &v1.arr);
}

test "identity matrix" {
    comptime {
    var I = Utils.Mat(4).identity();
        for(0..4) |i| {
            try expectEqual(I.at(i, i), 1.0);
        }
    }
}

test "copy matrix" {
    const m = Utils.Mat(5).identity();
    const copy = Utils.Mat(5).copy(&m);
    try expectEqualSlices(f32, &m.arr, &copy.arr);
}

test "basic matrix multiplication" {
    const I2 = Utils.Mat(2).identity();
    var m2 = Utils.Mat(2){
        .arr = .{
            3.4, 3.4,
            3.4, 3.4
        }
    };
    _ = m2.multInto(&I2);
    try expectEqualSlices(f32, &.{3.4, 3.4, 3.4, 3.4}, &m2.arr);

    const scale = Utils.Mat(2){
        .arr = .{
            2, 0,
            0, 1
        }
    };
    _ = m2.multInto(&scale);
    try expectEqualSlices(f32, &.{6.8, 6.8, 3.4, 3.4}, &m2.arr);

    var m = Utils.Mat(2).identity();
    _ = m.multInto(&scale).multInto(&scale).multInto(&scale);
    try expectEqualSlices(f32, &.{8, 0, 0, 1}, &m.arr);
}

test "advanced matrix multiplication" {
    var m = Utils.Mat(3){
        .arr = .{
            2.54, 1.873, 0.0,
            1.0, 5.55, -3.6,
            -4.772, 38.0, 2.0
        }
    };
    const m1 = Utils.Mat(3){
        .arr = .{
            -6.5, 2.0, 3.3,
            -4.67, -3.3, -2.0,
            9.08, 0.003, -4.5
        }
    };
    _ = m.multInto(&m1);
    const expected = Utils.Mat(3){
        .arr = .{
            -30.2576, 124.3255, -0.6,
            -5.6178, -103.06191, 7.88,
            44.5402, -153.97651, -9.0108
        }
    };
    try expectApproxEqSlices(f32, &expected.arr, &m.arr, 0.000001);
}

test "rotation matrices around axes" {
    const xaxis = Utils.Vec(3).init(.{1.0, 0.0, 0.0});
    _ = xaxis;
}
