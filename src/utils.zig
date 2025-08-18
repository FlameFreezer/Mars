const std = @import("std");
const c = @import("c");

pub const MAX_FRAMES_IN_FLIGHT: usize = 2;
pub const MAX_OBJECTS: usize = 512;

pub const MarsFlags = u16;
pub const Flags = struct {
    pub const NONE: MarsFlags = 0;
    pub const WINDOW_IS_MINIMIZED: MarsFlags = 1;
    pub const WINDOW_IS_MAXIMIZED: MarsFlags = 1 << 1;
    pub const WINDOW_SHOULD_CLOSE: MarsFlags = 1 << 2;
    pub const IS_PAUSED: MarsFlags = 1 << 3;
};

pub const State = struct {
    name: []const u8,
    nextEvent: c.SDL_Event,
    programStartTime: i64,
    time: i64,
    deltaTimeUs: i64,
    elapsedTime: i64,
    camera: Camera,
    activeFlags: MarsFlags = 0,
    window: *c.SDL_Window,
    instance: c.VkInstance,
    debugMessenger: c.VkDebugUtilsMessengerEXT,
    physicalDevice: c.VkPhysicalDevice,
    device: c.VkDevice,
    surface: c.VkSurfaceKHR,
    queues: Queues,
    swapchain: c.VkSwapchainKHR,
    swapchainImages: []c.VkImage,
    swapchainImageViews: []c.VkImageView,
    swapchainExtent: c.VkExtent2D,
    commandPool: c.VkCommandPool,
    commandBuffers: [MAX_FRAMES_IN_FLIGHT]c.VkCommandBuffer,
    cameraPushConstant: CameraPushConstant,
    graphicsPipelineLayout: c.VkPipelineLayout,
    graphicsPipeline: c.VkPipeline,
    fences: [MAX_FRAMES_IN_FLIGHT]c.VkFence,
    semaphores: []c.VkSemaphore,
    currentFrame: usize = 0,
    descriptorSetLayout: c.VkDescriptorSetLayout,
    descriptorPool: c.VkDescriptorPool,
    depthImage: c.VkImage,
    depthImageView: c.VkImageView,
    depthImageMemory: c.VkDeviceMemory,
    meshes: std.ArrayList(Mesh),
    objects: ObjectArrayHashMap,
    lastGeneratedId: u64,
};

pub const ObjectArrayHashMap = std.ArrayHashMap(u64, Object, Object.HashContext, true);

