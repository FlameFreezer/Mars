#ifndef MARS_H
#include <mars.h>
#endif

#include "linkedList.c"

#include <stdio.h>
#include <string.h>

static int const WIDTH = 800;
static int const HEIGHT = 600;
static char const* deviceExtensionNames[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static int const deviceExtensionNameCount = sizeof(deviceExtensionNames) / sizeof(char const*);

static PFN_vkCreateDebugUtilsMessengerEXT createVkDebugUtilsMessengerEXT = nullptr;
static PFN_vkDestroyDebugUtilsMessengerEXT destroyVkDebugUtilsMessengerEXT = nullptr;

static VkBool32 debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT	messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const*	pCallbackData,
	void* pUserData
) {
    printf("Validation Layer: %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

typedef struct {
    VkSurfaceCapabilitiesKHR capabilities;
    VkPresentModeKHR presentMode;
	VkSurfaceFormatKHR format;
    VkSurfaceKHR surface;
} MarsSurfaceInfo;

static MarsError marsPickQueueFamilyIndex(
    uint32_t* queueFamilyIndex, 
    uint32_t* queueCount, 
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR surface
) {
	//Get queue family properties for the current physical device
	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, nullptr);
	VkQueueFamilyProperties2* queueFamilyProperties = SDL_calloc(queueFamilyPropertyCount, sizeof(VkQueueFamilyProperties2));
	if(!queueFamilyProperties) {
		return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties);
	//Check each queue family index for needed support
	for(int i = 0; i < queueFamilyPropertyCount; i++) {
		if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
			VkBool32 surfaceSupport;
			if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport) != VK_SUCCESS) {
				SDL_free(queueFamilyProperties);
				return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to get physical device surface support!");
			}
			if(surfaceSupport == VK_TRUE) {
				//At this point, we have found our desired queue family index
				*queueFamilyIndex = i;
				*queueCount = queueFamilyProperties[i].queueFamilyProperties.queueCount;
				SDL_free(queueFamilyProperties);
				return MARS_SUCCESS;
			}
		}
	}
	SDL_free(queueFamilyProperties);
	return marsMakeError(MARS_SEARCH_FAIL, "");
}

static MarsError marsChoosePresentMode(
    VkPresentModeKHR* presentMode, 
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR surface
) {
	//Get present modes for the surface supported by the physical device
	VkPresentModeKHR* presentModes = nullptr;
	uint32_t presentModeCount = 0;
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(
		physicalDevice, 
		surface, 
		&presentModeCount, 
		nullptr
	) != VK_SUCCESS) {
		return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to get physical device surface present modes!");
	}
	presentModes = SDL_malloc(sizeof(VkPresentModeKHR) * presentModeCount);
	if(!presentModes) {
		return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	if(vkGetPhysicalDeviceSurfacePresentModesKHR(
		physicalDevice, 
		surface, 
		&presentModeCount, 
		presentModes
	) != VK_SUCCESS) {
		return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to get physical device surface present modes!");
	}
	//Select correct present mode
	for(int i = 0; i < presentModeCount; i++) {
		//This is our preferred presentation mode
		if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			*presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		//We are willing to use this present mode if we have to, but we'll continue the search
		else if(presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
			*presentMode = VK_PRESENT_MODE_FIFO_KHR;
		}
	}
	SDL_free(presentModes);
	//If we got here but didn't write to surfaceInfo.presentMode, then the current device did not
	// support any of the desired present modes
	if(!(*presentMode)) {
		return marsMakeError(MARS_SEARCH_FAIL, "");
	}
	return MARS_SUCCESS;
}

static MarsError marsCheckDeviceSurfaceFormats(
    VkSurfaceFormatKHR* surfaceFormat, 
    VkPhysicalDevice physicalDevice, 
    VkSurfaceKHR surface
) {
	//Get physical device's surface formats
	uint32_t surfaceFormatCount = 0;
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr) != VK_SUCCESS) {
		return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to get physical device surface formats!");
	}
	VkSurfaceFormatKHR* surfaceFormats = SDL_malloc(sizeof(VkSurfaceFormatKHR) * surfaceFormatCount);
	if(!surfaceFormats) {
		return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats) != VK_SUCCESS) {
		SDL_free(surfaceFormats);
		return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to get physical device surface formats!");
	}
	for(int j = 0; j < surfaceFormatCount; j++) {
		if(surfaceFormats[j].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			*surfaceFormat = surfaceFormats[j];
			SDL_free(surfaceFormats);
			return MARS_SUCCESS;
		}
	}
	*surfaceFormat = surfaceFormats[0];
	SDL_free(surfaceFormats);
	return MARS_SUCCESS;
}

