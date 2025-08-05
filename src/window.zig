const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

const WIDTH: u32 = 800;
const HEIGHT: u32 = 600;

pub fn init(state: *Utils.State) !void {
    state.window = c.SDL_CreateWindow(state.name.ptr, WIDTH, HEIGHT, 
        c.SDL_WINDOW_VULKAN | c.SDL_WINDOW_RESIZABLE) orelse return error.failedToCreateWindow;
}

pub fn destroy(state: *Utils.State) void {
    c.SDL_DestroyWindow(state.window);
    state.window = null;
}