pub const Buffer = struct {
    handle: c.VkBuffer,
    memory: c.VkDeviceMemory,

    pub fn create(physical: c.VkPhysicalDevice, device: c.VkDevice, 
        allocator: ?*c.VkAllocationCallbacks, size: u32, usage: c.VkBufferUsageFlags, 
        properties: c.VkMemoryPropertyFlags
    ) !Buffer {
        var result: Buffer = undefined;
        const bufferInfo: c.VkBufferCreateInfo = .{
            .sType = c.VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usage,
            .sharingMode = c.VK_SHARING_MODE_EXCLUSIVE
        };
        if(c.vkCreateBuffer(device, &bufferInfo, allocator, &result.handle) != c.VK_SUCCESS) {
            return error.failedToCreateBuffer;
        }

        var memoryRequirements: c.VkMemoryRequirements = undefined;
        c.vkGetBufferMemoryRequirements(device, result.handle, &memoryRequirements);

        const memoryAllocationInfo = c.VkMemoryAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = try findPhysicalDeviceMemoryTypeIndex(physical, memoryRequirements.memoryTypeBits, properties)
        };
        if(c.vkAllocateMemory(device, &memoryAllocationInfo, allocator, &result.memory) != c.VK_SUCCESS) {
            return error.failedToAllocateMemory;
        }
        if(c.vkBindBufferMemory(device, result.handle, result.memory, 0) != c.VK_SUCCESS) {
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
    hostMemory: []align(16) Mat(4),

    pub fn create(state: *const State, allocator: ?*c.VkAllocationCallbacks) !UniformBuffer {
        var result: UniformBuffer = undefined;
        const buffer = try Buffer.create(state.physicalDevice, state.device, allocator, @sizeOf(Mat(4)) * MAX_FRAMES_IN_FLIGHT,
            c.VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            c.VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | c.VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        result.handle = buffer.handle;
        result.deviceMemory = buffer.memory;
        if(c.vkMapMemory(state.device, result.deviceMemory, 0, @sizeOf(Mat(4)), 0, @ptrCast(&result.hostMemory.ptr)) != c.VK_SUCCESS) {
            return error.failedToMapMemory;
        }
        result.hostMemory.len = MAX_FRAMES_IN_FLIGHT;
        for(result.hostMemory) |*matrix| matrix.* = Mat(4).identity;
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

    pub fn create(Self: *Image, physicalDevice: c.VkPhysicalDevice, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) !void {
       _ = Self;
       _ = physicalDevice;
       _ = device;
       _ = allocator; 
    }

    pub fn destroy(Self: *const Image, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        _ = Self;
        _ = device;
        _ = allocator;
    }
};

pub fn findPhysicalDeviceMemoryTypeIndex(physical: c.VkPhysicalDevice, availableTypes: u32, 
        properties: c.VkMemoryPropertyFlags
    ) !u32 {
        var deviceMemoryProperties = c.VkPhysicalDeviceMemoryProperties2{
            .sType = c.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
        };
        c.vkGetPhysicalDeviceMemoryProperties2(physical, &deviceMemoryProperties);

        for(deviceMemoryProperties.memoryProperties.memoryTypes, 0..) |memType, i| {
            const currentTypeBit: u32 = @as(u32, @as(u32, 1) << @intCast(i));
            if(memType.propertyFlags & properties == properties and availableTypes & currentTypeBit != 0) {
                return @intCast(i);
            }
        }

        return error.failedToFindMemoryType;
    }

pub const CameraPushConstant = struct {
    view: Mat(4) align(16),
    perspective: Mat(4) align(16),

    pub const default = CameraPushConstant{
        .view = Mat(4).identity,
        .perspective = Mat(4).identity
    };
};

pub const Queues = struct {
    graphics: c.VkQueue,
    compute: c.VkQueue,
    present: c.VkQueue,
    transfer: c.VkQueue,
};

pub const QueueFamilyIndices = struct {
    graphicsIndex: ?u32 = null,
    computeIndex: ?u32 = null,
    presentIndex: ?u32 = null,
    transferIndex: ?u32 = null,

    pub fn isComplete(Self: *QueueFamilyIndices) bool {
        inline for(@typeInfo(QueueFamilyIndices).@"struct".fields) |field| {
            if(@field(Self.*, field.name) == null) return false;
        }
        return true;
    }
};

pub fn findQueueFamilyIndices(device: c.VkPhysicalDevice, surface: c.VkSurfaceKHR) !QueueFamilyIndices {
    var queueFamilyPropertiesCount: u32 = 0;
    _ = c.vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertiesCount, null);
    const queueFamilyProperties = try std.heap.page_allocator.alloc(c.VkQueueFamilyProperties2, queueFamilyPropertiesCount);
    defer std.heap.page_allocator.free(queueFamilyProperties);
    //Each property struct needs to have its type manually assigned for the next function call
    //  to work
    for(0..queueFamilyPropertiesCount) |i| {
        queueFamilyProperties[i] = .{
            .sType = c.VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2
        };
    }
    _ = c.vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertiesCount, queueFamilyProperties.ptr);

    for(0..queueFamilyProperties.len) |i| {
        const properties = queueFamilyProperties[i].queueFamilyProperties;
        var deviceQueueFamilyIndices = QueueFamilyIndices{};

        if(properties.queueFlags & c.VK_QUEUE_COMPUTE_BIT != 0) {
            deviceQueueFamilyIndices.computeIndex = @intCast(i);
        }
        if(properties.queueFlags & c.VK_QUEUE_GRAPHICS_BIT != 0) {
            deviceQueueFamilyIndices.graphicsIndex = @intCast(i);
        }
        if(properties.queueFlags & c.VK_QUEUE_TRANSFER_BIT != 0) {
            deviceQueueFamilyIndices.transferIndex = @intCast(i);
        }
        var presentSupport: c.VkBool32 = c.VK_FALSE;
        if(c.vkGetPhysicalDeviceSurfaceSupportKHR(device, @intCast(i), surface, &presentSupport) != c.VK_SUCCESS) {
            return error.failedToGetPhysicalDeviceSurfaceSupport;
        }
        if(presentSupport == c.VK_TRUE) {
           deviceQueueFamilyIndices.presentIndex = @intCast(i);
        }

        if(deviceQueueFamilyIndices.isComplete()) {
            return deviceQueueFamilyIndices;
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
        if(surfaceFormatCount != 0) {
            Self.*.formats = try std.heap.page_allocator.alloc(c.VkSurfaceFormatKHR, surfaceFormatCount);
            _ = c.vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, @ptrCast(&surfaceFormatCount), Self.*.formats.?.ptr);
        }

        var presentModeCount: usize = 0;
        _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, @ptrCast(&presentModeCount), null);
        if(presentModeCount != 0) {
            Self.*.presentModes = try std.heap.page_allocator.alloc(c.VkPresentModeKHR, presentModeCount);
            _ = c.vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, @ptrCast(&presentModeCount), Self.*.presentModes.?.ptr);
        }
    }

    pub fn free(Self: *SurfaceInfo) void {
        if(Self.*.formats) |formats| {
            std.heap.page_allocator.free(formats);
        }
        if(Self.*.presentModes) |presentModes| {
            std.heap.page_allocator.free(presentModes);
        }
    }
};

