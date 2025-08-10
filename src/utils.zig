const std = @import("std");
const c = @import("c");

pub const MAX_FRAMES_IN_FLIGHT: usize = 2;

pub const State = struct {
    name: []const u8,
    windowShouldClose: bool,
    nextEvent: c.SDL_Event,
    window: ?*c.SDL_Window = null,
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
    buffer: Buffer,
    uniformBuffer: Buffer,
    uniformBufferMapped: []UniformBufferObject,
    graphicsPipelineLayout: c.VkPipelineLayout,
    graphicsPipeline: c.VkPipeline,
    fences: [MAX_FRAMES_IN_FLIGHT]c.VkFence,
    semaphores: []c.VkSemaphore,
    currentFrame: usize = 0,
    startTime: *const i64,
    descriptorSetLayout: c.VkDescriptorSetLayout,
    descriptorPool: c.VkDescriptorPool,
    descriptorSets: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSet,
    depthImage: c.VkImage,
    depthImageView: c.VkImageView,
    depthImageMemory: c.VkDeviceMemory,
};

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

pub const UniformBufferObject = struct {
    model: Mat(4) align(16),
    view: Mat(4) align(16),
    perspective: Mat(4) align(16),

    pub const default = UniformBufferObject{
        .model = Mat(4).identity,
        .view = Mat(4).identity,
        .perspective = Mat(4).identity
    };
};

pub fn updateUniformBufferObject(state: State, uniformBufferObject: *UniformBufferObject, startTime: i64) !void {
    const now: i64 = std.time.milliTimestamp();
    const deltaTime: i64 = now - startTime;
    const fDeltaTime: f32 = @floatFromInt(deltaTime);
    //One rotation every 2 seconds
    uniformBufferObject.model = translate(.init(.{-5.0, -5.0, -5.0}));
    _ = uniformBufferObject.model.multInto(rotate(0.0, std.math.pi / 4000.0 * fDeltaTime, 0.0));
    uniformBufferObject.view = lookAt(
        Vec(3).init(.{0.0, 0.0, 0.0}),
        Vec(3).init(.{0.0, 0.0, -10.0}),
        Vec(3).init(.{0.0, 1.0, 0.0})
    );
    uniformBufferObject.perspective = perspective(1.0, 100.0, std.math.degreesToRadians(150.0), 
        @as(f32, @floatFromInt(state.swapchainExtent.height)) 
        / @as(f32, @floatFromInt(state.swapchainExtent.width)));
}

pub const Queues = struct {
    graphics: c.VkQueue,
    compute: c.VkQueue,
    present: c.VkQueue
};

pub const queueFamilyIndices = struct {
    graphicsIndex: ?u32 = null,
    computeIndex: ?u32 = null,
    presentIndex: ?u32 = null,

    pub fn isComplete(Self: *queueFamilyIndices) bool {
        return Self.*.graphicsIndex != null
            and Self.*.computeIndex != null
            and Self.*.presentIndex != null
        ;
    }
};

