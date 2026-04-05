module;

#include <string>
#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "mars_macros.h"

module mars;

constexpr float defaultPixelsPerMeter = 64.0f;

namespace mars {

    Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            return fatal(SDL_GetError());
        }
        return success();
    }

    void quit() noexcept {
        SDL_Quit();
    }
}
