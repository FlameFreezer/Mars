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

pub const State = struct {
    name: []const u8,
    nextEvent: c.SDL_Event,
    programStartTime: i64,
    time: i64,
    deltaTimeUs: i64,
    elapsedTime: i64,
    camera: Utils.Camera,
    activeFlags: Utils.MarsFlags = 0,
    GPU: Utils.GPUState,

    pub fn init(state: *State, name: []const u8) !void {
        state.programStartTime = std.time.milliTimestamp();
        state.time = state.programStartTime;
        state.deltaTimeUs = 0;
        if(!c.SDL_Init(c.SDL_INIT_VIDEO)) {
            return error.failedToInitializeSDL3;
        }
        state.name = name;
        state.activeFlags = Utils.Flags.NONE;
        try state.initGPU(state.name);
        state.camera = Utils.Camera.default;
        state.camera.pos = .{.x = -65.0, .y = 20.0, .z = 0.0};
        state.camera.setTarget(.{.x = 0.0, .y = 0.0, .z = 0.0});
    }

    pub fn mainLoop(state: *State) !void {
        try Mesh.init(&state.GPU, &Mesh.cubeVertices, &Mesh.cubeIndices, null);
        try loadObjects(&state.GPU);

        while(state.notFlagSet(Utils.Flags.WINDOW_SHOULD_CLOSE)) {
            const nowUs: i64 = std.time.microTimestamp();
            state.deltaTimeUs = nowUs - state.time * std.time.us_per_ms;
            state.time = @divFloor(nowUs, std.time.us_per_ms);
            state.elapsedTime = state.time - state.programStartTime;

            while(c.SDL_PollEvent(&state.nextEvent)) {
                try handleEvent(state);
            }

            try loadMeshes(&state.GPU);

            if(state.notFlagSet(Utils.Flags.IS_PAUSED)) {
                {var it = state.GPU.objects.iterator();
                while(it.next()) |obj| {
                    obj.value_ptr.*.angle += std.math.pi / 2000000.0 * @as(f32, @floatFromInt(state.deltaTimeUs));
                }}

                try Draw.drawFrame(&state.GPU, state.camera);
            }
        }
        _ = c.vkDeviceWaitIdle(state.GPU.device);
    }

    pub fn cleanup(state: *State) void {
        {var it = state.GPU.objects.iterator();
        while(it.next()) |entry| {
            entry.value_ptr.destroy(state.GPU.device, state.GPU.descriptorPool, null);
        }}
        state.GPU.objects.deinit();
        for(state.GPU.meshes.items) |*mesh| mesh.destroy(state.GPU.device, null);
        state.GPU.meshes.deinit();
        state.GPU.transferStagingBuffers.deinit();
        GraphicsPipeline.destroy(&state.GPU, null);
        DescriptorSetLayout.destroy(&state.GPU, null);
        CommandBuffer.destroy(&state.GPU, null);
        SyncObjects.destroy(&state.GPU, null);
        DepthResources.destroy(&state.GPU, null);
        Swapchain.destroy(&state.GPU, null);
        if(enableValidationLayers) {
            DebugMessenger.destroy(&state.GPU, null);
        }
        Device.destroy(&state.GPU, null);
        std.heap.page_allocator.free(state.GPU.queues.graphicsQueues);
        Window.destroy(&state.GPU, null);
        Instance.destroy(&state.GPU, null);
        c.SDL_Quit();
    }

    /// Returns true is the provided flags are set, since zig is so anal about truthy values
    pub fn isFlagSet(Self: *State, flag: Utils.MarsFlags) bool {
        return Self.*.activeFlags & flag != 0;
    }
    /// Returns true if the provided flags are not set, since zig is so anal about truthy values
    pub fn notFlagSet(Self: *State, flag: Utils.MarsFlags) bool {
        return Self.*.activeFlags & flag == 0;
    }

    fn initGPU(Self: *State, windowName: []const u8) !void {
        const state: *Utils.GPUState = &Self.*.GPU;
        try Instance.init(state, enableValidationLayers, null);
        if(enableValidationLayers) {
            DebugMessenger.init(state, null);
        }
        try Window.init(state, windowName, null);
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
        state.currentFrame = 0;
        state.lastGeneratedId = 0;
    }
};

fn loadMeshes(state: *Utils.GPUState) !void {
    if(state.*.notFlagSet(Utils.Flags.BEGAN_MESH_LOADING)) return;

    if(c.vkEndCommandBuffer(state.*.transferCommandBuffer) != c.VK_SUCCESS) {
        return error.failedToEndCommandBuffer;
    }
    const submitInfo = c.VkSubmitInfo2{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 0,
        .signalSemaphoreInfoCount = 0,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &c.VkCommandBufferSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = state.*.transferCommandBuffer
        }
    };
    if(c.vkQueueSubmit2(state.*.queues.transferQueue, 1, &submitInfo, state.*.transferOpFence) != c.VK_SUCCESS) {
        return error.failedToSubmitToQueue;
    }
}

fn loadObjects(state: *Utils.GPUState) !void {
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

fn handleEvent(state: *State) !void {
    switch(state.nextEvent.type) {
        c.SDL_EVENT_QUIT => state.activeFlags |= Utils.Flags.WINDOW_SHOULD_CLOSE,
        c.SDL_EVENT_WINDOW_CLOSE_REQUESTED => state.activeFlags 
            |= Utils.Flags.WINDOW_SHOULD_CLOSE,
        c.SDL_EVENT_WINDOW_RESIZED => try Swapchain.recreate(&state.GPU, null),
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
            try Swapchain.recreate(&state.GPU, null);
        },
        else => {}
    }
}