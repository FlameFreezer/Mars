const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const WIDTH: u32 = 800;
const HEIGHT: u32 = 600;

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    state.window = c.SDL_CreateWindow(state.name.ptr, WIDTH, HEIGHT, 
        c.SDL_WINDOW_VULKAN | c.SDL_WINDOW_RESIZABLE) orelse return error.failedToCreateWindow;

    if(!c.SDL_Vulkan_CreateSurface(state.window, state.instance, allocator, &state.surface)) {
        return error.failedToCreateWindowSurface;
    }
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.SDL_Vulkan_DestroySurface(state.instance, state.surface, allocator);
    c.SDL_DestroyWindow(state.window);
}