pub fn beginSingleTimeCommandBuffer(device: c.VkDevice, commandPool: c.VkCommandPool) !c.VkCommandBuffer {
    var commandBuffer: c.VkCommandBuffer = undefined;
    var allocInfo = c.VkCommandBufferAllocateInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .commandBufferCount = 1,
        .level = c.VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };
    if(c.vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != c.VK_SUCCESS) {
        return error.failedToAllocateSingleTimeCommandBuffer;
    }
    var beginInfo = c.VkCommandBufferBeginInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = c.VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    if(c.vkBeginCommandBuffer(commandBuffer, &beginInfo) != c.VK_SUCCESS) {
        return error.failedToBeginSingleTimeCommandBuffer;
    }
    return commandBuffer;
}

pub fn Vec(comptime n: usize) type {
    return struct {
        arr: [n]f32,
        
        pub const zero = @This(){ .arr = [1]f32{0.0} ** n};
        pub const len: usize = n;
        pub const VectorError = error{
            outOfBounds
        };

        pub fn init(values: [n]f32) @This() {
            var result: @This() = undefined;
            @memcpy(result.arr[0..], values[0..]);
            return result;
        }

        pub fn add(v1: @This(), v2: @This()) @This() {
            var result = v1;
            for(0..n) |i| result.arr[i] += v2.arr[i];
            return result;
        }

        pub fn addInto(v1: *@This(), v2: @This()) *@This() {
            for(0..n) |i| v1.arr[i] += v2.arr[i];
            return v1;
        }

        pub fn subtract(v1: @This(), v2: @This()) @This() {
            var result = v1;
            for(0..n) |i| result.arr[i] -= v2.arr[i];
            return result;
        }

        pub fn scale(v: @This(), factor: f32) @This() {
            var result = v;
            for(0..n) |i| result.arr[i] *= factor;
            return result;
        }

        pub fn magnitude(v: @This()) f32 {
            var result: f32 = 0;
            for(0..n) |i| {
                result += v.arr[i] * v.arr[i];
            }
            return std.math.sqrt(result);
        }

        pub fn normalize(v: @This()) @This() {
            return v.scale(1.0 / v.magnitude());
        }

        pub fn at(Self: @This(), i: usize) VectorError!f32 {
            if(i >= n) return VectorError.outOfBounds;
            return Self.arr[i];
        }

        pub fn equals(Self: @This(), other: @This()) bool {
            for(0..n) |i| if(Self.arr[i] != other.arr[i]) return false;
            return true;
        }

    };
}

pub fn dotProduct(comptime n: usize, v1: Vec(n), v2: Vec(n)) f32 {
    var result: f32 = 0;
    for(0..n) |i| result += v1.arr[i] * v2.arr[i];
    return result;
}

