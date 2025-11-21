#ifndef MARS
#define MARS
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

enum MarsErrorType {
    MARS_ALL_OKAY = 0,
    MARS_MISC_ERROR,
    MARS_VULKAN_VALIDATION_LAYER,
    MARS_INIT_SDL_FAIL,
    MARS_WINDOW_CREATION_FAIL,
    MARS_INSTANCE_CREATION_FAIL,
    MARS_DEBUG_MESSENGER_CREATION_FAIL,
};

typedef struct {
    enum MarsErrorType key;
    char const* message;
} MarsError;

typedef struct {
    VkInstance instance;
    SDL_Window* window;
    VkDebugUtilsMessengerEXT debugMessenger;
} MarsRenderer;

typedef struct {
    char* name;
    char* appName;
    MarsRenderer renderer;
} MarsGame;

static MarsError marsGlobalErrorResult;

MarsError marsInit(MarsGame* marsGame, char* name); 
void marsQuit(MarsGame* marsGame);
