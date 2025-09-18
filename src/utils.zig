const std = @import("std");
const c = @import("c");
const buildOpts = @import("buildOpts");

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
const TexutreSampler = @import("textureSampler.zig");

const enableValidationLayers: bool = buildOpts.isDebugBuild;

pub const MAX_FRAMES_IN_FLIGHT: usize = 2;
pub const MAX_OBJECTS: usize = 512;

pub const MarsFlags = u16;
pub const Flags = struct {
    pub const NONE: MarsFlags = 0;
    pub const WINDOW_IS_MINIMIZED: MarsFlags = 1;
    pub const WINDOW_IS_MAXIMIZED: MarsFlags = 1 << 1;
    pub const WINDOW_SHOULD_CLOSE: MarsFlags = 1 << 2;
    pub const IS_PAUSED: MarsFlags = 1 << 3;
    pub const BEGAN_MESH_LOADING: MarsFlags = 1 << 4;
    pub const RECREATE_SWAPCHAIN: MarsFlags = 1 << 5;
};

pub const State = struct {
    name: []const u8,
    nextEvent: c.SDL_Event,
    programStartTime: i64,
    time: i64,
    deltaTimeUs: i64,
    elapsedTime: i64,
    camera: Camera,
    activeFlags: MarsFlags,
    GPU: GPUState,
    objects: IDArrayHashMap(Object),
    IDGen: IDGenerator,

    pub fn init(self: *State, name: []const u8) !void {
        self.programStartTime = std.time.milliTimestamp();
        self.time = self.programStartTime;
        self.deltaTimeUs = 0;
        if(!c.SDL_Init(c.SDL_INIT_VIDEO)) {
            return error.failedToInitializeSDL3;
        }
        self.name = name;
        self.activeFlags = Flags.NONE;
        self.objects = IDArrayHashMap(Object).initContext(std.heap.page_allocator, 
            IDArrayHashContext{.hashModulus = MAX_OBJECTS});
        try self.GPU.init(self);
        self.camera = .default;
        self.camera.pos = .{.x = -65.0, .y = 20.0, .z = 0.0};
        self.camera.setTarget(.{.x = 0.0, .y = 0.0, .z = 0.0});
        self.IDGen = .default;
    }

    pub fn cleanup(self: *State) void {
        _ = c.vkDeviceWaitIdle(self.GPU.device);
        {var it = self.objects.iterator();
        while(it.next()) |entry| {
            entry.value_ptr.destroy(self.GPU.device, self.GPU.descriptorPool, null);
        }}
        self.objects.deinit();
        self.GPU.cleanup();
        c.SDL_Quit();
    }

    /// Returns true is the provided flags are set, since zig is so anal about truthy values
    pub fn isFlagSet(self: *const State, flag: MarsFlags) bool {
        return self.*.activeFlags & flag != 0;
    }
    /// Returns true if the provided flags are not set, since zig is so anal about truthy values
    pub fn notFlagSet(self: *const State, flag: MarsFlags) bool {
        return self.*.activeFlags & flag == 0;
    }
};

