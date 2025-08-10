const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const debugMessenger = @import("debugMessenger.zig");

const validationLayers = [_][]const u8{ "VK_LAYER_KHRONOS_validation" };
var enableValidationLayers: bool = false;

pub fn init(state: *Utils.State, debug: bool) !void {
    enableValidationLayers = debug;
    if(enableValidationLayers and checkValidationLayerSupport()) {
        return error.validationLayersRequestedButNotSupported;
    }
    const appInfo: c.VkApplicationInfo = .{
        .sType = c.VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "My Mars App",
        .applicationVersion = c.VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Mars",
        .engineVersion = c.VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = c.VK_MAKE_API_VERSION(0, 1, 4, 313),
    };
    
    var extensions = std.ArrayList([*:0]const u8).init(std.heap.page_allocator);
    try getRequiredExtensions(&extensions);
    defer extensions.deinit();

    var instanceInfo: c.VkInstanceCreateInfo = .{
        .sType = c.VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = null,
        .pNext = null,
        .enabledExtensionCount = @intCast(extensions.items.len),
        .ppEnabledExtensionNames = extensions.items.ptr
    };

    var debugUtilsMessengerInfo: c.VkDebugUtilsMessengerCreateInfoEXT = undefined;
    if(enableValidationLayers) {
        instanceInfo.enabledLayerCount = validationLayers.len;
        instanceInfo.ppEnabledLayerNames = @ptrCast(&validationLayers[0]);
        debugUtilsMessengerInfo = debugMessenger.createInfo();
        instanceInfo.pNext = &debugUtilsMessengerInfo;
    }

    if(c.vkCreateInstance(&instanceInfo, null, &state.instance) != c.VK_SUCCESS) {
        return error.failedToCreateInstance;
    }
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroyInstance(state.instance, allocator);
}

fn checkValidationLayerSupport() bool {
    var layerCount: u32 = 0;
    const availableLayers: ?[*]c.VkLayerProperties = null;
    _ = c.vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    if(availableLayers == null) {
        return false;
    }

    for(validationLayers) |layerName| {
        var layerFound: bool = false;
        
        for(0..layerCount) |i| {
            if(layerName == availableLayers.?[i].layerName) {
                layerFound = true;
                break;
            }
        }
        if(!layerFound) {
            return false;
        }
    }
    return true;
}

fn getRequiredExtensions(extensions: *std.ArrayList([*:0]const u8)) !void {
    var extensionCount: u32 = 0;
    var extensionNames: ?[*]const?[*:0]const u8 = null;
    extensionNames = c.SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    try extensions.*.resize(extensionCount);
    if(extensionNames) |names| {
        for(0..extensionCount) |i| {
            if(names[i]) |name| {
                extensions.*.items[i] = name;
            }
        }
    }
    try extensions.*.append("VK_KHR_get_physical_device_properties2");
    if(enableValidationLayers) {
        try extensions.*.append(c.VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        std.debug.print("Enabled Instance Extensions:\n", .{});
        for(extensions.*.items) |extensionName| {
            std.debug.print("\t{s}\n", .{extensionName});
        }
    }
}
