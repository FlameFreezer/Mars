module;

#include <SDL3/SDL.h>

#include "mars_macros.h"

module mars;

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