pub fn crossProduct(v1: Vec(3), v2: Vec(3)) Vec(3) {
    return Vec(3).init(.{
        v1.arr[1] * v2.arr[2] - v1.arr[2] * v2.arr[1],
        -v1.arr[0] * v2.arr[2] + v1.arr[2] * v2.arr[0],
        v1.arr[0] * v2.arr[1] - v1.arr[1] * v2.arr[0]
    });
}

pub const Mat4 = Mat(4);
pub fn Mat(comptime r: usize) type {
    return struct {
        arr: [r*r]f32,

        pub const default = @This(){ .arr = [1]f32{0.0} ** (r*r)};
        pub const identity = I:{
            var result = default;
            for(0..r) |i| result.setUnsafe(i, i, 1.0);
            break:I result;
        };

        pub const MatrixError = error{
            RowOutOfBounds,
            ColumnOutOfBounds
        };

        pub fn init(values: [r*r]f32) @This() {
            var result: @This() = undefined;
            @memcpy(result.arr[0..], values[0..]);
            return result;
        }

        pub fn at(Self: *const @This(), row: usize, col: usize) MatrixError!f32 {
            try boundsCheck(row, col);
            return Self.arr[row * r + col];
        }
        
        pub fn atUnsafe(Self: *const @This(), row: usize, col: usize) f32 {
            return Self.arr[row * r + col];
        }

        pub fn set(Self: * @This(), row: usize, col : usize, value: f32) MatrixError!void {
            try boundsCheck(row, col);
            Self.arr[row * r + col] = value;
        }
        
        pub fn setUnsafe(Self: *@This(), row: usize, col: usize, value: f32) void {
            Self.arr[row * r + col] = value;
        }

        pub fn mult(m1: @This(), m2: @This()) @This() {
            var result: @This() = default;
            for(0..r) |row| {
                for(0..r) |col| {
                    var sum: f32 = 0;
                    for(0..r) |i| {
                       sum += m1.atUnsafe(row, i) * m2.atUnsafe(i, col); 
                    }
                    result.setUnsafe(row, col, sum); 
                }
            }
            return result;
        }

        pub fn copy(other: @This()) @This() {
            var result: @This() = undefined;
            @memcpy(&result.arr, &other.arr);
            return result;
        }

        pub fn transform(Self: @This(), vector: Vec(r)) Vec(r) {
            var result = Vec(r).zero;
            for(0..r) |i| {
                for(0..r) |j| result.arr[i] += Self.atUnsafe(i,j) * vector.arr[j];
            }
            return result;

        }

        pub fn transformInto(Self: @This(), vector: *Vec(r)) *Vec(r) {
            const temp = Self.transform(vector.*);
            @memcpy(vector.arr[0..], temp.arr[0..]);
            return vector;
        }

        fn boundsCheck(row: usize, col: usize) MatrixError!void {
            if(row >= r) return MatrixError.RowOutOfBounds;
            if(col >= r) return MatrixError.ColumnOutOfBounds;
        }
    };
}

//Matrix taken from: https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
pub fn rotate(angle: f32, axis: Vec(3)) Mat(4) {
    const u = axis.normalize(); 
    const ux: f32 = u.arr[0];
    const uy: f32 = u.arr[1];
    const uz: f32 = u.arr[2];
    const cos: f32 = @cos(angle);
    const sin: f32 = @sin(angle);
    return Mat(4).init(.{
        ux * ux * (1.0 - cos) + cos, ux * uy * (1.0 - cos) - uz * sin, ux * uz * (1.0 - cos) + uy * sin, 0.0,
        ux * uy * (1.0 - cos) + uz * sin, uy * uy * (1.0 - cos) + cos, uy * uz * (1.0 - cos) - ux * sin, 0.0,
        ux * uz * (1.0 - cos) - uy * sin, uy * uz * (1.0 - cos) + ux * sin, uz * uz * (1.0 - cos) + cos, 0.0,
        0.0, 0.0, 0.0, 1.0
    });
}

