const std = @import("std");
const c = @import("c");
const Math = @import("math.zig");

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
};

pub const GPUState = struct {
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
    meshes: std.ArrayList(Mesh),
    objects: ObjectArrayHashMap,
    lastGeneratedId: u64,
    transferStagingBuffers: std.ArrayList(Buffer),

    /// Returns true is the provided flags are set, since zig is so anal about truthy values
    pub fn isFlagSet(Self: *GPUState, flag: MarsFlags) bool {
        return Self.*.activeFlags & flag != 0;
    }
    /// Returns true if the provided flags are not set, since zig is so anal about truthy values
    pub fn notFlagSet(Self: *GPUState, flag: MarsFlags) bool {
        return Self.*.activeFlags & flag == 0;
    }
};

pub const ObjectArrayHashMap = std.ArrayHashMap(u64, Object, Object.HashContext, true);

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

    pub fn destroy(Self: *Buffer, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkDestroyBuffer(device, Self.*.handle, allocator);
        c.vkFreeMemory(device, Self.*.memory, allocator);
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

    pub fn destroy(Self: *UniformBuffer, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkUnmapMemory(device, Self.deviceMemory);
        c.vkDestroyBuffer(device, Self.handle, allocator);
        c.vkFreeMemory(device, Self.deviceMemory, allocator);
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

    pub fn destroy(Self: *const Image, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkDestroyImageView(device, Self.view, allocator);
        c.vkFreeMemory(device, Self.memory, allocator);
        c.vkDestroyImage(device, Self.handle, allocator);
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

    pub fn isComplete(Self: *QueueFamilyIndices) bool {
        return Self.*.graphicsIndex != null and Self.*.presentIndex != null;
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

    pub fn query(Self: *SurfaceInfo, device: c.VkPhysicalDevice, surface: c.VkSurfaceKHR) !void {
        _ = c.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &Self.*.capabilities);

        var surfaceFormatCount: usize = 0;
        _ = c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, @ptrCast(&surfaceFormatCount), null);
        if (surfaceFormatCount != 0) {
            Self.*.formats = try std.heap.page_allocator.alloc(c.VkSurfaceFormatKHR, surfaceFormatCount);
            _ = c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, @ptrCast(&surfaceFormatCount), Self.*.formats.?.ptr);
        }

        var presentModeCount: usize = 0;
        _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, @ptrCast(&presentModeCount), null);
        if (presentModeCount != 0) {
            Self.*.presentModes = try std.heap.page_allocator.alloc(c.VkPresentModeKHR, presentModeCount);
            _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, @ptrCast(&presentModeCount), Self.*.presentModes.?.ptr);
        }
    }

    pub fn free(Self: *SurfaceInfo) void {
        if (Self.*.formats) |formats| {
            std.heap.page_allocator.free(formats);
        }
        if (Self.*.presentModes) |presentModes| {
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

const VertexAttributes: type = enum(u32) { COLOR = 0, POSITION };

pub const Vertex = struct {
    color: [3]f32,
    position: [3]f32,

    const numAttribs: usize = @typeInfo(@This()).@"struct".fields.len;

    pub fn create(inColor: [3]f32, inPosition: [3]f32) Vertex {
        return .{ .position = inPosition, .color = inColor };
    }

    pub fn inputBindingDescription() c.VkVertexInputBindingDescription {
        return .{ .binding = 0, .stride = @sizeOf(Vertex), .inputRate = c.VK_VERTEX_INPUT_RATE_VERTEX };
    }

    pub fn inputAttributeDescriptions() [numAttribs]c.VkVertexInputAttributeDescription {
        var attributes: [numAttribs]c.VkVertexInputAttributeDescription = undefined;
        attributes[@intFromEnum(VertexAttributes.COLOR)] = .{ .location = @intFromEnum(VertexAttributes.COLOR), .binding = 0, .format = c.VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 };
        attributes[@intFromEnum(VertexAttributes.POSITION)] = .{ .location = @intFromEnum(VertexAttributes.POSITION), .binding = 0, .format = c.VK_FORMAT_R32G32B32_SFLOAT, .offset = @offsetOf(Vertex, "position") };
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
        .dir = Math.Vec3.init(.{ 0.0, 0.0, 1.0 }),
        .upVector = Math.Vec3.init(.{ 0.0, 1.0, 0.0 }),
        .fov = std.math.pi / 2.0,
    };

    pub fn setTarget(Self: *Camera, targetPos: Pos) void {
        Self.dir = targetPos.vector().subtract(Self.pos.vector());
    }
};

pub const Mesh = struct {
    buffer: Buffer,
    /// The size of the chunk of buffer memory containing vertex data, in bytes.
    verticesSize: u32,
    /// The size of the chunk of buffer memory containing index data, in bytes.
    indicesSize: u32,
    objects: std.ArrayList(u64),

    pub fn destroy(Self: *Mesh, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        Self.buffer.destroy(device, allocator);
        Self.objects.deinit();
    }
};

pub const Model = struct {
    mesh: *Mesh,
    texture: *const Image,

    pub fn create(mesh: *Mesh, texture: *const Image) Model {
        return .{ .mesh = mesh, .texture = texture };
    }
};

pub const Object = struct {
    pos: Pos,
    scale: Pos,
    orientation: Math.Vec3,
    angle: f32,
    id: u64,
    mesh: *Mesh,
    uniformBuffer: UniformBuffer,
    descriptorSets: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSet,

    ///This structure contains information that the zig standard library ArrayHashMap needs.
    pub const HashContext = struct {
        hashModulus: u32,

        ///This hashing function must match the signature demanded by the zig standard library ArrayHashMap.
        pub fn hash(Self: HashContext, key: u64) u32 {
            return @intCast(key % Self.hashModulus);
        }

        ///This equality function must match the signature demanded by the zig standard library ArrayHashMap.
        pub fn eql(Self: HashContext, key1: u64, key2: u64, inMap: usize) bool {
            _ = Self;
            _ = inMap;
            return key1 == key2;
        }
    };

    pub fn getModelMatrix(Self: *const Object) Math.Mat4 {
        const scale = Math.scaleMatrix(.{ Self.scale.x, Self.scale.y, Self.scale.z });
        const translation = Math.translate(Self.pos.vector());
        const centerPos = Self.pos.vector().add(Self.scale.vector().scale(0.5));
        const rotation = Math.translate(centerPos).mult(Math.rotate(Self.angle, Self.orientation))
            .mult(Math.translate(centerPos.scale(-1.0)));
        return rotation.mult(translation).mult(scale);
    }

    pub fn create(state: *GPUState, pos: Pos, scaling: Pos, orientation: Math.Vec3, angle: f32, mesh: *Mesh, allocator: ?*c.VkAllocationCallbacks) !Object {
        var result: Object = undefined;
        result.pos = pos;
        result.scale = scaling;
        result.orientation = orientation;
        result.angle = angle;
        result.mesh = mesh;
        result.id = state.lastGeneratedId + 1;
        while (state.objects.contains(result.id)) {
            result.id += 1;
        }
        state.lastGeneratedId = result.id;
        result.mesh.objects.appendAssumeCapacity(result.id);
        result.uniformBuffer = try UniformBuffer.create(state, allocator);
        for (result.uniformBuffer.hostMemory) |*modelMatrix| modelMatrix.* = Math.Mat4.identity;

        const layouts: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSetLayout = .{state.descriptorSetLayout} ** MAX_FRAMES_IN_FLIGHT;
        const allocInfo = c.VkDescriptorSetAllocateInfo{ .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = state.descriptorPool, .descriptorSetCount = result.descriptorSets.len, .pSetLayouts = layouts[0..] };
        if (c.vkAllocateDescriptorSets(state.device, &allocInfo, &result.descriptorSets) != c.VK_SUCCESS) {
            return error.failedToAllocateDescriptorSets;
        }

        for (0..result.descriptorSets.len) |i| {
            const writeDescriptorSet = c.VkWriteDescriptorSet{ .sType = c.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = result.descriptorSets[i], .dstBinding = 0, .descriptorCount = 1, .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .pBufferInfo = &.{ .buffer = result.uniformBuffer.handle, .offset = @sizeOf(Math.Mat4) * i, .range = @sizeOf(Math.Mat4) } };
            c.vkUpdateDescriptorSets(state.device, 1, &writeDescriptorSet, 0, null);
        }
        return result;
    }

    pub fn destroy(Self: *Object, device: c.VkDevice, pool: c.VkDescriptorPool, allocator: ?*c.VkAllocationCallbacks) void {
        Self.uniformBuffer.destroy(device, allocator);
        _ = c.vkFreeDescriptorSets(device, pool, Self.descriptorSets.len, &Self.descriptorSets);
        for (0..Self.mesh.objects.items.len) |i| {
            if (Self.mesh.objects.items[i] == Self.id) {
                _ = Self.mesh.objects.swapRemove(i);
                break;
            }
        }
    }
};
