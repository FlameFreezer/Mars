#include "mars.h"

#include <SDL3/SDL.h>

namespace mars {

    Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            FATAL(SDL_GetError());
        }
        return SUCCESS;
    }

    void quit() noexcept {
        SDL_Quit();
    }
}
