const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    try createSwapchain(state, allocator);
    
    var imageCount: u32 = 0;
    _ = c.vkGetSwapchainImagesKHR(state.device, state.swapchain, &imageCount, null);
    state.swapchainImages = try std.heap.page_allocator.alloc(c.VkImage, imageCount);
    _ = c.vkGetSwapchainImagesKHR(state.device, state.swapchain, &imageCount, state.swapchainImages.ptr);

    try createSwapchainImageViews(&state.swapchainImageViews, state.swapchainImages, state.device, allocator);
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    std.heap.page_allocator.free(state.swapchainImages);
    for(state.swapchainImageViews) |imageView| {
        c.vkDestroyImageView(state.device, imageView, allocator);
    }
    std.heap.page_allocator.free(state.swapchainImageViews);
    c.vkDestroySwapchainKHR(state.device, state.swapchain, allocator);
}

fn createSwapchain(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    var surfaceInfo = Utils.surfaceInfo{};
    try surfaceInfo.query(state.physicalDevice, state.surface);
    defer surfaceInfo.free();

    state.swapchainExtent = chooseImageExtent(state.window, surfaceInfo.capabilities);

    const queueFamilyIndices = try Utils.findQueueFamilyIndices(state.physicalDevice, state.surface);

    //Assume that the graphics and present queue families have different indices
    var imageSharingMode: c.VkSharingMode = c.VK_SHARING_MODE_CONCURRENT;
    var queueFamilyIndicesCount: u32 = 2;
    const queueFamilyIndicesArr = [_]u32{@intCast(queueFamilyIndices.graphicsIndex.?), @intCast(queueFamilyIndices.presentIndex.?)};
    var queueFamilyIndicesArrayPtr: ?*const u32 = &queueFamilyIndicesArr[0];

    //If the graphics and present queue families have the same index, update the parameters
    if(queueFamilyIndices.graphicsIndex.? == queueFamilyIndices.presentIndex.?) {
        imageSharingMode = c.VK_SHARING_MODE_EXCLUSIVE;
        queueFamilyIndicesCount = 0;
        queueFamilyIndicesArrayPtr = null;
    }

    const swapchainInfo = c.VkSwapchainCreateInfoKHR{
        .sType = c.VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .clipped = c.VK_TRUE,
        .imageUsage = c.VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .flags = 0,
        .surface = state.surface,
        .minImageCount = surfaceInfo.capabilities.minImageCount + 1, //prevent waiting on images
        .imageArrayLayers = 1, //not using stereoscopic 3D
        .imageFormat = c.VK_FORMAT_B8G8R8A8_SRGB,
        .imageColorSpace = c.VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = state.swapchainExtent,
        .presentMode = findBestPresentMode(surfaceInfo.presentModes.?),
        .compositeAlpha = c.VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        .preTransform = c.VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .imageSharingMode = imageSharingMode,
        .queueFamilyIndexCount = queueFamilyIndicesCount,
        .pQueueFamilyIndices = queueFamilyIndicesArrayPtr,
    };
    if(c.vkCreateSwapchainKHR(state.device, &swapchainInfo, allocator, &state.swapchain) != c.VK_SUCCESS) {
        return error.failedToCreateSwapchain;
    }
}

fn createSwapchainImageViews(imageViews: *[]c.VkImageView, images: []c.VkImage, device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) !void {
    imageViews.* = try std.heap.page_allocator.alloc(c.VkImageView, images.len);
    var imageViewInfo = c.VkImageViewCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .format = c.VK_FORMAT_B8G8R8A8_SRGB,
        .viewType = c.VK_IMAGE_VIEW_TYPE_2D,
        .components = .{
            .r = c.VK_COMPONENT_SWIZZLE_IDENTITY, 
            .g = c.VK_COMPONENT_SWIZZLE_IDENTITY, 
            .b = c.VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = c.VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = .{
            .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .image = undefined,
    };
    for(images, imageViews.*) |image, *imageView| {
        imageViewInfo.image = image;
        if(c.vkCreateImageView(device, &imageViewInfo, allocator, imageView) != c.VK_SUCCESS) {
            return error.failedToCreateImageView;
        }
    }
}

fn chooseImageExtent(window: ?*c.GLFWwindow, capabilities: c.VkSurfaceCapabilitiesKHR) c.VkExtent2D {
    var resultExtent: c.VkExtent2D = undefined;
    if(capabilities.currentExtent.width != std.math.maxInt(u32)) {
        resultExtent = capabilities.currentExtent;
    }
    else {
        var width: i32 = 0;
        var height: i32 = 0;
        _ = c.glfwGetFramebufferSize(window, &width, &height);
        resultExtent = .{
            .width = std.math.clamp(@as(u32, @intCast(width)), 
                capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            .height = std.math.clamp(@as(u32, @intCast(height)),
                capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
        };
    }
    return resultExtent;
}

fn findBestPresentMode(presentModes: []c.VkPresentModeKHR) c.VkPresentModeKHR {
    for(presentModes) |presentMode| {
        if(presentMode == c.VK_PRESENT_MODE_MAILBOX_KHR) {
            return presentMode;
        }
    }
    return c.VK_PRESENT_MODE_FIFO_KHR;
}
