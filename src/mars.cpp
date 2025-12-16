module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

#include "mars_macros.h"

module mars;

namespace mars {
    Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::FATAL_ERROR, SDL_GetError()};
        }
        return success();
    }

    void quit() noexcept {
        SDL_Quit();
    }

    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game") {}
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name) {}
    Game::~Game() noexcept {}
    Error<noreturn> Game::init() noexcept {
        TRY(renderer.init(appName));
        return success();
    }
    Error<noreturn> Game::draw() noexcept {
        TRY(renderer.drawFrame());
        return success();
    }
}