pub const GPUState = struct {
    appState: *State,
    activeFlags: MarsFlags,
    window: *c.SDL_Window,
    instance: c.VkInstance,
    debugMessenger: c.VkDebugUtilsMessengerEXT,
    physicalDevice: c.VkPhysicalDevice,
    device: c.VkDevice,
    surface: c.VkSurfaceKHR,
    queues: Queues,
    queueFamilyIndices: QueueFamilyIndices,
    swapchain: c.VkSwapchainKHR,
    swapchainImages: []c.VkImage,
    swapchainImageViews: []c.VkImageView,
    swapchainExtent: c.VkExtent2D,
    commandPool: c.VkCommandPool,
    /// Each frame gets a command buffer, alongside one for transfer operations
    commandBuffers: [MAX_FRAMES_IN_FLIGHT + 1]c.VkCommandBuffer,
    transferCommandBuffer: c.VkCommandBuffer,
    cameraPushConstant: CameraPushConstant,
    graphicsPipelineLayout: c.VkPipelineLayout,
    graphicsPipeline: c.VkPipeline,
    fences: [MAX_FRAMES_IN_FLIGHT]c.VkFence,
    transferOpFence: c.VkFence,
    semaphores: []c.VkSemaphore,
    currentFrame: usize = 0,
    descriptorSetLayout: c.VkDescriptorSetLayout,
    descriptorPool: c.VkDescriptorPool,
    depthImage: Image,
    transferStagingBuffers: std.ArrayList(Buffer),
    textureSampler: c.VkSampler,
    meshes: IDArrayHashMap(Mesh),
    textures: IDHashMap(Image),
    models: IDArrayHashMap(Model),

    /// Returns true is the provided flags are set, since zig is so anal about truthy values
    pub fn isFlagSet(self: *const GPUState, flag: MarsFlags) bool {
        return self.*.activeFlags & flag != 0;
    }
    /// Returns true if the provided flags are not set, since zig is so anal about truthy values
    pub fn notFlagSet(self: *const GPUState, flag: MarsFlags) bool {
        return self.*.activeFlags & flag == 0;
    }

    pub fn init(self: *GPUState, parent: *State) !void {
        self.appState = parent;
        try Instance.init(self, enableValidationLayers, null);
        if(enableValidationLayers) {
            DebugMessenger.init(self, null);
        }
        try Window.init(self, parent.name, null);
        try Device.init(self, null);
        try Swapchain.init(self, null);
        try DepthResources.init(self, null);
        try SyncObjects.init(self, null);
        try CommandBuffer.init(self, null);
        try DescriptorSetLayout.init(self, null);
        try GraphicsPipeline.init(self, null);
        try TexutreSampler.init(self, null); 
        self.meshes = IDArrayHashMap(Mesh).initContext(std.heap.page_allocator, IDArrayHashContext{.hashModulus = MAX_OBJECTS});
        self.textures = IDHashMap(Image).initContext(std.heap.page_allocator, IDHashContext{.hashModulus = MAX_OBJECTS});
        self.models = IDArrayHashMap(Model).initContext(std.heap.page_allocator, IDArrayHashContext{.hashModulus = MAX_OBJECTS});
        self.transferStagingBuffers = std.ArrayList(Buffer).init(std.heap.page_allocator);
        self.currentFrame = 0;
    }

    pub fn cleanup(self: *GPUState) void {
        {var it = self.meshes.iterator();
        while(it.next()) |entry| {
            entry.value_ptr.destroy(self.device, null);
        }}
        self.meshes.deinit();
        {var it = self.textures.valueIterator();
        while(it.next()) |texture| {
            texture.destroy(self.device, null);
        }}
        self.textures.deinit();
        self.transferStagingBuffers.deinit();
        TexutreSampler.destroy(self, null);
        GraphicsPipeline.destroy(self, null);
        DescriptorSetLayout.destroy(self, null);
        CommandBuffer.destroy(self, null);
        SyncObjects.destroy(self, null);
        self.depthImage.destroy(self.device, null);
        Swapchain.destroy(self, null);
        if(enableValidationLayers) {
            DebugMessenger.destroy(self, null);
        }
        Device.destroy(self, null);
        std.heap.page_allocator.free(self.queues.graphicsQueues);
        Window.destroy(self, null);
        Instance.destroy(self, null);
    }
};

pub fn IDHashMap(comptime valueType: type) type {
    return std.HashMap(u64, valueType, IDHashContext, 80);
}
pub fn IDArrayHashMap(comptime valueType: type) type {
    return std.ArrayHashMap(u64, valueType, IDArrayHashContext, true);
}

pub const IDHashContext = struct {
    hashModulus: u32,

    pub fn hash(self: @This(), key: u64) u32 {
        return @as(u32, key % self.hashModulus);
    }

    pub fn eql(self: @This(), key1: u64, key2: u64) bool {
        _ = self;
        return key1 == key2;
    }
};

pub const IDArrayHashContext = struct {
    hashModulus: u32,

    pub fn hash(self: @This(), key: u64) u32 {
        return @as(u32, key % self.hashModulus);
    }

    pub fn eql(self: @This(), key1: u64, key2: u64, inMap: u64) bool {
        _ = self; _ = inMap;
        return key1 == key2;
    }
};

