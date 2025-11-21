#include "mars.h"

#include <stdio.h>
#include <string.h>

int const WIDTH = 800;
int const HEIGHT = 600;
char const* deviceExtensionNames[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
int const deviceExtensionNameCount = 1;

struct MarsNode {
    char const* name;
    struct MarsNode* next;
    struct MarsNode* prev;
};

typedef struct {
    struct MarsNode* head;
    struct MarsNode* tail;
} MarsLinkedList;

MarsError marsPush(MarsLinkedList* list, char const* name) {
    if(!list || !name) return marsMakeError(MARS_MISC_ERROR, "Bad call to function: marsPush");
    struct MarsNode* newNode = SDL_malloc(sizeof(struct MarsNode));
    if(!newNode) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    newNode->name = name;
    newNode->prev = list->tail;
    newNode->next = NULL;
    if(!list->head) {
	list->head = newNode;
	list->tail = newNode;
    }
    else {
	list->tail->next = newNode;
    }
    return MARS_SUCCESS;
}

void marsRemove(MarsLinkedList* list, char const* name) {
    if(!list || !name) return;
    //Find the desired node
    struct MarsNode* iterator = list->head;
    while(iterator && strcmp(iterator->name, name) != 0) {
	iterator = iterator->next;
    }
    if(!iterator) return;
    if(iterator->prev) {
	iterator->prev->next = iterator->next;
    }
    else {
	list->head = iterator->next;
    }
    if(iterator->next) {
	iterator->next->prev = iterator->prev;
    }
    else {
	list->tail = iterator->prev;
    }
    SDL_free(iterator);
}

void marsClear(MarsLinkedList* list) {
    if(!list) return;
    struct MarsNode* iterator = list->head;
    while(iterator) {
	iterator = iterator->next;
	SDL_free(iterator->prev);
    }
    list->head = NULL;
    list->tail = NULL;
}


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

MarsError marsCreateVkSwapchainKHR(VkSwapchainKHR* swapchain, VkPhysicalDevice const physicalDevice, VkDevice const device, VkSurfaceKHR const surface) {
    VkSurfaceCapabilitiesKHR surfaceCapabilities = {0};
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS) {
	return marsMakeError(MARS_MISC_ERROR, "Failed to get physical device surface capabilities!");
    }
    uint32_t presentModeCount = 0;
    if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL) != VK_SUCCESS) {
	return marsMakeError(MARS_MISC_ERROR, "Failed to get physical device surface present modes!");
    }
    VkPresentModeKHR* presentModes = SDL_malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    if(!presentModes) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes) != VK_SUCCESS) {
	return marsMakeError(MARS_MISC_ERROR, "Failed to get physical device surface present modes!");
    }
    VkPresentModeKHR presentMode = 0;
    for(int i = 0; i < presentModeCount; i++) {
	if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
	    presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	    break;
	}
	else if(presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
	    presentMode = VK_PRESENT_MODE_FIFO_KHR;
	}
    }
    if(!presentMode) {
	return marsMakeError(MARS_SWAPCHAIN_CREATION_FAIL, "Failed to find valid presentation support!");
    }

    SDL_free(presentModes);

    VkSwapchainCreateInfoKHR swapchainInfo = {
	.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	.pNext = NULL,
	.flags = 0,
	.surface = surface,
	.minImageCount = surfaceCapabilities.minImageCount + 1,
	.imageFormat = VK_FORMAT_B8G8R8_SRGB,
	.imageColorSpace = 0,
	.imageExtent = surfaceCapabilities.currentExtent,
	.imageArrayLayers = 1,
	.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
	.queueFamilyIndexCount = 1,
	.pQueueFamilyIndices = NULL,
	.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
	.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
	.presentMode = presentMode,
    };
}

MarsError marsCreateVkDevice(VkDevice* device, VkPhysicalDevice* physicalDevice, VkInstance const instance, VkSurfaceKHR const surface) {
    uint32_t physicalDeviceCount = 0;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;

    //Get all the physical devices installed on the system
    if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL) != VK_SUCCESS) {
	return marsMakeError(MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL, "Failed to enumerate physical devices!");
    }
    VkPhysicalDevice* physicalDevices = SDL_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    if(!physicalDevices) {
	return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS) {
	return marsMakeError(MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL, "Failed to enumerate physical devices!");
    }

    //Iterate through each of these devices
    for(int i = 0; i < physicalDeviceCount; i++) {
	//Check that the device supports the extensions needed by the application
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
	//Construct linked list of extension names
	MarsLinkedList deviceExtensions = {0};
	for(int j = 0; j < deviceExtensionNameCount; j++) {
	   MARS_TRY(marsPush(&deviceExtensions, deviceExtensionNames[j])); 
	}
	//Check if we have every needed extension
	for(int j = 0; j < deviceExtensionPropertyCount; j++) {
	    //Remove the current extension name from the list, if we have it
	    marsRemove(&deviceExtensions, extensionProperties[j].extensionName);
	    //Once we've "ticked off" each name from the list, we know the device supports all the needed
	    //	extensions
	    if(deviceExtensions.head == NULL) {
		marsClear(&deviceExtensions);
		goto Queue_Check;
	    }
	}
	//If we got here, the current physical device does not support the needed extensions, so we
	// continue searching
	marsClear(&deviceExtensions);
	SDL_free(extensionProperties);
	continue;

    Queue_Check:
	SDL_free(extensionProperties);

	//Get queue family properties for the current physical device
	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCount, NULL);
	VkQueueFamilyProperties2* queueFamilyProperties = SDL_malloc(sizeof(VkQueueFamilyProperties2) * queueFamilyPropertyCount);
	if(!queueFamilyProperties) {
	    return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties);
	//Check each queue family index for needed support
	for(int j = 0; j < queueFamilyPropertyCount; j++) {
	    if(queueFamilyProperties[j].queueFamilyProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
		VkBool32 surfaceSupport;
		if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, surface, &surfaceSupport) != VK_SUCCESS) {
		    SDL_free(queueFamilyProperties);
		    return marsMakeError(MARS_MISC_ERROR, "Failed to get physical device surface support!");
		}
		if(surfaceSupport == VK_TRUE) {
		    //At this point, we have found our desired physical device and queue family index
		    queueFamilyIndex = j;
		    queueCount = queueFamilyProperties[j].queueFamilyProperties.queueCount;
		    *physicalDevice = physicalDevices[i];
		    SDL_free(queueFamilyProperties);
		    goto Device_Creation;
		}
	    }
	}
    }
    //If we got here, none of the physical devices supported the features we needed
    return marsMakeError(MARS_FIND_SUITABLE_GPU_FAIL, "Failed to find a suitable GPU!");

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
	.enabledExtensionCount = deviceExtensionNameCount,
	.ppEnabledExtensionNames = deviceExtensionNames,
	.pEnabledFeatures = NULL
    };

    if(vkCreateDevice(*physicalDevice, &deviceInfo, NULL, device) != VK_SUCCESS) {
	return marsMakeError(MARS_DEVICE_CREATION_FAIL, "Failed to create VkDevice!");
    }
    SDL_free(queuePriorities);
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

MarsError marsCreateVkInstance(VkInstance* instance, char* appName) {
    uint32_t extCount = 0;
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