static MarsError marsCheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
	//Check that the device supports the extensions needed by the application
	uint32_t deviceExtensionPropertyCount = 0;
	if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, nullptr) != VK_SUCCESS) {
		return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to enumerate physical device extension properties!");
	}
	VkExtensionProperties* extensionProperties = SDL_malloc(sizeof(VkExtensionProperties) * deviceExtensionPropertyCount);
	if(!extensionProperties) {
	    return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
	}
	if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, extensionProperties) != VK_SUCCESS) {
	    SDL_free(extensionProperties);
	    return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to enumerate physical device extension properties!");
	}
	//Construct linked list of extension names
	MarsLinkedList deviceExtensions = {};
	for(int j = 0; j < deviceExtensionNameCount; j++) {
	   MARS_TRY(marsPush(&deviceExtensions, deviceExtensionNames[j])); 
	}
	//Check if we have every needed extension
	for(int j = 0; j < deviceExtensionPropertyCount; j++) {
	    //Remove the current extension name from the list, if we have it
	    marsRemove(&deviceExtensions, extensionProperties[j].extensionName);
	    //Once we've "ticked off" each name from the list, we know the device supports all the needed
	    //	extensions
	    if(deviceExtensions.head == nullptr) {
			marsClear(&deviceExtensions);
			SDL_free(extensionProperties);
			return MARS_SUCCESS;
	    }
	}
	SDL_free(extensionProperties);
	return marsMakeError(MARS_SEARCH_FAIL, "");
}

static MarsError marsCreateVkDeviceAndSwapchain(
	VkDevice* device, 
	VkPhysicalDevice* physicalDevice, 
	VkSwapchainKHR* swapchain, 
	VkInstance const instance, 
	VkSurfaceKHR const surface
) {
    uint32_t queueFamilyIndex = 0;
    uint32_t queueCount = 0;
    MarsSurfaceInfo surfaceInfo = {};
    surfaceInfo.surface = surface;
    float* queuePriorities = nullptr;
	VkPhysicalDevice* physicalDevices = nullptr;

    //Get all the physical devices installed on the system
    uint32_t physicalDeviceCount = 0;
    if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS) {
		return marsMakeError(MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL, "Failed to enumerate physical devices!");
    }
    physicalDevices = SDL_malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    if(!physicalDevices) {
		return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
    if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices) != VK_SUCCESS) {
		SDL_free(physicalDevices);
		return marsMakeError(MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL, "Failed to enumerate physical devices!");
    }

    //Iterate through each of these devices
    for(int i = 0; i < physicalDeviceCount; i++) {
		//Check device extension support for the current physical device
		MarsError result = marsCheckDeviceExtensionSupport(physicalDevices[i]);
		if(result.key == MARS_SEARCH_FAIL) {
			continue;
		}
		if(result.key != MARS_ALL_OKAY) {
			SDL_free(physicalDevices);
			return result;
		}

    	//Get physical device's capabilities with the surface
    	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    	    physicalDevices[i], 
    	    surface, 
    	    &surfaceInfo.capabilities
    	) != VK_SUCCESS) {
			SDL_free(physicalDevices);
    	    return marsMakeError(MARS_VULKAN_QUERY_ERROR, "Failed to get physical device surface capabilities!");
    	}

		//Get a surface format to use
		result = marsCheckDeviceSurfaceFormats(&surfaceInfo.format, physicalDevices[i], surface);
		if(result.key != MARS_ALL_OKAY) {
			SDL_free(physicalDevices);
			return result;
		}
		//Choose a present mode to use
		result = marsChoosePresentMode(&surfaceInfo.presentMode, physicalDevices[i], surface);
		if(result.key == MARS_SEARCH_FAIL) {
			continue;
		}
		else if(result.key != MARS_ALL_OKAY) {
			SDL_free(physicalDevices);
			return result;
		}
		//Pick the desired queue family index
		result = marsPickQueueFamilyIndex(&queueFamilyIndex, &queueCount, physicalDevices[i], surface);
		//At this point if all has succeeded, we are ready to use the current physical device and
		// create the logical device
		if(result.key == MARS_ALL_OKAY) {
			*physicalDevice = physicalDevices[i];
			SDL_free(physicalDevices);
			goto Device_Creation;
		}
		//Otherwise, if something went wrong, return the error
		else if(result.key != MARS_SEARCH_FAIL) {
			SDL_free(physicalDevices);
			return result;
		}
    } 
    //If we got here, none of the physical devices supported the features we needed
    return marsMakeError(MARS_FIND_SUITABLE_GPU_FAIL, "Failed to find a suitable GPU!");

