const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

//  VkDeviceCreateInfo.ppEnabledExtensionNames assumes each name has an alignment of
//      VK_MAX_EXTENSION_NAME_SIZE, so the string literal pointers have to be casted to fit into this 
//      space to prevent a segmentation fault
const deviceExtensions = [_]*const[c.VK_MAX_EXTENSION_NAME_SIZE]u8{ 
    @ptrCast(c.VK_KHR_SWAPCHAIN_EXTENSION_NAME),
    @ptrCast(c.VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
};

pub fn init(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) !void {
    try pickPhysicalDevice(&state.physicalDevice, state.surface, state.instance);
    try createLogicalDevice(&state.device, &state.queues, &state.queueFamilyIndices, state.physicalDevice, state.surface, allocator);
}

pub fn destroy(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) void {
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
        const deviceQueueFamilies = Utils.findQueueFamilyIndices(device, surface) catch continue:findBestDevice; 

        if(!try checkDeviceExtensionSupport(device)) {
            continue:findBestDevice;
        }

        var deviceSurfaceInfo = Utils.SurfaceInfo{};
        try deviceSurfaceInfo.query(device, surface);
        defer deviceSurfaceInfo.free();

        if(deviceSurfaceInfo.capabilities.supportedTransforms 
            & c.VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR == 0
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
        //Prefer to use just one queue family
        if(deviceQueueFamilies.graphicsIndex.? == deviceQueueFamilies.presentIndex.?) currentDeviceScore += 1;
        //Prefer to have more than one graphics queue
        if(deviceQueueFamilies.graphicsQueueCount > 1) currentDeviceScore += 1;

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
        return error.failedToFindSuitableGPU;
    }
}

fn createLogicalDevice(device: *c.VkDevice, queues: *Utils.Queues, queueFamilyIndices: *Utils.QueueFamilyIndices, physical: c.VkPhysicalDevice, 
    surface: c.VkSurfaceKHR, allocator: ?*c.VkAllocationCallbacks
) !void {
    queueFamilyIndices.* = try Utils.findQueueFamilyIndices(physical, surface);
    var queueCreateInfos: [2]c.VkDeviceQueueCreateInfo = undefined;
    var numQueueInfos: u32 = 1;
    const graphicsQueuePriorities = try std.heap.page_allocator.alloc(f32, queueFamilyIndices.graphicsQueueCount);
    defer std.heap.page_allocator.free(graphicsQueuePriorities);
    //Graphics Queue Info
    @memset(graphicsQueuePriorities, 0.0);
    queueCreateInfos[0] = c.VkDeviceQueueCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndices.graphicsIndex.?,
        .queueCount = queueFamilyIndices.graphicsQueueCount,
        .pQueuePriorities = graphicsQueuePriorities.ptr
    };
    //Present Queue Info
    if(queueFamilyIndices.graphicsIndex.? != queueFamilyIndices.presentIndex.?) {
        numQueueInfos += 1;
        queueCreateInfos[1] = c.VkDeviceQueueCreateInfo{
            .sType = c.VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamilyIndices.presentIndex.?,
            .queueCount = 1,
            .pQueuePriorities = &[1]f32{0.0}
        };
    }

    //pNext chain of device features to enable
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
        .queueCreateInfoCount = numQueueInfos,
        .pQueueCreateInfos = &queueCreateInfos,
        .pNext = &dynamicRenderingLocalRead
    };

    if(c.vkCreateDevice(physical, &deviceInfo, allocator, device) != c.VK_SUCCESS) {
        return error.failedToCreateLogicalDevice;
    }

    try createQueues(device.*, queueFamilyIndices.*, queues);
}

fn createQueues(device: c.VkDevice, queueFamilyIndices: Utils.QueueFamilyIndices, queues: *Utils.Queues) !void {
    //  Allocate array of queues
    queues.*.graphicsQueues = try std.heap.page_allocator.alloc(c.VkQueue, queueFamilyIndices.graphicsQueueCount);
    //  Fill the array with handles to all the graphics queues
    for(queues.*.graphicsQueues, 0..) |*queue, i| {
        c.vkGetDeviceQueue(device, queueFamilyIndices.graphicsIndex.?, @intCast(i), queue);
    }
    //  Index to track which queue in the array to access
    var queueIndex: usize = 0;
    //  If the present queue family is the same as the graphics queue family, we assign the present
    //      queue to one in the queue array
    if(queueFamilyIndices.graphicsIndex.? == queueFamilyIndices.presentIndex.?) {
        queues.*.presentQueue = queues.*.graphicsQueues[queueIndex];
        //  Iterating queueIndex like this ensures it never exceeds the amount of queues we
        //      actually created, allowing queues to be reused for multiple purposes if needed
        queueIndex = (queueIndex + 1) % queueFamilyIndices.graphicsQueueCount;
    }
    //  Otherwise we set the present queue to a separate handle
    else {
        c.vkGetDeviceQueue(device, queueFamilyIndices.presentIndex.?, 0, &queues.presentQueue);
    }
    //  Get handle to the transfer queue
    queues.*.transferQueue = queues.*.graphicsQueues[queueIndex];
}