pub fn rotate2(cos: f32, sin: f32, axis: Vec(3)) Mat(4) {
    const u = axis.normalize();
    const ux: f32 = u.arr[0];
    const uy: f32 = u.arr[1];
    const uz: f32 = u.arr[2];
    return Mat(4).init(.{
        ux * ux * (1.0 - cos) + cos, ux * uy * (1.0 - cos) - uz * sin, ux * uz * (1.0 - cos) + uy * sin, 0.0,
        ux * uy * (1.0 - cos) + uz * sin, uy * uy * (1.0 - cos) + cos, uy * uz * (1.0 - cos) - ux * sin, 0.0,
        ux * uz * (1.0 - cos) - uy * sin, uy * uz * (1.0 - cos) + ux * sin, uz * uz * (1.0 - cos) + cos, 0.0,
        0.0, 0.0, 0.0, 1.0
    });
}

pub fn translate(shift: Vec(3)) Mat(4) {
    return Mat(4).init(.{
        1, 0, 0, shift.arr[0],
        0, 1, 0, shift.arr[1],
        0, 0, 1, shift.arr[2],
        0, 0, 0, 1
    });
}

pub fn scaleMatrix(factors: [3]f32) Mat(4) {
    var result = Mat(4).identity;
    result.setUnsafe(0, 0, factors[0]);
    result.setUnsafe(1, 1, factors[1]);
    result.setUnsafe(2, 2, factors[2]);
    return result;
}

pub fn view(direction: Vec(3), cameraLocation: Vec(3), upVector: Vec(3)) Mat(4) {
    const W = direction.normalize(); //Camera space z-axis
    const U = crossProduct(upVector, W).normalize(); //Camera space x-axis
    const V = crossProduct(W, U).normalize(); //Camera space y-axis
    return Mat(4).init(.{
        U.arr[0], U.arr[1], U.arr[2], -dotProduct(3, U, cameraLocation),
        V.arr[0], V.arr[1], V.arr[2], -dotProduct(3, V, cameraLocation),
        W.arr[0], W.arr[1], W.arr[2], -dotProduct(3, W, cameraLocation),
        0.0, 0.0, 0.0, 1.0
    });
}

pub fn lookAt(where: Vec(3), cameraLocation: Vec(3), cameraAngle: f32) Mat(4) {
    return view(where.subtract(cameraLocation), cameraLocation, cameraAngle); 
}

pub fn perspective(near: f32, far: f32, fieldOfView: f32, dimension: f32) Mat(4) {
    const width: f32 = @abs(@tan(fieldOfView / 2.0)) * near * 2.0;
    const height: f32 = dimension * width;
    const per = Mat(4).init(.{
        near, 0.0, 0.0, 0.0,
        0.0, near, 0.0, 0.0,
        0.0, 0.0, near + far, -(near * far),
        0.0, 0.0, 1.0, 0.0
    });
    const ortho = Mat(4).init(.{
        2.0 / width, 0.0, 0.0, 0.0,
        0.0, -2.0 / height, 0.0, 0.0,
        0.0, 0.0, 1.0 / (far - near), -near / (far - near),
        0.0, 0.0, 0.0, 1.0
    });
    return ortho.mult(per);
}

const VertexAttributes: type = enum (u32) {
    COLOR = 0,
    POSITION
};

pub const Vertex = struct {
    color: [3]f32,
    position: [3]f32,

    const numAttribs: usize = @typeInfo(@This()).@"struct".fields.len;

    pub fn create(inColor: [3]f32, inPosition: [3]f32) Vertex {
        return .{
            .position = inPosition,
            .color = inColor
        };
    }

    pub fn inputBindingDescription() c.VkVertexInputBindingDescription {
        return .{
            .binding = 0,
            .stride = @sizeOf(Vertex),
            .inputRate = c.VK_VERTEX_INPUT_RATE_VERTEX
        };
    }

    pub fn inputAttributeDescriptions() [numAttribs]c.VkVertexInputAttributeDescription {
        var attributes: [numAttribs]c.VkVertexInputAttributeDescription = undefined;
        attributes[@intFromEnum(VertexAttributes.COLOR)] = .{
            .location = @intFromEnum(VertexAttributes.COLOR),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        };
        attributes[@intFromEnum(VertexAttributes.POSITION)] = .{
            .location = @intFromEnum(VertexAttributes.POSITION),
            .binding = 0,
            .format = c.VK_FORMAT_R32G32B32_SFLOAT,
            .offset = @offsetOf(Vertex, "position")
        };
        return attributes;
    }
};

