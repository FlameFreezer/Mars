#include "mars.h"

#include <stdio.h>
#include <stdlib.h>

int const WIDTH = 800;
int const HEIGHT = 600;

void marsInitRenderer(MarsRenderer* marsRenderer, char* appName) {
    Uint32 extCount = 0;
    char const * const * sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
    char const** extNames = malloc(sizeof(char*) * extCount + 1);
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
    free(extNames);
}

int marsInit(MarsGame* marsGame, char* name) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
	printf("Failed to Init SDL3!");
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
    vkDestroyInstance(marsGame->renderer.instance, NULL);
    SDL_Quit();
}
