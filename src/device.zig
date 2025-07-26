const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const deviceExtensions = [_][]const u8{ 
    c.VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    if(c.glfwCreateWindowSurface(state.instance, state.window, null, &state.surface) != 0) {
        return error.failed_to_create_window_surface;
    }
    try pickPhysicalDevice(&state.physicalDevice, state.surface, state.instance);
    try createLogicalDevice(&state.device, &state.queues, state.physicalDevice, state.surface, allocator);
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroySurfaceKHR(state.instance, state.surface, allocator);
    _ = c.vkDeviceWaitIdle(state.device);
    c.vkDestroyDevice(state.device, allocator);
}

fn checkDeviceExtensionSupport(device: c.VkPhysicalDevice) !bool {
    var extensionPropertyCount: u32 = 0;
    _ = c.vkEnumerateDeviceExtensionProperties(device, null, &extensionPropertyCount, null);
    const availableExtensionProperties = try std.heap.page_allocator.alloc(c.VkExtensionProperties, extensionPropertyCount);
    defer std.heap.page_allocator.free(availableExtensionProperties);
    _ = c.vkEnumerateDeviceExtensionProperties(device, null, &extensionPropertyCount, availableExtensionProperties.ptr);

    for(deviceExtensions) |name| {
        const foundExtension: bool = for(availableExtensionProperties) |property| {
            if(c.strcmp(&property.extensionName, name.ptr) == 0) {
                break true;
            }
        } else false;
        if(!foundExtension) {
            return false;
        }
    }
    return true;
}

fn pickPhysicalDevice(physical: *c.VkPhysicalDevice, surface: c.VkSurfaceKHR, instance: c.VkInstance) !void {
    var physicalDeviceCount: u32 = 0;
    _ = c.vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, null);
    const physicalDevices = try std.heap.page_allocator.alloc(c.VkPhysicalDevice, physicalDeviceCount);
    defer std.heap.page_allocator.free(physicalDevices);
    _ = c.vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.ptr);

    var bestDevice: ?c.VkPhysicalDevice = null;
    var bestDeviceScore: u32 = 0;
    findBestDevice:for(physicalDevices) |device| {
        const deviceQueueFamilyIndices = Utils.findQueueFamilyIndices(device, surface) catch continue:findBestDevice; 

        if(!try checkDeviceExtensionSupport(device)) {
            continue:findBestDevice;
        }

        var deviceSurfaceInfo = Utils.surfaceInfo{};
        try deviceSurfaceInfo.query(device, surface);
        defer deviceSurfaceInfo.free();

        if(deviceSurfaceInfo.capabilities.supportedTransforms 
            & c.VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR == 0
        ) continue:findBestDevice;
        if(deviceSurfaceInfo.capabilities.supportedCompositeAlpha 
            & c.VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR == 0
        ) continue:findBestDevice;
        if(deviceSurfaceInfo.capabilities.supportedUsageFlags
            & c.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT == 0
        ) continue:findBestDevice;

        const supportsRequiredFormat: bool = for(deviceSurfaceInfo.formats.?) |format| {
            if(format.format == c.VK_FORMAT_B8G8R8A8_SRGB
                and format.colorSpace == c.VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
            ) break true;
        } else false;

        if(!supportsRequiredFormat) {
            continue:findBestDevice;
        }

        if(bestDevice == null) {
            bestDevice = device;
        }

        var currentDeviceScore: u32 = 0;
        if(deviceQueueFamilyIndices.graphicsIndex.? != deviceQueueFamilyIndices.presentIndex.?) {
            currentDeviceScore += 1;
        }

        findBestPresentMode:for(deviceSurfaceInfo.presentModes.?) |presentMode| {
            if(presentMode == c.VK_PRESENT_MODE_MAILBOX_KHR) {
                currentDeviceScore += 2;
                break:findBestPresentMode;
            }
        }

        if(currentDeviceScore > bestDeviceScore) {
            bestDeviceScore = currentDeviceScore;
            bestDevice = device;
        }

    } 
    if(bestDevice) |dev| {
        physical.* = dev;
    }
    else {
        return error.failed_to_find_suitable_GPU;
    }
}

fn createLogicalDevice(device: *c.VkDevice, queues: *Utils.Queues, physical: c.VkPhysicalDevice, 
    surface: c.VkSurfaceKHR, allocator: ?*c.VkAllocationCallbacks
) !void {
    const indices: Utils.queueFamilyIndices = try Utils.findQueueFamilyIndices(physical, surface);
    var queueCreateInfos = std.ArrayList(c.VkDeviceQueueCreateInfo).init(std.heap.page_allocator);
    defer queueCreateInfos.deinit();
    var uniqueQueueFamilyIndices = std.ArrayList(u32).init(std.heap.page_allocator);
    defer uniqueQueueFamilyIndices.deinit();
    const priorities = comptime [1]f32{1.0};

    const queueFamilyIndicesFields = @typeInfo(Utils.queueFamilyIndices).@"struct".fields;
    inline for(0..queueFamilyIndicesFields.len) |i| {
        const indexUnique:bool = for(uniqueQueueFamilyIndices.items) |x| {
            if(x == @as(u32, @intCast(@field(indices, queueFamilyIndicesFields[i].name).?))) break false;
        } else true;

        if(indexUnique) {
            try uniqueQueueFamilyIndices.append(@intCast(@field(indices, queueFamilyIndicesFields[i].name).?));

            const queueCreateInfo = c.VkDeviceQueueCreateInfo{
                .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .flags = 0,
                .pQueuePriorities = &priorities,
                .queueCount = 1,
                .queueFamilyIndex = @intCast(@field(indices, queueFamilyIndicesFields[i].name).?)
            };

            try queueCreateInfos.append(queueCreateInfo);
        }
    }


    var sync2 = c.VkPhysicalDeviceSynchronization2Features{
        .sType = c.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .synchronization2 = c.VK_TRUE
    };
    var dynamicRendering = c.VkPhysicalDeviceDynamicRenderingFeatures{
        .sType = c.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .dynamicRendering = c.VK_TRUE,
        .pNext = &sync2
    };
    var dynamicRenderingLocalRead = c.VkPhysicalDeviceDynamicRenderingLocalReadFeatures{
        .sType = c.VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_LOCAL_READ_FEATURES,
        .dynamicRenderingLocalRead = c.VK_TRUE,
        .pNext = &dynamicRendering
    };
    const deviceInfo: c.VkDeviceCreateInfo = .{
        .sType = c.VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, 
        .enabledExtensionCount = deviceExtensions.len,
        .ppEnabledExtensionNames = @ptrCast(&deviceExtensions),
        .queueCreateInfoCount = @intCast(queueCreateInfos.items.len),
        .pQueueCreateInfos = queueCreateInfos.items.ptr,
        .pNext = &dynamicRenderingLocalRead
    };

    if(c.vkCreateDevice(physical, &deviceInfo, allocator, device) != c.VK_SUCCESS) {
        return error.failed_to_create_logical_device;
    }

    const FIRST_QUEUE_INDEX: u32 = 0;
    c.vkGetDeviceQueue(device.*, @intCast(indices.graphicsIndex.?), FIRST_QUEUE_INDEX, &queues.graphics);
    c.vkGetDeviceQueue(device.*, @intCast(indices.computeIndex.?), FIRST_QUEUE_INDEX, &queues.compute);
    c.vkGetDeviceQueue(device.*, @intCast(indices.presentIndex.?), FIRST_QUEUE_INDEX, &queues.present);
}