pub const IDGenerator = struct {
    lastGeneratedID: u64,
    timeout: u64,

    pub const Self = @This();
    pub const default = Self{.lastGeneratedID = 0, .timeout = MAX_OBJECTS};    

    pub fn nextID(self: *@This(), hashMap: *anyopaque) ?u64 {
        var ID = self.lastGeneratedID; 
        var timeout = 0;
        while(hashMap.*.contains(ID) and timeout < self.timeout) {
            ID +%= 1;
            timeout += 1;
        }
        if(timeout >= self.timeout) return null;
        self.lastGeneratedID = ID;
        return ID;
    }
};

pub const Buffer = struct {
    handle: c.VkBuffer,
    memory: c.VkDeviceMemory,

    pub fn create(physical: c.VkPhysicalDevice, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks, size: u32, usage: c.VkBufferUsageFlags, properties: c.VkMemoryPropertyFlags) !Buffer {
        var result: Buffer = undefined;
        const bufferInfo: c.VkBufferCreateInfo = .{ .sType = c.VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = usage, .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE };
        if (c.vkCreateBuffer(device, &bufferInfo, allocator, &result.handle) != c.VK_SUCCESS) {
            return error.failedToCreateBuffer;
        }

        var memoryRequirements: c.VkMemoryRequirements = undefined;
        c.vkGetBufferMemoryRequirements(device, result.handle, &memoryRequirements);

        const memoryAllocationInfo = c.VkMemoryAllocateInfo{ .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = memoryRequirements.size, .memoryTypeIndex = try findPhysicalDeviceMemoryTypeIndex(physical, memoryRequirements.memoryTypeBits, properties) };
        if (c.vkAllocateMemory(device, &memoryAllocationInfo, allocator, &result.memory) != c.VK_SUCCESS) {
            return error.failedToAllocateMemory;
        }
        if (c.vkBindBufferMemory(device, result.handle, result.memory, 0) != c.VK_SUCCESS) {
            return error.failedToBindBufferMemory;
        }
        return result;
    }

    pub fn destroy(self: *Buffer, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkDestroyBuffer(device, self.*.handle, allocator);
        c.vkFreeMemory(device, self.*.memory, allocator);
    }
};

pub const UniformBuffer = struct {
    handle: c.VkBuffer,
    deviceMemory: c.VkDeviceMemory,
    hostMemory: []align(16) Math.Mat4,

    pub fn create(state: *const GPUState, allocator: ?*c.VkAllocationCallbacks) !UniformBuffer {
        var result: UniformBuffer = undefined;
        const buffer = try Buffer.create(state.physicalDevice, state.device, allocator, @sizeOf(Math.Mat4) * MAX_FRAMES_IN_FLIGHT, c.VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        result.handle = buffer.handle;
        result.deviceMemory = buffer.memory;
        if (c.vkMapMemory(state.device, result.deviceMemory, 0, @sizeOf(Math.Mat4), 0, @ptrCast(&result.hostMemory.ptr)) != c.VK_SUCCESS) {
            return error.failedToMapMemory;
        }
        result.hostMemory.len = MAX_FRAMES_IN_FLIGHT;
        for (result.hostMemory) |*matrix| matrix.* = Math.Mat4.identity;
        return result;
    }

    pub fn destroy(self: *UniformBuffer, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkUnmapMemory(device, self.deviceMemory);
        c.vkDestroyBuffer(device, self.handle, allocator);
        c.vkFreeMemory(device, self.deviceMemory, allocator);
    }
};

pub const Image = struct {
    handle: c.VkImage,
    view: c.VkImageView,
    memory: c.VkDeviceMemory,

    pub fn create(physicalDevice: c.VkPhysicalDevice, device: c.VkDevice, 
        allocator: ?*c.VkAllocationCallbacks, format: c.VkFormat, extent: c.VkExtent2D, 
        usage: c.VkImageUsageFlags, memProperties: c.VkMemoryPropertyFlags, 
        aspect: c.VkImageAspectFlags
    ) !Image {
        var result: Image = undefined;
        const imageInfo = c.VkImageCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = c.VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = c.VkExtent3D{.width = extent.width, .height = extent.height, .depth = 1},
            .arrayLayers = 1,
            .mipLevels = 1,
            .initialLayout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE,
            .samples = 1,
            .usage = usage,
            .flags = 0,
            .tiling = c.VK_IMAGE_TILING_OPTIMAL
        };
        if(c.vkCreateImage(device, &imageInfo, allocator,&result.handle) != c.VK_SUCCESS) {
            return error.failedToCreateImage;
        }
        var memRequirements: c.VkMemoryRequirements = undefined;
        c.vkGetImageMemoryRequirements(device, result.handle, &memRequirements);

        const allocInfo = c.VkMemoryAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = try findPhysicalDeviceMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, memProperties)
        };
        if(c.vkAllocateMemory(device, &allocInfo, allocator, &result.memory) != c.VK_SUCCESS) {
            return error.failedToAllocateMemory;
        }

        _ = c.vkBindImageMemory(device, result.handle, result.memory, 0);

        const imageViewInfo = c.VkImageViewCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = result.handle,
            .viewType = c.VK_IMAGE_VIEW_TYPE_2D,
            .components = c.VkComponentMapping{
                .r = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = c.VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .format = format,
            .flags = 0,
            .subresourceRange = c.VkImageSubresourceRange{
                .aspectMask = aspect,
                .baseArrayLayer = 0,
                .baseMipLevel = 0,
                .layerCount = 1,
                .levelCount = 1
            }
        };
        if(c.vkCreateImageView(device, &imageViewInfo, allocator, &result.view) != c.VK_SUCCESS) {
            return error.failedToCreateImageView;
        }
        return result;
    }

    pub fn destroy(self: *const Image, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkDestroyImageView(device, self.view, allocator);
        c.vkFreeMemory(device, self.memory, allocator);
        c.vkDestroyImage(device, self.handle, allocator);
    }
};

