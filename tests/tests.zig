const std = @import("std");
const c = @import("c");
const Mars = @import("Mars");

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
    const v1 = Mars.Vec(3).default;
    try expectEqualSlices(f32, &.{0.0, 0.0, 0.0}, &v1.arr);
    const v2 = Mars.Vec(3).init(.{1.0, -3.67, 3.14159});
    try expectEqualSlices(f32, &.{1.0, -3.67, 3.14159}, &v2.arr);
    const v3 = Mars.Vec(2).init(.{0.0, 7.7});
    try expectEqualSlices(f32, &.{0.0, 7.7}, &v3.arr);
}

test "vector addition" {
    var v1 = Mars.Vec(3).init(.{1.0, 1.0, 1.0});
    const v2 = Mars.Vec(3).init(.{-1.0, 0.0, 4.32});
    const vSum = v1.add(v2);
    try expectEqualSlices(f32, &.{0.0, 1.0, 5.32}, &vSum.arr);
    _ = v1.addInto(v2);
    try expectEqualSlices(f32, &.{0.0, 1.0, 5.32}, &v1.arr);
    const v3 = Mars.Vec(3).init(.{1.0, 1.0, 1.0});
    _ = v1.addInto(v3).addInto(v3).addInto(v3);
    try expectEqualSlices(f32, &.{3.0, 4.0, 8.32}, &v1.arr);
}

test "identity matrix" {
    const I = Mars.Mat(4).identity;
    for(0..4) |i| {
        try expectEqual(I.at(i, i), 1.0);
    }
}

test "copy matrix" {
    const m = Mars.Mat(5).identity;
    const copy = Mars.Mat(5).copy(m);
    try expectEqualSlices(f32, &m.arr, &copy.arr);
    try expect(&m != &copy);
}