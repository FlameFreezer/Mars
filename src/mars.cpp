module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>
#include <cstdint>
#include <print>
#include <chrono>

#include "mars_macros.h"

module mars;

namespace mars {
    bool deinitializedLibrary = false;
    Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::FATAL_ERROR, SDL_GetError()};
        }
        return success();
    }

    void quit() noexcept {
        SDL_Quit();
        deinitializedLibrary = true;
    }

    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game"), prevTime(std::chrono::high_resolution_clock::now()) {}
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name), prevTime(std::chrono::high_resolution_clock::now()) {}
    Error<noreturn> Game::init() noexcept {
        TRY(renderer.init(appName));
        return success();
    }
    Error<noreturn> Game::draw() noexcept {
        auto const deltaTime = std::chrono::duration<std::int64_t, std::chrono::nanoseconds::period>(std::chrono::high_resolution_clock::now() - prevTime);
        prevTime = std::chrono::high_resolution_clock::now();
        TRY(renderer.drawFrame(deltaTime));
        return success();
    }
    void Game::destroy() noexcept {
        if(deinitializedLibrary) {
            std::println("Error: the Mars library was deinitialized before destroying this object: mars::Game. Make sure to call \"mars::quit()\" only after ALL objects have been destroyed!");
            return;
        }
        renderer.destroy();
    }
    void Game::setFlag(std::uint64_t flag) noexcept {
        renderer.flags |= flag;
    }
}