Device_Creation:
    queuePriorities = SDL_calloc(queueCount, sizeof(float));
    if(!queuePriorities) {
		return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }

    VkDeviceQueueCreateInfo queueInfo = {
    	.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    	.pNext = nullptr,
    	.flags = 0,
    	.queueFamilyIndex = queueFamilyIndex,
    	.queueCount = queueCount,
    	.pQueuePriorities = queuePriorities
    };

    VkDeviceCreateInfo deviceInfo = {
    	.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    	.pNext = nullptr,
    	.flags = 0,
    	.queueCreateInfoCount = 1,
    	.pQueueCreateInfos = &queueInfo,
    	.enabledExtensionCount = deviceExtensionNameCount,
    	.ppEnabledExtensionNames = deviceExtensionNames,
    	.pEnabledFeatures = nullptr
    };

    if(vkCreateDevice(*physicalDevice, &deviceInfo, nullptr, device) != VK_SUCCESS) {
		return marsMakeError(MARS_DEVICE_CREATION_FAIL, "Failed to create VkDevice!");
    }
    SDL_free(queuePriorities);

	VkSwapchainCreateInfoKHR swapchainInfo = {
    	.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    	.pNext = nullptr,
    	.flags = 0,
    	.surface = surfaceInfo.surface,
    	.minImageCount = surfaceInfo.capabilities.minImageCount + 1,
    	.imageFormat = surfaceInfo.format.format,
    	.imageColorSpace = surfaceInfo.format.colorSpace,
    	.imageExtent = surfaceInfo.capabilities.currentExtent,
    	.imageArrayLayers = 1,
    	.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    	.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    	.queueFamilyIndexCount = 1,
    	.pQueueFamilyIndices = nullptr,
    	.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    	.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    	.presentMode = surfaceInfo.presentMode,
		.clipped = VK_FALSE,
		.oldSwapchain = nullptr
    };

    if(vkCreateSwapchainKHR(*device, &swapchainInfo, nullptr, swapchain) != VK_SUCCESS) {
		return marsMakeError(MARS_SWAPCHAIN_CREATION_FAIL, "Failed to create VkSwapchainKHR!");
    }
    return MARS_SUCCESS;
}

static MarsError marsCreateVkDebugUtilsMessenger(VkDebugUtilsMessengerEXT* debugMessenger, VkInstance const instance) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
    	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    	.pNext = nullptr,
    	.flags = 0,
    	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    	.messageType = /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |*/
    	    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    	    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    	.pfnUserCallback = debugCallback,
		.pUserData = nullptr,
    };
    createVkDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(createVkDebugUtilsMessengerEXT != nullptr) {
		if(createVkDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, debugMessenger) != VK_SUCCESS) {
			return marsMakeError(MARS_DEBUG_MESSENGER_CREATION_FAIL, "Failed to create vkDebugUtilsMessenger!");
		}
    }
    destroyVkDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
	instance, "vkDestroyDebugUtilsMessengerEXT");
    return MARS_SUCCESS;
}

static MarsError marsCreateVkInstance(VkInstance* instance, char const* appName) {
    uint32_t extCount = 0;
    char const * const * sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
    char const** extNames = SDL_malloc(sizeof(char const*) * (extCount + 1));
    if(!extNames) {
		return marsMakeError(MARS_MEMORY_ALLOC_FAIL, "Failed to allocate host memory!");
    }
	extNames[0] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	memcpy(&extNames[1], sdlExtNames, extCount++ * sizeof(char const*));
	printf("Instance Extensions\n");
	for(int i = 0; i < extCount; i++) {
		printf("\t%s\n", extNames[i]);
	}

    VkApplicationInfo appInfo = {
    	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    	.pNext = nullptr,
    	.pApplicationName = appName,
    	.applicationVersion = 1,
    	.pEngineName = nullptr,
    	.engineVersion = 0,
    	.apiVersion = VK_MAKE_API_VERSION(1, 4, 313, 0)
    };
    VkInstanceCreateInfo instanceInfo = {
    	.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    	.pNext = nullptr,
    	.flags = 0,
    	.pApplicationInfo = &appInfo,
    	.enabledLayerCount = 0,
    	.ppEnabledLayerNames = nullptr,
    	.enabledExtensionCount = extCount,
    	.ppEnabledExtensionNames = extNames
    };

    if(vkCreateInstance(&instanceInfo, nullptr, instance) != VK_SUCCESS) {
		return marsMakeError(MARS_INSTANCE_CREATION_FAIL, "Failed to create VkInstance!");
	}
    SDL_free(extNames);
    return MARS_SUCCESS;
}

static MarsError marsInitRenderer(MarsRenderer* marsRenderer, char const* appName) {
	marsRenderer->window = SDL_CreateWindow(appName, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    if(marsRenderer->window == nullptr) {
		return marsMakeError(MARS_WINDOW_CREATION_FAIL, SDL_GetError());
    }
    MARS_TRY(marsCreateVkInstance(&marsRenderer->instance, appName));
    MARS_TRY(marsCreateVkDebugUtilsMessenger(&marsRenderer->debugMessenger, marsRenderer->instance));
    if(!SDL_Vulkan_CreateSurface(marsRenderer->window, marsRenderer->instance, nullptr, &marsRenderer->surface)) {
		return marsMakeError(MARS_SURFACE_CREATION_FAIL, SDL_GetError());
    }
    MARS_TRY(marsCreateVkDeviceAndSwapchain(
		&marsRenderer->device, 
		&marsRenderer->physicalDevice, 
		&marsRenderer->swapchain, 
		marsRenderer->instance, 
		marsRenderer->surface
    ));
    
    return MARS_SUCCESS;
}