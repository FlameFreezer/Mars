#ifndef MARS_H
#include <mars.hpp>
#endif

#include "renderer/renderer.cpp"

MarsError marsMakeError(enum MarsErrorType key, char const* message) {
    MarsError result;
    result.key = key;
    result.message = message;
    return result;
}

MarsError marsInit(MarsGame& marsGame, char const* name) {
    if(!SDL_Init(SDL_INIT_VIDEO)) {
        return marsMakeError(MARS_INIT_SDL_FAIL, SDL_GetError());
    }
    if(name == nullptr) {
        marsGame.name = "My Mars App";
    }
    else {
        marsGame.name = name;
    }
    MARS_TRY(marsInitRenderer(marsGame.renderer, name));
    return MARS_SUCCESS;
}

void marsQuit(MarsGame& marsGame) {
    vkDestroySwapchainKHR(marsGame.renderer.device, marsGame.renderer.swapchain, nullptr);
    SDL_Vulkan_DestroySurface(marsGame.renderer.instance, marsGame.renderer.surface, nullptr);
    SDL_DestroyWindow(marsGame.renderer.window);
    vkDestroyDevice(marsGame.renderer.device, nullptr);
    destroyVkDebugUtilsMessengerEXT(marsGame.renderer.instance, marsGame.renderer.debugMessenger, nullptr);
    vkDestroyInstance(marsGame.renderer.instance, nullptr);
    SDL_Quit();
}
