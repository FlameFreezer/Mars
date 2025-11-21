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
    printf("Validation Layer: %s", pCallbackData->pMessage);
    return VK_FALSE;
}

MarsError marsInitVkInstance(VkInstance* instance, char* appName) {
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

MarsError marsInitVkDebugUtilsMessenger(VkInstance const instance, VkDebugUtilsMessengerEXT* debugMessenger) {
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
	.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.pNext = NULL,
	.flags = 0,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
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
    MARS_TRY(marsInitVkInstance(&marsRenderer->instance, appName));
    MARS_TRY(marsInitVkDebugUtilsMessenger(marsRenderer->instance, &marsRenderer->debugMessenger));
    
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
    SDL_DestroyWindow(marsGame->renderer.window);
    destroyVkDebugUtilsMessengerEXT(marsGame->renderer.instance, marsGame->renderer.debugMessenger, NULL);
    vkDestroyInstance(marsGame->renderer.instance, NULL);
    SDL_Quit();
}
