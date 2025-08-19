const std = @import("std");

pub const Vec3 = Vec(3);
pub const Vec4 = Vec(4);
pub const Mat4 = Mat(4);
pub fn Vec(comptime n: usize) type {
    return struct {
        arr: [n]f32,
        
        pub const zero = @This(){ .arr = [1]f32{0.0} ** n};
        pub const len: usize = n;
        pub const VectorError = error{
            outOfBounds
        };

        pub fn init(values: [n]f32) @This() {
            var result: @This() = undefined;
            @memcpy(result.arr[0..], values[0..]);
            return result;
        }

        pub fn add(v1: @This(), v2: @This()) @This() {
            var result = v1;
            for(0..n) |i| result.arr[i] += v2.arr[i];
            return result;
        }

        pub fn addInto(v1: *@This(), v2: @This()) *@This() {
            for(0..n) |i| v1.arr[i] += v2.arr[i];
            return v1;
        }

        pub fn subtract(v1: @This(), v2: @This()) @This() {
            var result = v1;
            for(0..n) |i| result.arr[i] -= v2.arr[i];
            return result;
        }

        pub fn scale(v: @This(), factor: f32) @This() {
            var result = v;
            for(0..n) |i| result.arr[i] *= factor;
            return result;
        }

        pub fn magnitude(v: @This()) f32 {
            var result: f32 = 0;
            for(0..n) |i| {
                result += v.arr[i] * v.arr[i];
            }
            return std.math.sqrt(result);
        }

        pub fn normalize(v: @This()) @This() {
            return v.scale(1.0 / v.magnitude());
        }

        pub fn at(Self: @This(), i: usize) VectorError!f32 {
            if(i >= n) return VectorError.outOfBounds;
            return Self.arr[i];
        }

        pub fn equals(Self: @This(), other: @This()) bool {
            for(0..n) |i| if(Self.arr[i] != other.arr[i]) return false;
            return true;
        }

    };
}

pub fn dotProduct(comptime n: usize, v1: Vec(n), v2: Vec(n)) f32 {
    var result: f32 = 0;
    for(0..n) |i| result += v1.arr[i] * v2.arr[i];
    return result;
}

pub fn crossProduct(v1: Vec3, v2: Vec3) Vec3 {
    return Vec3.init(.{
        v1.arr[1] * v2.arr[2] - v1.arr[2] * v2.arr[1],
        -v1.arr[0] * v2.arr[2] + v1.arr[2] * v2.arr[0],
        v1.arr[0] * v2.arr[1] - v1.arr[1] * v2.arr[0]
    });
}

pub fn Mat(comptime r: usize) type {
    return struct {
        arr: [r*r]f32,

        pub const default = @This(){ .arr = [1]f32{0.0} ** (r*r)};
        pub const identity = I:{
            var result = default;
            for(0..r) |i| result.setUnsafe(i, i, 1.0);
            break:I result;
        };

        pub const MatrixError = error{
            RowOutOfBounds,
            ColumnOutOfBounds
        };

        pub fn init(values: [r*r]f32) @This() {
            var result: @This() = undefined;
            @memcpy(result.arr[0..], values[0..]);
            return result;
        }

        pub fn at(Self: *const @This(), row: usize, col: usize) MatrixError!f32 {
            try boundsCheck(row, col);
            return Self.arr[row * r + col];
        }
        
        pub fn atUnsafe(Self: *const @This(), row: usize, col: usize) f32 {
            return Self.arr[row * r + col];
        }

        pub fn set(Self: * @This(), row: usize, col : usize, value: f32) MatrixError!void {
            try boundsCheck(row, col);
            Self.arr[row * r + col] = value;
        }
        
        pub fn setUnsafe(Self: *@This(), row: usize, col: usize, value: f32) void {
            Self.arr[row * r + col] = value;
        }

        pub fn mult(m1: @This(), m2: @This()) @This() {
            var result: @This() = default;
            for(0..r) |row| {
                for(0..r) |col| {
                    var sum: f32 = 0;
                    for(0..r) |i| {
                       sum += m1.atUnsafe(row, i) * m2.atUnsafe(i, col); 
                    }
                    result.setUnsafe(row, col, sum); 
                }
            }
            return result;
        }

        pub fn copy(other: @This()) @This() {
            var result: @This() = undefined;
            @memcpy(&result.arr, &other.arr);
            return result;
        }

        pub fn transform(Self: @This(), vector: Vec(r)) Vec(r) {
            var result = Vec(r).zero;
            for(0..r) |i| {
                for(0..r) |j| result.arr[i] += Self.atUnsafe(i,j) * vector.arr[j];
            }
            return result;

        }

        pub fn transformInto(Self: @This(), vector: *Vec(r)) *Vec(r) {
            const temp = Self.transform(vector.*);
            @memcpy(vector.arr[0..], temp.arr[0..]);
            return vector;
        }

        fn boundsCheck(row: usize, col: usize) MatrixError!void {
            if(row >= r) return MatrixError.RowOutOfBounds;
            if(col >= r) return MatrixError.ColumnOutOfBounds;
        }
    };
}

