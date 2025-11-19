#include "mars.h"

#include <stdio.h>

int const WIDTH = 800;
int const HEIGHT = 600;

PFN_vkCreateDebugUtilsMessengerEXT createVkDebugUtilsMessengerEXT = NULL;
PFN_vkDestroyDebugUtilsMessengerEXT destroyVkDebugUtilsMessengerEXT = NULL;

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT 		messageSeverity,
		       VkDebugUtilsMessageTypeFlagsEXT			messageTypes,
		       VkDebugUtilsMessengerCallbackDataEXT const*	pCallbackData,
		       void*						pUserData) {
    printf("VUID: %s, %s\n", pCallbackData->pMessageIdName, pCallbackData->pMessage); 
    return VK_FALSE;
}

void marsInitRenderer(MarsRenderer* marsRenderer, char* appName) {
    Uint32 extCount = 0;
    char const * const * sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
    char const** extNames = SDL_malloc(sizeof(char*) * extCount + 1);
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

    VkResult result = vkCreateInstance(&instanceInfo, NULL, &marsRenderer->instance);
    SDL_free(extNames);
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
    createVkDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(marsRenderer->instance, "vkCreateDebugUtilsMessengerEXT");
    if(createVkDebugUtilsMessengerEXT != NULL) {
	createVkDebugUtilsMessengerEXT(marsRenderer->instance, &debugMessengerInfo, NULL, &marsRenderer->debugMessenger);
    }
    destroyVkDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
	marsRenderer->instance, "vkDestroyDebugUtilsMessengerEXT");
}

int marsInit(MarsGame* marsGame, char* name) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
	return 1;
    }
    if(name == NULL) {
	marsGame->name = "My Mars App";
    }
    else {
	marsGame->name = name;
    }
    marsGame->renderer.window = SDL_CreateWindow(name, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
    marsInitRenderer(&marsGame->renderer, name);
    return 0;
}

void marsQuit(MarsGame* marsGame) {
    SDL_DestroyWindow(marsGame->renderer.window);
    destroyVkDebugUtilsMessengerEXT(marsGame->renderer.instance, marsGame->renderer.debugMessenger, NULL);
    vkDestroyInstance(marsGame->renderer.instance, NULL);
    SDL_Quit();
}
