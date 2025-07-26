const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const WIDTH: u32 = 800;
const HEIGHT: u32 = 600;

pub fn init(state: *Utils.State) void {
    c.glfwWindowHint(c.GLFW_CLIENT_API, c.GLFW_NO_API);
    state.window = c.glfwCreateWindow(WIDTH, HEIGHT, "Zig Vulkan", null, null);
}

pub fn destroy(state: *Utils.State) void {
    c.glfwDestroyWindow(state.window);
}