//Matrix taken from: https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
pub fn rotate(angle: f32, axis: Vec3) Mat4 {
    const u = axis.normalize(); 
    const ux: f32 = u.arr[0];
    const uy: f32 = u.arr[1];
    const uz: f32 = u.arr[2];
    const cos: f32 = @cos(angle);
    const sin: f32 = @sin(angle);
    return Mat4.init(.{
        ux * ux * (1.0 - cos) + cos, ux * uy * (1.0 - cos) - uz * sin, ux * uz * (1.0 - cos) + uy * sin, 0.0,
        ux * uy * (1.0 - cos) + uz * sin, uy * uy * (1.0 - cos) + cos, uy * uz * (1.0 - cos) - ux * sin, 0.0,
        ux * uz * (1.0 - cos) - uy * sin, uy * uz * (1.0 - cos) + ux * sin, uz * uz * (1.0 - cos) + cos, 0.0,
        0.0, 0.0, 0.0, 1.0
    });
}

pub fn translate(shift: Vec3) Mat4 {
    var result = Mat4.identity;
    result.setUnsafe(0, 3, shift.arr[0]);
    result.setUnsafe(1, 3, shift.arr[1]);
    result.setUnsafe(2, 3, shift.arr[2]);
    return result;
}

pub fn scaleMatrix(factors: [3]f32) Mat4 {
    var result = Mat4.identity;
    result.setUnsafe(0, 0, factors[0]);
    result.setUnsafe(1, 1, factors[1]);
    result.setUnsafe(2, 2, factors[2]);
    return result;
}

pub fn view(direction: Vec3, cameraLocation: Vec3, upVector: Vec3) Mat4 {
    const W = direction.normalize(); //Camera space z-axis
    const U = crossProduct(upVector, W).normalize(); //Camera space x-axis
    const V = crossProduct(W, U).normalize(); //Camera space y-axis
    return Mat4.init(.{
        U.arr[0], U.arr[1], U.arr[2], -dotProduct(3, U, cameraLocation),
        V.arr[0], V.arr[1], V.arr[2], -dotProduct(3, V, cameraLocation),
        W.arr[0], W.arr[1], W.arr[2], -dotProduct(3, W, cameraLocation),
        0.0, 0.0, 0.0, 1.0
    });
}

pub fn lookAt(where: Vec3, cameraLocation: Vec3, cameraAngle: f32) Mat4 {
    return view(where.subtract(cameraLocation), cameraLocation, cameraAngle); 
}

pub fn perspective(near: f32, far: f32, fieldOfView: f32, dimension: f32) Mat4 {
    const width: f32 = @abs(@tan(fieldOfView / 2.0)) * near * 2.0;
    const height: f32 = dimension * width;
    const per = Mat4.init(.{
        near, 0.0, 0.0, 0.0,
        0.0, near, 0.0, 0.0,
        0.0, 0.0, near + far, -(near * far),
        0.0, 0.0, 1.0, 0.0
    });
    const ortho = Mat4.init(.{
        2.0 / width, 0.0, 0.0, 0.0,
        0.0, -2.0 / height, 0.0, 0.0,
        0.0, 0.0, 1.0 / (far - near), -near / (far - near),
        0.0, 0.0, 0.0, 1.0
    });
    return ortho.mult(per);
}