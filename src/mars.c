#include "mars.h"

#include <stdio.h>
#include <string.h>

int const WIDTH = 800;
int const HEIGHT = 600;

char const* deviceExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

MarsError marsCreateVkDevice(VkDevice* device, VkPhysicalDevice* physicalDevice, VkInstance const instance, VkSurfaceKHR const surface) {
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
    VkPhysicalDevice* physicalDevices = SDL_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    if(!physicalDevices) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS) {
	return marsMakeError(MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL, "Failed to enumerate physical devices!");
    }
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    uint32_t queueFamilyPropertyCount = 0;

    for(int i = 0; i < physicalDeviceCount; i++) {
	uint32_t deviceExtensionPropertyCount = 0;
	if(vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &deviceExtensionPropertyCount, NULL) != VK_SUCCESS) {
	    return marsMakeError(MARS_MISC_ERROR, "Failed to enumerate physical device extension properties!");
	}
	VkExtensionProperties* extensionProperties = SDL_malloc(sizeof(VkExtensionProperties) * deviceExtensionPropertyCount);
	if(!extensionProperties) {
	    return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	if(vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &deviceExtensionPropertyCount, extensionProperties) != VK_SUCCESS) {
	    SDL_free(extensionProperties);
	    return marsMakeError(MARS_MISC_ERROR, "Failed to enumerate physical device extension properties!");
	}
	for(int j = 0; j < deviceExtensionPropertyCount; j++) {
	    if(strcmp(extensionProperties[j].extensionName, deviceExtensionNames[0]) == 0) {
		goto Queue_Check;
	    }
	}
	SDL_free(extensionProperties);
	return marsMakeError(MARS_FIND_SUITABLE_GPU_FAIL, "Failed to find a GPU supporting needed extensions!");

    Queue_Check:
	SDL_free(extensionProperties);
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCount, NULL);
	VkQueueFamilyProperties2* queueFamilyProperties = SDL_malloc(sizeof(VkQueueFamilyProperties2) * queueFamilyPropertyCount);
	if(!queueFamilyProperties) {
	    return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties);
	for(int j = 0; j < queueFamilyPropertyCount; j++) {
	    if(queueFamilyProperties[j].queueFamilyProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
		VkBool32 surfaceSupport;
		if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, surface, &surfaceSupport) != VK_SUCCESS) {
		    SDL_free(queueFamilyProperties);
		    return marsMakeError(MARS_MISC_ERROR, "Failed to get physical device surface support!");
		}
		if(surfaceSupport == VK_TRUE) {
		    queueFamilyIndex = j;
		    queueCount = queueFamilyProperties[j].queueFamilyProperties.queueCount;
		    *physicalDevice = physicalDevices[i];
		    SDL_free(queueFamilyProperties);
		    goto Device_Creation;
		}
	    }
	}
    }
    return marsMakeError(MARS_FIND_SUITABLE_GPU_FAIL, "Failed to find a GPU with needed queue families!");

Device_Creation:
    SDL_free(physicalDevices);

    float* queuePriorities = SDL_calloc(queueCount, sizeof(float));
    if(!queuePriorities) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }

    VkDeviceQueueCreateInfo queueInfo = {
	.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
	.pNext = NULL,
	.flags = 0,
	.queueFamilyIndex = queueFamilyIndex,
	.queueCount = queueCount,
	.pQueuePriorities = queuePriorities
    };

    VkDeviceCreateInfo deviceInfo = {
	.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	.pNext = NULL,
	.flags = 0,
	.queueCreateInfoCount = 1,
	.pQueueCreateInfos = &queueInfo,
	.enabledExtensionCount = sizeof(deviceExtensionNames) / sizeof(char*),
	.ppEnabledExtensionNames = deviceExtensionNames,
	.pEnabledFeatures = NULL
    };

    if(vkCreateDevice(*physicalDevice, &deviceInfo, NULL, device) != VK_SUCCESS) {
	return marsMakeError(MARS_DEVICE_CREATION_FAIL, "Failed to create VkDevice!");
    }
    SDL_free(queuePriorities);
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
    printf("Extensions\n");
    for(int i = 0; i < extCount; i++) {
	printf("\t%s\n", extNames[i]);
    }

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
    MARS_TRY(marsCreateVkDevice(&marsRenderer->device, &marsRenderer->physicalDevice, marsRenderer->instance, marsRenderer->surface));
    
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