pub fn findPhysicalDeviceMemoryTypeIndex(physical: c.VkPhysicalDevice, availableTypes: u32, properties: c.VkMemoryPropertyFlags) !u32 {
    var deviceMemoryProperties = c.VkPhysicalDeviceMemoryProperties2{ .sType = c.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
    c.vkGetPhysicalDeviceMemoryProperties2(physical, &deviceMemoryProperties);

    for (deviceMemoryProperties.memoryProperties.memoryTypes, 0..) |memType, i| {
        const currentTypeBit: u32 = @as(u32, @as(u32, 1) << @intCast(i));
        if (memType.propertyFlags & properties == properties and availableTypes & currentTypeBit != 0) {
            return @intCast(i);
        }
    }

    return error.failedToFindMemoryType;
}

pub const CameraPushConstant = struct {
    view: Math.Mat4 align(16),
    perspective: Math.Mat4 align(16),

    pub const default = CameraPushConstant{ .view = Math.Mat4.identity, .perspective = Math.Mat4.identity };
};

pub const Queues = struct {
    graphicsQueues: []c.VkQueue,
    presentQueue: c.VkQueue,
    transferQueue: c.VkQueue,
};

pub const QueueFamilyIndices = struct {
    graphicsIndex: ?u32 = null,
    presentIndex: ?u32 = null,
    graphicsQueueCount: u32 = 0,
    presentQueueCount: u32 = 0,

    pub fn isComplete(self: *QueueFamilyIndices) bool {
        return self.*.graphicsIndex != null and self.*.presentIndex != null;
    }
};

pub fn findQueueFamilyIndices(device: c.VkPhysicalDevice, surface: c.VkSurfaceKHR) !QueueFamilyIndices {
    var queueFamilyPropertiesCount: u32 = 0;
    _ = c.vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertiesCount, null);
    const queueFamilyProperties = try std.heap.page_allocator.alloc(c.VkQueueFamilyProperties2, queueFamilyPropertiesCount);
    defer std.heap.page_allocator.free(queueFamilyProperties);
    //Each property struct needs to have its type manually assigned for the next function call
    //  to work
    for (queueFamilyProperties) |*property| {
        property.* = .{ .sType = c.VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 };
    }
    _ = c.vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertiesCount, queueFamilyProperties.ptr);

    var result = QueueFamilyIndices{};
    for (0..queueFamilyProperties.len) |i| {
        const properties = queueFamilyProperties[i].queueFamilyProperties;

        //  According to the vulkan spec, there is always a queue family that supports both graphics
        //      AND compute operations
        if (properties.queueFlags & (c.VK_QUEUE_COMPUTE_BIT | c.VK_QUEUE_GRAPHICS_BIT) != 0) {
            result.graphicsIndex = @intCast(i);
            result.graphicsQueueCount = properties.queueCount;
        }
        var presentSupport: c.VkBool32 = c.VK_FALSE;
        if (c.vkGetPhysicalDeviceSurfaceSupportKHR(device, @intCast(i), surface, &presentSupport) != c.VK_SUCCESS) {
            return error.failedToGetPhysicalDeviceSurfaceSupport;
        }
        if (presentSupport == c.VK_TRUE) {
            result.presentIndex = @intCast(i);
            result.presentQueueCount = properties.queueCount;
        }

        if (result.isComplete()) {
            return result;
        }
    }

    return error.queueFamiliesUnsupported;
}
pub const SurfaceInfo = struct {
    capabilities: c.VkSurfaceCapabilitiesKHR = undefined,
    formats: ?[]c.VkSurfaceFormatKHR = null,
    presentModes: ?[]c.VkPresentModeKHR = null,

    pub fn query(self: *SurfaceInfo, device: c.VkPhysicalDevice, surface: c.VkSurfaceKHR) !void {
        _ = c.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &self.*.capabilities);

        var surfaceFormatCount: usize = 0;
        _ = c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, @ptrCast(&surfaceFormatCount), null);
        if (surfaceFormatCount != 0) {
            self.*.formats = try std.heap.page_allocator.alloc(c.VkSurfaceFormatKHR, surfaceFormatCount);
            _ = c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, @ptrCast(&surfaceFormatCount), self.*.formats.?.ptr);
        }

        var presentModeCount: usize = 0;
        _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, @ptrCast(&presentModeCount), null);
        if (presentModeCount != 0) {
            self.*.presentModes = try std.heap.page_allocator.alloc(c.VkPresentModeKHR, presentModeCount);
            _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, @ptrCast(&presentModeCount), self.*.presentModes.?.ptr);
        }
    }

    pub fn free(self: *SurfaceInfo) void {
        if (self.*.formats) |formats| {
            std.heap.page_allocator.free(formats);
        }
        if (self.*.presentModes) |presentModes| {
            std.heap.page_allocator.free(presentModes);
        }
    }
};

