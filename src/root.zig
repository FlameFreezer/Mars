const std = @import("std");
const c = @import("c");
const buildOpts = @import("buildOpts");

const Utils = @import("utils.zig");
const Math = @import("math.zig");
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
    state.lastGeneratedId = 0;
    state.activeFlags = Utils.Flags.NONE;

    try Instance.init(state, enableValidationLayers, null);
    if(enableValidationLayers) {
        DebugMessenger.init(state, null);
    }
    try Window.init(state, null);
    try Device.init(state, null);
    try Swapchain.init(state, null);
    try DepthResources.init(state, null);
    try SyncObjects.init(state, null);
    try CommandBuffer.init(state, null);
    try DescriptorSetLayout.init(state, null);
    try GraphicsPipeline.init(state, null);
    state.meshes = std.ArrayList(Utils.Mesh).init(std.heap.page_allocator);
    state.objects = Utils.ObjectArrayHashMap.initContext(std.heap.page_allocator, 
        Utils.Object.HashContext{.hashModulus = Utils.MAX_OBJECTS});
    state.transferStagingBuffers = std.ArrayList(Utils.Buffer).init(std.heap.page_allocator);
    state.programStartTime = std.time.milliTimestamp();
    state.time = state.programStartTime;
    state.deltaTimeUs = 0;
    
    state.camera = .{
        .pos = .{.x = -65.0, .y = 20.0, .z = 0.0},
    };
    state.camera.setTarget(.{.x = 0.0, .y = 0.0, .z = 0.0});
}

pub fn mainLoop(state: *Utils.State) !void {
    try loadMeshes(state);
    try loadObjects(state);

    while(state.activeFlags & Utils.Flags.WINDOW_SHOULD_CLOSE == 0) {
        const nowUs: i64 = std.time.microTimestamp();
        state.deltaTimeUs = nowUs - state.time * std.time.us_per_ms;
        state.time = @divFloor(nowUs, std.time.us_per_ms);
        state.elapsedTime = state.time - state.programStartTime;

        while(c.SDL_PollEvent(&state.nextEvent)) {
            try handleEvent(state);
        }

        if(state.activeFlags & Utils.Flags.IS_PAUSED == 0) {
            {var it = state.objects.iterator();
            while(it.next()) |obj| {
                obj.value_ptr.*.angle += std.math.pi / 2000000.0 * @as(f32, @floatFromInt(state.deltaTimeUs));
            }}

            try Draw.drawFrame(state);
        }
    }
    _ = c.vkDeviceWaitIdle(state.device);
}

pub fn cleanup(state: *Utils.State) void {
    {var it = state.objects.iterator();
    while(it.next()) |entry| {
        entry.value_ptr.destroy(state.device, state.descriptorPool, null);
    }}
    state.objects.deinit();
    for(state.meshes.items) |*mesh| mesh.destroy(state.device, null);
    state.meshes.deinit();
    state.transferStagingBuffers.deinit();
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
    std.heap.page_allocator.free(state.queues.graphicsQueues);
    Window.destroy(state, null);
    Instance.destroy(state, null);
    c.SDL_Quit();
}

fn handleEvent(state: *Utils.State) !void {
    switch(state.nextEvent.type) {
        c.SDL_EVENT_QUIT => state.activeFlags |= Utils.Flags.WINDOW_SHOULD_CLOSE,
        c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => state.activeFlags 
            |= Utils.Flags.WINDOW_SHOULD_CLOSE,
        c.SDL_EVENT_WINDOW_RESIZED => try Swapchain.recreate(state, null),
        c.SDL_EVENT_WINDOW_MINIMIZED => {
            state.activeFlags |= Utils.Flags.WINDOW_IS_MINIMIZED | Utils.Flags.IS_PAUSED;
            state.activeFlags &= ~Utils.Flags.WINDOW_IS_MAXIMIZED;
        },
        c.SDL_EVENT_WINDOW_MAXIMIZED => {
            state.activeFlags |= Utils.Flags.WINDOW_IS_MAXIMIZED;
            state.activeFlags &= ~Utils.Flags.WINDOW_IS_MINIMIZED;
        },
        c.SDL_EVENT_WINDOW_RESTORED => {
            state.activeFlags &= ~Utils.Flags.WINDOW_IS_MAXIMIZED;
            state.activeFlags &= ~Utils.Flags.WINDOW_IS_MINIMIZED;
            state.activeFlags &= ~Utils.Flags.IS_PAUSED;
            try Swapchain.recreate(state, null);
        },
        else => {}
    }
}

fn loadMeshes(state: *Utils.State) !void {
    try Mesh.init(state, &Mesh.cubeVertices, &Mesh.cubeIndices, null);
    if(c.vkEndCommandBuffer(state.*.commandBuffers[Utils.MAX_FRAMES_IN_FLIGHT]) != c.VK_SUCCESS) {
        return error.failedToEndCommandBuffer;
    }
    const submitInfo = c.VkSubmitInfo2{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 0,
        .signalSemaphoreInfoCount = 0,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &c.VkCommandBufferSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = state.commandBuffers[Utils.MAX_FRAMES_IN_FLIGHT]
        }
    };
    if(c.vkQueueSubmit2(state.queues.transferQueue, 1, &submitInfo, state.*.transferOpFence) != c.VK_SUCCESS) {
        return error.failedToSubmitToQueue;
    }
}

fn loadObjects(state: *Utils.State) !void {
    const obj = try Utils.Object.create(state, 
        Utils.Pos{.x = -25.0, .y = -5.0, .z = -5.0},
        Utils.Pos{.x = 10.0, .y = 10.0, .z = 10.0},
        Math.Vec3.init(.{0.0, 1.0, 0.0}),
        0.0, &state.meshes.items[0], null
    );
    try state.objects.put(obj.id, obj);

    const obj2 = try Utils.Object.create(state,
        Utils.Pos{.x = 20, .y = -5.0, .z = -5.0},
        Utils.Pos{.x = 10.0, .y = 10.0, .z = 10.0},
        Math.Vec3.init(.{0.0, 1.0, 0.0}),
        0.0, &state.meshes.items[0], null
    );
    try state.objects.put(obj2.id, obj2);
}