const std = @import("std");
const Mars = @import("Mars");
const c = Mars.c;

const Data = @import("data.zig");

pub fn main() !void {
    var mars: Mars = undefined;
    try mars.state.init("Mars App");
    try mars.createMesh(&Data.cubeVertices, &Data.cubeIndices);
    try loadObjects(&mars);

    while(mars.state.notFlagSet(Mars.Flags.WINDOW_SHOULD_CLOSE)) {
        const nowUs: i64 = std.time.microTimestamp();
        mars.state.deltaTimeUs = nowUs - mars.state.time * std.time.us_per_ms;
        mars.state.time = @divFloor(nowUs, std.time.us_per_ms);
        mars.state.elapsedTime = mars.state.time - mars.state.programStartTime;

        while(c.SDL_PollEvent(&mars.state.nextEvent)) {
            try handleEvent(&mars.state);
        }

        try mars.loadMeshes();

        if(mars.state.notFlagSet(Mars.Flags.IS_PAUSED)) {
            {var it = mars.state.objects.iterator();
            while(it.next()) |obj| {
                obj.value_ptr.*.angle += std.math.pi / 2000000.0 * @as(f32, @floatFromInt(mars.state.deltaTimeUs));
            }}

            try Mars.drawFrame(&mars.state.GPU, mars.state.camera);
        }
    }

    mars.state.cleanup();
}

fn loadObjects(mars: *Mars) !void {
    const obj = try Mars.Object.create(&mars.state, 
        Mars.Pos{.x = -25.0, .y = -5.0, .z = -5.0},
        Mars.Pos{.x = 10.0, .y = 10.0, .z = 10.0},
        Mars.Math.Vec3.init(.{0.0, 1.0, 0.0}),
        0.0, &mars.state.GPU.meshes.items[0], null
    );
    try mars.state.objects.put(obj.id, obj);

    const obj2 = try Mars.Object.create(&mars.state,
        Mars.Pos{.x = 20, .y = -5.0, .z = -5.0},
        Mars.Pos{.x = 10.0, .y = 10.0, .z = 10.0},
        Mars.Math.Vec3.init(.{0.0, 1.0, 0.0}),
        0.0, &mars.state.GPU.meshes.items[0], null
    );
    try mars.state.objects.put(obj2.id, obj2);
}

fn handleEvent(state: *Mars.State) !void {
    switch(state.nextEvent.type) {
        c.SDL_EVENT_QUIT => state.activeFlags |= Mars.Flags.WINDOW_SHOULD_CLOSE,
        c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => state.activeFlags 
            |= Mars.Flags.WINDOW_SHOULD_CLOSE,
        c.SDL_EVENT_WINDOW_RESIZED => state.activeFlags |= Mars.Flags.RECREATE_SWAPCHAIN,
        c.SDL_EVENT_WINDOW_MINIMIZED => {
            state.activeFlags |= Mars.Flags.WINDOW_IS_MINIMIZED | Mars.Flags.IS_PAUSED;
            state.activeFlags &= ~Mars.Flags.WINDOW_IS_MAXIMIZED;
        },
        c.SDL_EVENT_WINDOW_MAXIMIZED => {
            state.activeFlags |= Mars.Flags.WINDOW_IS_MAXIMIZED;
            state.activeFlags &= ~Mars.Flags.WINDOW_IS_MINIMIZED;
        },
        c.SDL_EVENT_WINDOW_RESTORED => {
            state.activeFlags &= ~Mars.Flags.WINDOW_IS_MAXIMIZED;
            state.activeFlags &= ~Mars.Flags.WINDOW_IS_MINIMIZED;
            state.activeFlags &= ~Mars.Flags.IS_PAUSED;
            state.activeFlags |= Mars.Flags.RECREATE_SWAPCHAIN;
        },
        else => {}
    }
}