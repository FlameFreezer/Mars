#include "mars.h"

#include <stdio.h>

int const WIDTH = 800;
int const HEIGHT = 600;

MarsError marsMakeError(enum MarsErrorType key, char const* message) {
    MarsError result;
    result.key = key;
    result.message = message;
    return result;
}

PFN_vkCreateDebugUtilsMessengerEXT createVkDebugUtilsMessengerEXT = NULL;
PFN_vkDestroyDebugUtilsMessengerEXT destroyVkDebugUtilsMessengerEXT = NULL;

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT 		messageSeverity,
		       VkDebugUtilsMessageTypeFlagsEXT			messageTypes,
		       VkDebugUtilsMessengerCallbackDataEXT const*	pCallbackData,
		       void*						pUserData) {
    printf("Validation Layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

MarsError marsCreateVkDevice(VkDevice* device, VkPhysicalDevice* physicalDevice, VkInstance const instance) {
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
    VkPhysicalDevice* physicalDevices = SDL_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    if(!physicalDevices) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS) {
	return marsMakeError(MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL, "Failed to enumerate physical devices!");
    }
    //For now, just pick the first phsyical device found
    *physicalDevice = physicalDevices[0];
    SDL_free(physicalDevices);

    VkDeviceCreateInfo deviceInfo = {
	.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	.pNext = NULL,
	.flags = 0,
	.queueCreateInfoCount = 0,
	.pQueueCreateInfos = NULL,
	.enabledExtensionCount = 0,
	.ppEnabledExtensionNames = NULL,
	.pEnabledFeatures = NULL
    };
    if(vkCreateDevice(*physicalDevice, &deviceInfo, NULL, device) != VK_SUCCESS) {
	return marsMakeError(MARS_DEVICE_CREATION_FAIL, "Failed to create VkDevice!");
    }
    return MARS_SUCCESS;
}

MarsError marsCreateVkInstance(VkInstance* instance, char* appName) {
    Uint32 extCount = 0;
    char const * const * sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
    char const** extNames = SDL_malloc(sizeof(char*) * extCount + 1);
    if(!extNames) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    extNames[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    memcpy(&extNames[1], sdlExtNames, extCount++ * sizeof(char*));

    VkApplicationInfo appInfo = {
	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
	.pNext = NULL,
	.pApplicationName = appName,
	.applicationVersion = 1,
	.pEngineName = NULL,
	.engineVersion = 0,
	.apiVersion = VK_MAKE_API_VERSION(1, 4, 313, 0)
    };
    VkInstanceCreateInfo instanceInfo = {
	.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	.pNext = NULL,
	.flags = 0,
	.pApplicationInfo = &appInfo,
	.enabledLayerCount = 0,
	.ppEnabledLayerNames = NULL,
	.enabledExtensionCount = extCount,
	.ppEnabledExtensionNames = extNames
    };

    if(vkCreateInstance(&instanceInfo, NULL, instance) != VK_SUCCESS) {
	return marsMakeError(MARS_INSTANCE_CREATION_FAIL, "Failed to create VkInstance!");
    }
    SDL_free(extNames);
    return MARS_SUCCESS;
}

MarsError marsCreateVkDebugUtilsMessenger(VkDebugUtilsMessengerEXT* debugMessenger, VkInstance const instance) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.pNext = NULL,
	.flags = 0,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType = /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |*/
	    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = debugCallback,
	.pUserData = NULL,
    };
    createVkDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(createVkDebugUtilsMessengerEXT != NULL) {
	if(createVkDebugUtilsMessengerEXT(instance, &debugMessengerInfo, NULL, debugMessenger) != VK_SUCCESS) {
	    return marsMakeError(MARS_DEBUG_MESSENGER_CREATION_FAIL, "Failed to create vkDebugUtilsMessenger!");
	}
    }
    destroyVkDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
	instance, "vkDestroyDebugUtilsMessengerEXT");
    return MARS_SUCCESS;
}

MarsError marsInitRenderer(MarsRenderer* marsRenderer, char* appName) {
    MARS_TRY(marsCreateVkInstance(&marsRenderer->instance, appName));
    MARS_TRY(marsCreateVkDebugUtilsMessenger(&marsRenderer->debugMessenger, marsRenderer->instance));
    if(!SDL_Vulkan_CreateSurface(marsRenderer->window, marsRenderer->instance, NULL, &marsRenderer->surface)) {
	return marsMakeError(MARS_SURFACE_CREATION_FAIL, SDL_GetError());
    }
    MARS_TRY(marsCreateVkDevice(&marsRenderer->device, &marsRenderer->physicalDevice, marsRenderer->instance));
    
    return MARS_SUCCESS;
}

MarsError marsInit(MarsGame* marsGame, char* name) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
	return marsMakeError(MARS_INIT_SDL_FAIL, SDL_GetError());
    }
    if(name == NULL) {
	marsGame->name = "My Mars App";
    }
    else {
	marsGame->name = name;
    }
    marsGame->renderer.window = SDL_CreateWindow(name, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if(marsGame->renderer.window == NULL) {
	return marsMakeError(MARS_WINDOW_CREATION_FAIL, SDL_GetError());
    }
    MarsError result = marsInitRenderer(&marsGame->renderer, name);
    if(result.key != MARS_ALL_OKAY) {
	return result;
    }
    return result;
}

void marsQuit(MarsGame* marsGame) {
    SDL_Vulkan_DestroySurface(marsGame->renderer.instance, marsGame->renderer.surface, NULL);
    SDL_DestroyWindow(marsGame->renderer.window);
    vkDestroyDevice(marsGame->renderer.device, NULL);
    destroyVkDebugUtilsMessengerEXT(marsGame->renderer.instance, marsGame->renderer.debugMessenger, NULL);
    vkDestroyInstance(marsGame->renderer.instance, NULL);
    SDL_Quit();
}
