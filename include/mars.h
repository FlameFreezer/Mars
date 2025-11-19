#ifndef MARS
#define MARS
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

typedef struct {
    VkInstance instance;
    SDL_Window* window;
} MarsRenderer;

typedef struct {
    char* name;
    char* appName;
    MarsRenderer renderer;
} MarsGame;


int marsInit(MarsGame* marsGame, char* name); 
void marsQuit(MarsGame* marsGame);