pub fn findQueueFamilyIndices(device: c.VkPhysicalDevice, surface: c.VkSurfaceKHR) !queueFamilyIndices {
    var queueFamilyPropertiesCount: u32 = 0;
    _ = c.vkGetPhysicalDeviceQueueFamilyProperties2(device, &queueFamilyPropertiesCount, null);
    const queueFamilyProperties = try std.heap.page_allocator.alloc(c.VkQueueFamilyProperties2, queueFamilyPropertiesCount);
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
        var deviceQueueFamilyIndices = queueFamilyIndices{};

        if(properties.queueFlags & c.VK_QUEUE_COMPUTE_BIT != 0) {
            deviceQueueFamilyIndices.computeIndex = @intCast(i);
        }
        if(properties.queueFlags & c.VK_QUEUE_GRAPHICS_BIT != 0) {
            deviceQueueFamilyIndices.graphicsIndex = @intCast(i);
        }
        var presentSupport: c.VkBool32 = c.VK_FALSE;
        if(c.vkGetPhysicalDeviceSurfaceSupportKHR(device, @intCast(i), surface, &presentSupport) != c.VK_SUCCESS) {
            return error.failedToGetPhysicalDeviceSurfaceSupport;
        }
        if(presentSupport == c.VK_TRUE) {
           deviceQueueFamilyIndices.presentIndex = @intCast(i);
        }

        if(deviceQueueFamilyIndices.isComplete()) {
            std.heap.page_allocator.free(queueFamilyProperties);
            return deviceQueueFamilyIndices;
        }
    }

    return error.queueFamiliesUnsupported;
}
pub const surfaceInfo = struct {
    capabilities: c.VkSurfaceCapabilitiesKHR = undefined,
    formats: ?[]c.VkSurfaceFormatKHR = null,
    presentModes: ?[]c.VkPresentModeKHR = null,

    pub fn query(Self: *surfaceInfo, device: c.VkPhysicalDevice, surface: c.VkSurfaceKHR) !void {
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

    pub fn free(self: *surfaceInfo) void {
        if(self.*.formats) |formats| {
            std.heap.page_allocator.free(formats);
        }
        if(self.*.presentModes) |presentModes| {
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

pub fn Vec(comptime n: u32) type {
    return struct {
        arr: [n]f32,
        
        pub const default = @This(){ .arr = [1]f32{0.0} ** n};
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

pub fn Mat(comptime r: u32) type {
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

        pub fn at(self: *const @This(), row: usize, col: usize) MatrixError!f32 {
            try boundsCheck(row, col);
            return self.arr[row * r + col];
        }
        
        pub fn atUnsafe(self: *const @This(), row: usize, col: usize) f32 {
            return self.arr[row * r + col];
        }

        pub fn set(self: * @This(), row: usize, col : usize, value: f32) MatrixError!void {
            try boundsCheck(row, col);
            self.arr[row * r + col] = value;
        }
        
        pub fn setUnsafe(self: *@This(), row: usize, col: usize, value: f32) void {
            self.arr[row * r + col] = value;
        }

        pub fn multInto(m1: *@This(), m2: @This()) *@This() {
            var result: @This() = default;
            for(0..r) |row| {
                for(0..r) |col| {
                    var sum: f32 = 0;
                    for(0..r) |i| {
                       sum += m2.atUnsafe(row, i) * m1.atUnsafe(i, col); 
                    }
                    result.set(row, col, sum) catch unreachable; 
                }
            }
            @memcpy(m1.arr[0..], result.arr[0..]);
            return m1;
        }

        pub fn mult(m1: @This(), m2: @This()) @This() {
            var result = @This().copy(m1);
            _ = result.multInto(m2);
            return result;
        }

        pub fn copy(other: @This()) @This() {
            var result: @This() = undefined;
            @memcpy(&result.arr, &other.arr);
            return result;
        }

        pub fn transform(self: @This(), vector: *Vec(r)) *Vec(r) {
            var result = Vec(3).default;
            for(0..r) |i| {
                for(0..r) |j| result.arr[i] += self.atUnsafe(i,j) * vector.arr[j];
            }
            @memcpy(vector.arr[0..], result.arr[0..]);
            return vector;
        }

        fn boundsCheck(row: usize, col: usize) MatrixError!void {
            if(row >= r) return MatrixError.RowOutOfBounds;
            if(col >= r) return MatrixError.ColumnOutOfBounds;
        }
    };
}

pub fn rotate(tx: f32, ty: f32, tz: f32) Mat(4) {
    const rz = Mat(4){ .arr = [16]f32{
        @cos(tz), -@sin(tz), 0, 0,
        @sin(tz), @cos(tz), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }};
    const ry = Mat(4){ .arr = [16]f32{
        @cos(ty), 0, @sin(ty), 0,
        0, 1, 0, 0,
        -@sin(ty), 0, @cos(ty), 0,
        0, 0, 0, 1
    }};
    var rx = Mat(4){ .arr = [16]f32{
        1, 0, 0, 0,
        0, @cos(tx), -@sin(tx), 0,
        0, @sin(tx), @cos(ty), 0,
        0, 0, 0, 1
    }};

    _ = rx.multInto(ry).multInto(rz);
    return rx;
}

pub fn translate(shift: Vec(3)) Mat(4) {
    return Mat(4).init(.{
        1, 0, 0, shift.arr[0],
        0, 1, 0, shift.arr[1],
        0, 0, 1, shift.arr[2],
        0, 0, 0, 1
    });
}

pub fn lookAt(where: Vec(3), cameraLocation: Vec(3), upVector: Vec(3)) Mat(4) {
    const W = where.subtract(cameraLocation).normalize();
    const U = crossProduct(upVector, W).normalize();
    const V = crossProduct(W, U);
    return Mat(4).init(.{
        U.arr[0], U.arr[1], U.arr[2], dotProduct(3, U, cameraLocation.scale(-1.0)),
        V.arr[0], V.arr[1], V.arr[2], dotProduct(3, V, cameraLocation.scale(-1.0)),
        W.arr[0], W.arr[1], W.arr[2], dotProduct(3, W, cameraLocation.scale(-1.0)),
        0.0, 0.0, 0.0, 1.0
    });
}

pub fn perspective(near: f32, far: f32, fieldOfView: f32, dimension: f32) Mat(4) {
    const width: f32 = @tan(fieldOfView / 2.0) * near * 2.0;
    const height: f32 = dimension * width;
    const per = Mat(4).init(.{
        near, 0, 0, 0,
        0, near, 0, 0,
        0, 0, near + far, -(near * far),
        0, 0, 1, 0
    });
    const ortho = Mat(4).init(.{
        2.0 / width, 0.0, 0.0, 0.0,
        0.0, -2.0 / height, 0.0, 0.0,
        0.0, 0.0, 1.0 / (far - near), -near / (far - near),
        0.0, 0.0, 0.0, 1.0
    });
    return per.mult(ortho);
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

pub const Cube = struct {
    w: f32, //extension along x-axis
    h: f32, //extension along y-axis
    l: f32, //extension along z-axis

    pub const numVertices: u32 = 24;
    pub const numIndices: u32 = 36;

    pub fn getVertices(self: Cube) [numVertices]Vertex {
        const white = [3]f32{1.0, 1.0, 1.0};
        const red = [3]f32{1.0, 0.0, 0.0};
        const green = [3]f32{0.0, 1.0, 0.0};
        const blue = [3]f32{0.0, 0.0, 1.0};
        const yellow = [3]f32{1.0, 1.0, 0.0};
        const purple = [3]f32{1.0, 0.0, 1.0};
        return .{
            //FRONT FACE
            .create(white,.{0.0, self.h, 0.0}),
            .create(white,.{self.w, self.h, 0.0}),
            .create(white,.{self.w, 0.0, 0.0}),
            .create(white,.{0.0, 0.0, 0.0}),
            //BACK FACE
            .create(purple,.{0.0, self.h, self.l}),
            .create(purple,.{self.w, self.h, self.l}),
            .create(purple,.{self.w, 0.0, self.l}),
            .create(purple,.{0.0, 0.0, self.l}),
            //RIGHT FACE
            .create(yellow,.{self.w, self.h, 0.0}),
            .create(yellow,.{self.w, self.h, self.l}),
            .create(yellow,.{self.w, 0.0, self.l}),
            .create(yellow,.{self.w, 0.0, 0.0}),
            //LEFT FACE
            .create(green,.{0.0, self.h, 0.0}),
            .create(green,.{0.0, self.h, self.l}),
            .create(green,.{0.0, 0.0, self.l}),
            .create(green,.{0.0, 0.0, 0.0}),
            //TOP FACE
            .create(red,.{0.0, self.h, self.l}),
            .create(red,.{self.w, self.h, self.l}),
            .create(red,.{self.w, self.h, 0.0}),
            .create(red,.{0.0, self.h, 0.0}),
            //BOTTOM FACE
            .create(blue,.{0.0, 0.0, self.l}),
            .create(blue,.{self.w, 0.0, self.l}),
            .create(blue,.{self.w, 0.0, 0.0}),
            .create(blue,.{0.0, 0.0, 0.0}),
        };
    }

    pub fn getIndices(self: ?Cube) [numIndices]u32 {
        //self argument purely for more natural calling of this function. It is not needed
        _ = self;
        return .{
            //FRONT FACE
            0, 1, 2,
            0, 2, 3,
            //BACK FACE
            4, 5, 6,
            4, 6, 7,
            //RIGHT FACE
            8, 9, 10,
            8, 10, 11,
            //LEFT FACE
            12, 13, 14, 
            12, 14, 15,
            //TOP FACE
            16, 17, 18,
            16, 18, 19,
            //BOTTOM FACE
            20, 21, 22,
            20, 22, 23
        };
    }
};

