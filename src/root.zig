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
const Descriptors = @import("descriptors.zig");
const DepthResources = @import("depthResources.zig");

const enableValidationLayers: bool = buildOpts.isDebugBuild;

pub usingnamespace(Utils);

pub fn init(state: *Utils.State, name: []const u8) !void {
    if(!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        return error.failedToInitializeSDL3;
    }
    state.currentFrame = 0;
    state.name = name;
    state.windowShouldClose = false;

    try Window.init(state);
    try Instance.init(state, enableValidationLayers);
    if(enableValidationLayers) {
        DebugMessenger.init(state, null);
    }
    try Device.init(state, null);
    try Swapchain.init(state, null);
    try DepthResources.init(state, null);
    try SyncObjects.init(state, null);
    try CommandBuffer.init(state, null);
    try Buffer.init(state, null);
    try Descriptors.init(state, null);
    try GraphicsPipeline.init(state, null);
    const tempStartTime = try std.heap.page_allocator.create(i64);
    tempStartTime.* = std.time.milliTimestamp();
    state.startTime = tempStartTime;
}

pub fn mainLoop(state: *Utils.State) !void {
    while(!state.windowShouldClose) {
        while(c.SDL_PollEvent(&state.nextEvent)) {
            //try Events.handleEvent(state);
            switch(state.nextEvent.type) {
                c.SDL_EVENT_QUIT => state.windowShouldClose = true,
                else => {}
            }
        }
        try Draw.drawFrame(state);
    }
    _ = c.vkDeviceWaitIdle(state.device);
}

pub fn cleanup(state: *Utils.State) void {
    Buffer.destroy(state, null);
    GraphicsPipeline.destroy(state, null);
    Descriptors.destroy(state, null);
    CommandBuffer.destroy(state, null);
    SyncObjects.destroy(state, null);
    DepthResources.destroy(state, null);
    Swapchain.destroy(state, null);
    if(enableValidationLayers) {
        DebugMessenger.destroy(state, null);
    }
    Device.destroy(state, null);
    Instance.destroy(state, null);
    Window.destroy(state);
    c.SDL_Quit();
    std.heap.page_allocator.destroy(state.startTime);
}

