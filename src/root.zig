const std = @import("std");
const c = @import("c");
const buildOpts = @import("buildOpts");

const Utils = @import("utils.zig");
const Window = @import("window.zig");
const Instance = @import("instance.zig");
const DebugMessenger = @import("debugMessenger.zig");
const Device = @import("device.zig");
const Swapchain = @import("swapchain.zig");
const GraphicsPipeline = @import("graphicsPipeline.zig");
const Buffer = @import("buffer.zig");
const CommandBuffer = @import("commandBuffer.zig");
const SyncObjects = @import("syncObjects.zig");
const Draw = @import("draw.zig");

const enableValidationLayers: bool = buildOpts.isDebugBuild;

pub const State = Utils.State;

pub fn init(state: *Utils.State) !void {
    if(c.glfwInit() == c.GLFW_FALSE) {
        return error.glfwFailedInit;
    }
    state.currentFrame = 0;

    Window.init(state);
    try Instance.init(state, enableValidationLayers);
    if(enableValidationLayers) {
        DebugMessenger.init(state, null);
    }
    try Device.init(state, null);
    try Swapchain.init(state, null);
    try SyncObjects.init(state, null);
    try CommandBuffer.init(state, null);
    try Buffer.init(state, null);
    try GraphicsPipeline.init(state, null);
}

pub fn mainLoop(state: *Utils.State) !void {
    while(c.glfwWindowShouldClose(state.window) == c.GLFW_FALSE) {
        c.glfwPollEvents();
        try Draw.drawFrame(state);
    }
    _ = c.vkDeviceWaitIdle(state.device);
}

pub fn cleanup(state: *Utils.State) void {
    Buffer.destroy(state, null);
    GraphicsPipeline.destroy(state, null);
    CommandBuffer.destroy(state, null);
    SyncObjects.destroy(state, null);
    Swapchain.destroy(state, null);
    if(enableValidationLayers) {
        DebugMessenger.destroy(state, null);
    }
    Device.destroy(state, null);
    Instance.destroy(state, null);
    Window.destroy(state);
    c.glfwTerminate();
}