pub const Pos = struct {
    x: f32,
    y: f32,
    z: f32,

    pub fn vector(pos: *const Pos) Vec(3) {
        return Vec(3).init(.{pos.x, pos.y, pos.z});
    }
};

pub const Camera = struct {
    pos: Pos = .{.x = 0.0, .y = 0.0, .z = 0.0},
    dir: Vec(3) = .init(.{0.0, 0.0, 1.0}),
    angle: f32 = 0.0,
    fov: f32 = std.math.pi / 2.0,

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

pub const Object = struct {
    pos: Pos,
    scale: Pos,
    orientation: Vec(3),
    angle: f32,
    id: u64,
    mesh: *Mesh,
    uniformBuffer: UniformBuffer,
    descriptorSets: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSet,

    pub const HashContext = struct {
        hashModulus: u32,

        pub fn hash(Self: HashContext, key: u64) u32 {
            return @intCast(key % Self.hashModulus);
        }

        pub fn eql(Self: HashContext, key1: u64, key2: u64, inMap: usize) bool {
            _ = Self; _ = inMap;
            return key1 == key2;
        }
    };

    pub fn getModelMatrix(Self: *const Object) Mat(4){
        const scale = scaleMatrix(.{Self.scale.x, Self.scale.y, Self.scale.z});
        const translation = translate(Self.pos.vector());
        const centerPos = Self.pos.vector().add(Self.scale.vector().scale(0.5));
        const rotation = translate(centerPos).mult(rotate(Self.angle, Self.orientation)).mult(translate(centerPos.scale(-1.0)));
        return rotation.mult(translation).mult(scale);
    }

    pub fn create(state: *State, pos: Pos, scaling: Pos, orientation: Vec(3), angle: f32, mesh: *Mesh, allocator: ?*c.VkAllocationCallbacks) !Object {
        var result: Object = undefined;
        result.pos = pos;
        result.scale = scaling;
        result.orientation = orientation;
        result.angle = angle;
        result.mesh = mesh;
        result.id = state.lastGeneratedId + 1;
        while(state.objects.contains(result.id)) {
            result.id += 1;
        }
        state.lastGeneratedId = result.id;
        result.mesh.objects.appendAssumeCapacity(result.id);
        result.uniformBuffer = try UniformBuffer.create(state, allocator);
        for(result.uniformBuffer.hostMemory) |*modelMatrix| modelMatrix.* = Mat4.identity;

        const layouts: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSetLayout = .{state.descriptorSetLayout} ** MAX_FRAMES_IN_FLIGHT;
        const allocInfo = c.VkDescriptorSetAllocateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = state.descriptorPool,
            .descriptorSetCount = result.descriptorSets.len,
            .pSetLayouts = layouts[0..]
        };
        if(c.vkAllocateDescriptorSets(state.device, &allocInfo, &result.descriptorSets) != c.VK_SUCCESS) {
            return error.failedToAllocateDescriptorSets;
        }

        for(0..result.descriptorSets.len) |i| {
            const writeDescriptorSet = c.VkWriteDescriptorSet{
                .sType = c.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = result.descriptorSets[i],
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &.{
                    .buffer = result.uniformBuffer.handle,
                    .offset = @sizeOf(Mat4) * i,
                    .range = @sizeOf(Mat4)
                }
            };
            c.vkUpdateDescriptorSets(state.device, 1, &writeDescriptorSet, 0, null);
        }
        return result;
    }

    pub fn destroy(Self: *Object, device: c.VkDevice, pool: c.VkDescriptorPool, allocator: ?*c.VkAllocationCallbacks) void {
        Self.uniformBuffer.destroy(device, allocator);
        _ = c.vkFreeDescriptorSets(device, pool, Self.descriptorSets.len, &Self.descriptorSets); 
        for(0..Self.mesh.objects.items.len) |i| {
            if(Self.mesh.objects.items[i] == Self.id) {
                _ = Self.mesh.objects.swapRemove(i);
                break;
            }
        }
    }
};