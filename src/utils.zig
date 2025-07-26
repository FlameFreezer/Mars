const std = @import("std");
const c = @import("c");

pub const MAX_FRAMES_IN_FLIGHT: usize = 2;

pub const Buffer = struct {
    handle: c.VkBuffer,
    memory: c.VkDeviceMemory,

    pub fn Destroy(Self: *Buffer, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) void {
        c.vkDestroyBuffer(device, Self.*.handle, allocator);
        c.vkFreeMemory(device, Self.*.memory, allocator);
    }
};

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
    graphicsPipelineLayout: c.VkPipelineLayout,
    graphicsPipeline: c.VkPipeline,
    fences: [MAX_FRAMES_IN_FLIGHT]c.VkFence,
    semaphores: []c.VkSemaphore,
    currentFrame: usize = 0
};

pub const queueFamilyIndices = struct {
    graphicsIndex: ?usize = null,
    computeIndex: ?usize = null,
    presentIndex: ?usize = null,

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

        if(properties.queueFlags & c.VK_QUEUE_GRAPHICS_BIT == c.VK_QUEUE_GRAPHICS_BIT) {
            deviceQueueFamilyIndices.graphicsIndex = i;
        }

        if(properties.queueFlags & c.VK_QUEUE_COMPUTE_BIT == c.VK_QUEUE_COMPUTE_BIT) {
            deviceQueueFamilyIndices.computeIndex = i;
        }

        var presentSupport: c.VkBool32 = c.VK_FALSE;
        _ = c.vkGetPhysicalDeviceSurfaceSupportKHR(device, @intCast(i), surface, &presentSupport);
        if(presentSupport == c.VK_TRUE) {
           deviceQueueFamilyIndices.presentIndex = i;
        }

        if(deviceQueueFamilyIndices.isComplete()) {
            std.heap.page_allocator.free(queueFamilyProperties);
            return deviceQueueFamilyIndices;
        }
    }

    return error.queue_families_unsupported;
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
        return error.failed_to_allocate_single_time_command_buffer;
    }
    var beginInfo = c.VkCommandBufferBeginInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = c.VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    if(c.vkBeginCommandBuffer(commandBuffer, &beginInfo) != c.VK_SUCCESS) {
        return error.failed_to_begin_single_time_command_buffer;
    }
    return commandBuffer;
}
