const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn createInfo() c.VkDebugUtilsMessengerCreateInfoEXT {
    return .{
        .sType = c.VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = c.VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | c.VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | c.VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = c.VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | c.VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | c.VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = null
    };
}

pub fn init(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) void {
    var debugMessengerInfo: c.VkDebugUtilsMessengerCreateInfoEXT = createInfo();
    const createFn: ?*const fn(c.VkInstance, ?*c.VkDebugUtilsMessengerCreateInfoEXT, 
        ?*c.VkAllocationCallbacks, ?*c.VkDebugUtilsMessengerEXT) callconv(.c) c.VkResult 
        = @ptrCast(c.vkGetInstanceProcAddr(state.instance, "vkCreateDebugUtilsMessengerEXT"));
    _ = createFn.?(state.instance, &debugMessengerInfo, allocator, &state.debugMessenger);
}

pub fn destroy(state: *Utils.GPUState, allocator: ?*c.VkAllocationCallbacks) void {
    const destroyFn: ?*const fn(c.VkInstance, c.VkDebugUtilsMessengerEXT, 
        ?*c.VkAllocationCallbacks) callconv(.c) void 
        = @ptrCast(c.vkGetInstanceProcAddr(state.instance, "vkDestroyDebugUtilsMessengerEXT"));
    destroyFn.?(state.instance, state.debugMessenger, allocator);
}

fn debugCallback
(
    messageSeverity: c.VkDebugUtilsMessageSeverityFlagBitsEXT, 
    messageType: c.VkDebugUtilsMessageTypeFlagsEXT,
    pCallbackData: ?[*]const c.VkDebugUtilsMessengerCallbackDataEXT,
    pUserData: ?*anyopaque
) callconv(.c) c.VkBool32 {
    std.debug.print("validation layer: {s}\n", .{pCallbackData.?[0].pMessage});
    _ = messageSeverity;
    _ = messageType;
    _ = pUserData;
    return c.VK_FALSE;
}
