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
const CommandBuffer = @import("commandBuffer.zig");
const SyncObjects = @import("syncObjects.zig");
const Draw = @import("draw.zig");
const DescriptorSetLayout = @import("descriptorSetLayout.zig");
const DepthResources = @import("depthResources.zig");
const Mesh = @import("mesh.zig");

const enableValidationLayers: bool = buildOpts.isDebugBuild;

pub usingnamespace(Utils);

pub fn init(state: *Utils.State, name: []const u8) !void {
    if(!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        return error.failedToInitializeSDL3;
    }
    state.currentFrame = 0;
    state.name = name;
    state.camera = .{
        .angle = 0.0,
        .dir = Utils.Vec(3).init(.{0.0, 0.0, 1.0}),
        .pos = .{.x = 0.0, .y = -7.0, .z = -15.0}
    };

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
    try DescriptorSetLayout.init(state, null);
    try GraphicsPipeline.init(state, null);
    state.meshes = std.ArrayList(Utils.Mesh).init(std.heap.page_allocator);
    state.programStartTime = std.time.milliTimestamp();
    state.time = state.programStartTime;
    state.deltaTimeUs = 0;
    const mesh = try Mesh.init(state, &Mesh.cubeVertices, &Mesh.cubeIndices, null);
    try state.meshes.append(mesh);
    state.objects = try std.ArrayList(Utils.Object).initCapacity(std.heap.page_allocator, Utils.MAX_OBJECTS);
    state.objects.appendAssumeCapacity(try Utils.Object.create(state, 
        Utils.Pos{
            .x = -5.0, 
            .y = -5.0, 
            .z = -5.0
        },
        Utils.Pos{
            .x = 10.0,
            .y = 10.0,
            .z = 10.0
        },
        Utils.Vec(3).init(.{0.0, 1.0, 0.0}),
        0.0, state.meshes.items[0], null));
}

pub fn mainLoop(state: *Utils.State) !void {
    var windowShouldClose: bool = false;
    while(!windowShouldClose) {
        const nowUs: i64 = std.time.microTimestamp();
        state.time = @divFloor(nowUs, std.time.us_per_ms);
        state.deltaTimeUs = nowUs - state.programStartTime * std.time.us_per_ms;
        state.elapsedTime = state.time - state.programStartTime;
        while(c.SDL_PollEvent(&state.nextEvent)) {
            //try Events.handleEvent(state);
            switch(state.nextEvent.type) {
                c.SDL_EVENT_QUIT => windowShouldClose = true,
                else => {}
            }
        }
        state.objects.items[0].angle += std.math.pi / 2000.0 * @as(f32, @floatFromInt(state.elapsedTime));
        try Draw.drawFrame(state);
    }
    _ = c.vkDeviceWaitIdle(state.device);
}

pub fn cleanup(state: *Utils.State) void {
    for(state.meshes.items) |*mesh| mesh.buffer.destroy(state.device, null);
    state.meshes.deinit();
    for(state.objects.items) |*object| object.uniformBuffer.destroy(state.device, null);
    GraphicsPipeline.destroy(state, null);
    DescriptorSetLayout.destroy(state, null);
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
}

