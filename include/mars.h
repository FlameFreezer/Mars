#define MARS_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

// Error Handling
enum MarsErrorType {
    MARS_ALL_OKAY = 0,
    MARS_MISC_ERROR,
    MARS_SEARCH_FAIL,
    MARS_MEMORY_ALLOC_FAIL,
    MARS_VULKAN_QUERY_ERROR,
    MARS_VULKAN_VALIDATION_LAYER,
    MARS_INIT_SDL_FAIL,
    MARS_WINDOW_CREATION_FAIL,
    MARS_INSTANCE_CREATION_FAIL,
    MARS_SURFACE_CREATION_FAIL,
    MARS_DEBUG_MESSENGER_CREATION_FAIL,
    MARS_ENUMERATE_PHYSICAL_DEVICES_FAIL,
    MARS_FIND_SUITABLE_GPU_FAIL,
    MARS_DEVICE_CREATION_FAIL,
    MARS_SWAPCHAIN_CREATION_FAIL,
};

typedef struct {
    enum MarsErrorType key;
    char const* message;
} MarsError;

static MarsError marsGlobalErrorResult;

MarsError marsMakeError(enum MarsErrorType key, char const* message);

#define MARS_SUCCESS marsMakeError(MARS_ALL_OKAY, "")
#define MARS_TRY(proc) marsGlobalErrorResult = proc;\
    if(marsGlobalErrorResult.key != MARS_ALL_OKAY) return marsGlobalErrorResult

typedef struct {
    VkInstance instance;
    SDL_Window* window;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkSwapchainKHR swapchain;
} MarsRenderer;

typedef struct {
    char const* name;
    char const* appName;
    MarsRenderer renderer;
} MarsGame;


MarsError marsInit(MarsGame* marsGame, char const* name); 
void marsQuit(MarsGame* marsGame);
