const std = @import("std");
const Mars = @import("Mars");
const c = Mars.c;

pub const cubeVertices = makeVertices:{
    const white = [3]f32{1.0, 1.0, 1.0};
    const red = [3]f32{1.0, 0.0, 0.0};
    const green = [3]f32{0.0, 1.0, 0.0};
    const blue = [3]f32{0.0, 0.0, 1.0};
    const yellow = [3]f32{1.0, 1.0, 0.0};
    const purple = [3]f32{1.0, 0.0, 1.0};

    const frontFace = [4][3]f32{
        .{0.0, 1.0, 0.0},
        .{1.0, 1.0, 0.0},
        .{1.0, 0.0, 0.0},
        .{0.0, 0.0, 0.0}
    };
    const backFace = [4][3]f32{
        .{0.0, 1.0, 1.0},
        .{1.0, 1.0, 1.0},
        .{1.0, 0.0, 1.0},
        .{0.0, 0.0, 1.0}
    };
    const noMap = [2]f32{0.0, 0.0};

    break:makeVertices [_]Mars.Vertex{
        //FRONT FACE
        .create(white,frontFace[0],noMap),
        .create(white,frontFace[1],noMap),
        .create(white,frontFace[2],noMap),
        .create(white,frontFace[3],noMap),
        //BACK FACE
        .create(purple,backFace[0],noMap),
        .create(purple,backFace[1],noMap),
        .create(purple,backFace[2],noMap),
        .create(purple,backFace[3],noMap),
        //RIGHT FACE
        .create(yellow,frontFace[1],noMap),
        .create(yellow,backFace[1],noMap),
        .create(yellow,backFace[2],noMap),
        .create(yellow,frontFace[2],noMap),
        //LEFT FACE
        .create(green,frontFace[0],noMap),
        .create(green,backFace[0],noMap),
        .create(green,backFace[3],noMap),
        .create(green,frontFace[3],noMap),
        //TOP FACE
        .create(red,backFace[0],noMap),
        .create(red,backFace[1],noMap),
        .create(red,frontFace[1],noMap),
        .create(red,frontFace[0],noMap),
        //BOTTOM FACE
        .create(blue,backFace[3],noMap),
        .create(blue,backFace[2],noMap),
        .create(blue,frontFace[2],noMap),
        .create(blue,frontFace[3],noMap),
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