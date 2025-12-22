module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>
#include <cstdint>
#include <chrono>

#include "mars_macros.h"

module mars;

namespace mars {
    Error<noreturn> initLibrary() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::FATAL_ERROR, SDL_GetError()};
        }
        return success();
    }
    void deinitLibrary() noexcept {
        SDL_Quit();
    }

    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game") {}
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name) {}
    Error<noreturn> Game::init() noexcept {
        TRY(initLibrary());
        TRY(renderer.init(appName));
        return success();
    }
    Game::~Game() noexcept {
        renderer.destroy();
        deinitLibrary();
    }
    void Game::setRendererFlags(RendererFlags flag) noexcept {
        renderer.flags |= flag;
    }
    void Game::setFlags(GameFlags flag) noexcept {
        flags |= flag;
    }
    bool Game::rendererHasFlags(RendererFlags flag) noexcept {
        return renderer.flags & flag;
    }
    bool Game::hasFlags(GameFlags flag) noexcept {
        return flags & flag;
    }
    Error<noreturn> Game::draw() noexcept {
        auto const now = std::chrono::steady_clock::now();
        auto const deltaTime = std::chrono::duration<std::int64_t, std::chrono::nanoseconds::period>(
	    now - prevTime);
        prevTime = now;
        TRY(renderer.drawFrame(deltaTime));
        return success();
    }
}
