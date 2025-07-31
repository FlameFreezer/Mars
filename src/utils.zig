const std = @import("std");
const c = @import("c");

pub const MAX_FRAMES_IN_FLIGHT: usize = 2;

pub const Buffer = struct {
    handle: c.VkBuffer,
    memory: c.VkDeviceMemory,

    pub fn create(physical: c.VkPhysicalDevice, device: c.VkDevice, 
        allocator: ?*c.VkAllocationCallbacks, size: u32, usage: c.VkBufferUsageFlags, 
        properties: c.VkMemoryPropertyFlags) !Buffer 
    {
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

    fn findPhysicalDeviceMemoryTypeIndex(physical: c.VkPhysicalDevice, availableTypes: u32, 
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
};

pub const UBO = struct {
    model: mat(4) align(16),
    view: mat(4) align(16),
    perspective: mat(4) align(16)
};

pub fn updateUBO(state: *State, deltaTime: i64) !void {
    //One rotation every 2 seconds
    var ubo: UBO = undefined;
    ubo.model = rotate(0, 0, 2.0 * std.math.pi / @as(f32, @floatFromInt(2000 * deltaTime)));
    ubo.view = mat(4).identity();
    ubo.perspective = mat(4).identity();

    state.uniformBufferMapped[state.currentFrame] = ubo;
}

pub const Queues = struct {
    graphics: c.VkQueue,
    compute: c.VkQueue,
    present: c.VkQueue
};

pub const State = struct {
    window: ?*c.GLFWwindow = null,
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
    uniformBufferMapped: []UBO,
    graphicsPipelineLayout: c.VkPipelineLayout,
    graphicsPipeline: c.VkPipeline,
    fences: [MAX_FRAMES_IN_FLIGHT]c.VkFence,
    semaphores: []c.VkSemaphore,
    currentFrame: usize = 0,
    startTime: i64,
    descriptorSetLayout: c.VkDescriptorSetLayout,
    descriptorPool: c.VkDescriptorPool,
    descriptorSets: [MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSet
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

pub fn vec(comptime n: u32) type {
    return struct {
        arr: [n]f32,
        
        pub const default = @This(){ .arr = [1]f32{0.0} ** n};

        pub fn init(values: [n]f32) @This() {
            return .{.arr = values};
        }

        pub fn add(v1: *const @This(), v2: *const @This()) @This() {
            var result = default;
            inline for(0..n) |i| {
                result.arr[i] = v1.arr[i] + v2.arr[i];
            }
            return result;
        }

        pub fn addInto(v1: *@This(), v2: *const @This()) *@This() {
            inline for(0..n) |i| {
                v1.arr[i] += v2.arr[i];
            }
            return v1;
        }
    };
}

pub fn mat(comptime r: u32) type {
    return struct {
        arr: [r*r]f32,

        pub const default = @This(){ .arr = [1]f32{0.0} ** (r*r)};

        pub const MatrixError = error{
            RowOutOfBounds,
            ColumnOutOfBounds
        };

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

        pub inline fn identity() @This() {
            comptime var result = default;
            inline for(0..r) |i| {
                comptime result.arr[i * r + i] = 1.0;
            }
            return result;
        }

        pub fn multInto(m1: *@This(), m2: *const @This()) *@This() {
            var result: @This() = @This().default;
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

        pub fn copy(other: *const @This()) @This() {
            var result: @This() = undefined;
            @memcpy(&result.arr, &other.arr);
            return result;
        }

        fn boundsCheck(row: usize, col: usize) MatrixError!void {
            if(row >= r) return MatrixError.RowOutOfBounds;
            if(col >= r) return MatrixError.ColumnOutOfBounds;
        }
    };
}

pub fn rotate(tx: f32, ty: f32, tz: f32) mat(4) {
    const rx = mat(4){ .arr = [16]f32{
        1, 0, 0, 0,
        0, @cos(tx), -@sin(tx), 0,
        0, @sin(tx), @cos(ty), 0,
        0, 0, 0, 1
    }};
    const ry = mat(4){ .arr = [16]f32{
        @cos(ty), 0, @sin(ty), 0,
        0, 1, 0, 0,
        -@sin(ty), 0, @cos(ty), 0,
        0, 0, 0, 1
    }};
    const rz = mat(4){ .arr = [16]f32{
        @cos(tz), -@sin(tz), 0, 0,
        @sin(tz), @cos(tz), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    }};
    var result = mat(4).identity();
    _ = result.multInto(&rx).multInto(&ry).multInto(&rz);
    return result;
}