pub fn beginSingleTimeCommandBuffer(device: c.VkDevice, commandPool: c.VkCommandPool) !c.VkCommandBuffer {
    var commandBuffer: c.VkCommandBuffer = undefined;
    var allocInfo = c.VkCommandBufferAllocateInfo{ .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = commandPool, .commandBufferCount = 1, .level = c.VK_COMMAND_BUFFER_LEVEL_PRIMARY };
    if (c.vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != c.VK_SUCCESS) {
        return error.failedToAllocateSingleTimeCommandBuffer;
    }
    var beginInfo = c.VkCommandBufferBeginInfo{ .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = c.VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    if (c.vkBeginCommandBuffer(commandBuffer, &beginInfo) != c.VK_SUCCESS) {
        return error.failedToBeginSingleTimeCommandBuffer;
    }
    return commandBuffer;
}


pub const Vertex = struct {
    color: [3]f32,
    position: [3]f32,
    texCoord: [2]f32,

    const Attributes = enum(u32) { COLOR, POSITION, TEXCOORD };
    const numAttribs: usize = @typeInfo(Attributes).@"enum".fields.len;

    pub fn create(inColor: [3]f32, inPosition: [3]f32, inTexCoord: [2]f32) Vertex {
        return .{ .position = inPosition, .color = inColor, .texCoord = inTexCoord };
    }

    pub fn inputBindingDescription() c.VkVertexInputBindingDescription {
        return .{ .binding = 0, .stride = @sizeOf(Vertex), .inputRate = c.VK_VERTEX_INPUT_RATE_VERTEX };
    }

    pub fn inputAttributeDescriptions() [numAttribs]c.VkVertexInputAttributeDescription {
        var attributes: [numAttribs]c.VkVertexInputAttributeDescription = undefined;
        attributes[@intFromEnum(Attributes.COLOR)] = .{ 
            .location = @intFromEnum(Attributes.COLOR), 
            .binding = 0, 
            .format = c.VK_FORMAT_R32G32B32_SFLOAT, 
            .offset = 0 
        };
        attributes[@intFromEnum(Attributes.POSITION)] = .{ 
            .location = @intFromEnum(Attributes.POSITION), 
            .binding = 0, 
            .format = c.VK_FORMAT_R32G32B32_SFLOAT, 
            .offset = @offsetOf(Vertex, "position") 
        };
        attributes[@intFromEnum(Attributes.TEXCOORD)] = .{
            .location = @intFromEnum(Attributes.TEXCOORD),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32_SFLOAT,
            .offset = @offsetOf(Vertex, "texCoord")
        };
        return attributes;
    }
};

pub const Pos = struct {
    x: f32,
    y: f32,
    z: f32,

    pub fn vector(pos: *const Pos) Math.Vec3 {
        return Math.Vec3.init(.{ pos.x, pos.y, pos.z });
    }
};

pub const Camera = struct {
    pos: Pos,
    dir: Math.Vec3,
    upVector: Math.Vec3,
    fov: f32,

    pub const default = Camera{
        .pos = .{ .x = 0.0, .y = 0.0, .z = 0.0 },
        .dir = .init(.{ 0.0, 0.0, 1.0 }),
        .upVector = .init(.{ 0.0, 1.0, 0.0 }),
        .fov = std.math.pi / 2.0,
    };

    pub fn setTarget(self: *Camera, targetPos: Pos) void {
        self.dir = targetPos.vector().subtract(self.pos.vector());
    }
};

pub const Mesh = struct {
    buffer: Buffer,
    /// The size of the chunk of buffer memory containing vertex data, in bytes.
    verticesSize: u32,
    /// The size of the chunk of buffer memory containing index data, in bytes.
    indicesSize: u32,
    id: u64,

    pub fn create(state: *GPUState, vertices: []const Vertex, indices: []const u32, allocator: ?*c.VkAllocationCallbacks) !Mesh {
        //  Set flag that we began loading meshes if needed
        if(state.notFlagSet(Flags.BEGAN_MESH_LOADING)) {
            state.activeFlags |= Flags.BEGAN_MESH_LOADING;
            const cmdBeginInfo = c.VkCommandBufferBeginInfo{
                .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = c.VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            };
            if(c.vkBeginCommandBuffer(state.*.transferCommandBuffer, &cmdBeginInfo) != c.VK_SUCCESS) {
                return error.failedToBeginCommandBuffer;
            }
        }

        //  Initialize the Mesh object
        var result: Mesh = undefined;
        result.verticesSize = @as(u32, @intCast(vertices.len)) * @sizeOf(Vertex);
        result.indicesSize = @as(u32, @intCast(indices.len)) * @sizeOf(u32);
        result.id = state.appState.IDGen.nextID(&state.meshes) orelse unreachable;
        result.buffer = try Buffer.create(state.physicalDevice, state.device, allocator, 
            result.verticesSize + result.indicesSize, 
            c.VK_BUFFER_USAGE_VERTEX_BUFFER_BIT 
                | c.VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
                | c.VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            c.VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        //  Initialize the staging buffer for this Mesh's loading operation
        const stagingBuffer = try Buffer.create(state.physicalDevice, state.device, allocator,
            result.verticesSize + result.indicesSize, 
            c.VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        var memory: ?*anyopaque = null;
        if(c.vkMapMemory(state.device, stagingBuffer.memory, 0, result.verticesSize, 0, &memory) != c.VK_SUCCESS) {
            return error.failedToMapMemory;
        }
        @memcpy(@as([*]Vertex, @ptrCast(@alignCast(memory))), vertices);
        c.vkUnmapMemory(state.device, stagingBuffer.memory);

        if(c.vkMapMemory(state.device, stagingBuffer.memory, result.verticesSize, result.indicesSize, 0, &memory) != c.VK_SUCCESS) {
            return error.failedToMapMemory;
        }
        @memcpy(@as([*]u32, @ptrCast(@alignCast(memory))), indices);
        c.vkUnmapMemory(state.device, stagingBuffer.memory);

        c.vkCmdCopyBuffer(state.*.transferCommandBuffer, stagingBuffer.handle, result.buffer.handle, 1, 
            &.{
                .srcOffset = 0, 
                .dstOffset = 0, 
                .size = result.verticesSize + result.indicesSize
            }
        );
        //  Add the staging buffer to the state's list so that it can be used and freed later
        try state.transferStagingBuffers.append(stagingBuffer);

        return result;
    }

    pub fn destroy(self: *Mesh, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        self.buffer.destroy(device, allocator);
        self.objects.deinit();
    }
};

pub const Model = struct {
    mesh: u64,
    texture: u64,
    id: u64,
    objects: std.ArrayList(u64),

    pub fn create(state: *GPUState, mesh: u64, texture: u64) Model {
        return .{
            .mesh = mesh,
            .texture = texture,
            .id = state.appState.IDGen.nextID(state.models) orelse return error.failedToGenerateID,
            .objects = std.ArrayList(u64).init(std.heap.page_allocator)
        };
    }

    pub fn destroy(self: *Model, state: *GPUState) void {
        self.objects.deinit();
        state.models.swapRemove(self.id);
    }
};

pub const Object = struct {
    pos: Pos,
    scale: Pos,
    orientation: Math.Vec3,
    angle: f32,
    id: u64,
    model: u64,
    uniformBuffer: UniformBuffer,
    descriptorSets: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSet,

    pub fn getModelMatrix(self: *const Object) Math.Mat4 {
        const scale = Math.scaleMatrix(.{ self.scale.x, self.scale.y, self.scale.z });
        const translation = Math.translate(self.pos.vector());
        const centerPos = self.pos.vector().add(self.scale.vector().scale(0.5));
        const rotation = Math.translate(centerPos).mult(Math.rotate(self.angle, self.orientation))
            .mult(Math.translate(centerPos.scale(-1.0)));
        return rotation.mult(translation).mult(scale);
    }

    pub fn create(state: *State, pos: Pos, scaling: Pos, orientation: Math.Vec3, angle: f32, model: u64, allocator: ?*c.VkAllocationCallbacks) !Object {
        const gpu = &state.GPU;
        var result: Object = undefined;
        result.pos = pos;
        result.scale = scaling;
        result.orientation = orientation;
        result.angle = angle;
        result.model = model;
        result.id = state.IDGen.nextID(&state.objects) orelse return error.outOfObjectSpace;
        state.GPU.models.getPtr(model).?.objects.append(result.id);
        result.uniformBuffer = try UniformBuffer.create(gpu, allocator);
        for (result.uniformBuffer.hostMemory) |*modelMatrix| modelMatrix.* = Math.Mat4.identity;

        const layouts: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSetLayout = .{gpu.descriptorSetLayout} ** MAX_FRAMES_IN_FLIGHT;
        const allocInfo = c.VkDescriptorSetAllocateInfo{ 
            .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, 
            .descriptorPool = gpu.descriptorPool, 
            .descriptorSetCount = result.descriptorSets.len, 
            .pSetLayouts = layouts[0..] 
        };
        if (c.vkAllocateDescriptorSets(gpu.device, &allocInfo, &result.descriptorSets) != c.VK_SUCCESS) {
            return error.failedToAllocateDescriptorSets;
        }

        for (0..result.descriptorSets.len) |i| {
            const writeDescriptorSet = c.VkWriteDescriptorSet{ 
                .sType = c.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 
                .dstSet = result.descriptorSets[i], 
                .dstBinding = 0, 
                .descriptorCount = 1, 
                .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                .pBufferInfo = &.{ 
                    .buffer = result.uniformBuffer.handle, 
                    .offset = @sizeOf(Math.Mat4) * i, 
                    .range = @sizeOf(Math.Mat4) 
                } 
            };
            c.vkUpdateDescriptorSets(gpu.device, 1, &writeDescriptorSet, 0, null);
        }
        return result;
    }

    pub fn destroy(self: *Object, state: *State, device: c.VkDevice, pool: c.VkDescriptorPool, allocator: ?*c.VkAllocationCallbacks) void {
        self.uniformBuffer.destroy(device, allocator);
        _ = c.vkFreeDescriptorSets(device, pool, self.descriptorSets.len, &self.descriptorSets);
        state.objects.swapRemove(self.id);
        //  Remove ID of the object from the list held by its model
        for(state.GPU.models.getPtr(self.model).?.objects.items, 0..) |objectId, i| {
            if(objectId == self.id) {
                state.GPU.models.getPtr(self.model).?.objects.swapRemove(i);
                break;
            }
        }
    }
};
