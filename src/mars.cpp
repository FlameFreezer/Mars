module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <string>
#include <chrono>

#include "mars_macros.h"

module mars;

namespace mars {
    Error<noreturn> initLibrary() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            return {ErrorTag::FATAL_ERROR, SDL_GetError()};
        }
        return success();
    }
    void deinitLibrary() noexcept {
        SDL_Quit();
    }

    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game"), flags(0) {}
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name), flags(0) {}
    Error<noreturn> Game::init() noexcept {
        TRY(initLibrary());
        TRY(renderer.init(appName));
        time = std::chrono::steady_clock::now();
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
    std::chrono::steady_clock::time_point Game::getFrameTime() const noexcept {
        return time;
    }
    std::chrono::nanoseconds Game::getDeltaTime() const noexcept {
        return deltaTime;
    }
    std::chrono::duration<float, std::chrono::seconds::period> Game::getDeltaTimeSeconds() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(deltaTime);
    }
    void Game::updateTime() noexcept {
        auto const now = std::chrono::steady_clock::now();
        deltaTime = now - time;
        time = now;
    }
    void Game::updateKeyState() noexcept {
        keyState = SDL_GetKeyboardState(nullptr);
    }
    Error<noreturn> Game::draw() noexcept {
        if(camera.aspect == Camera::autoAspect) {
            camera.aspect = static_cast<float>(renderer.swapchainImageExtent.width) / static_cast<float>(renderer.swapchainImageExtent.height);
            TRY(camera.loadMatrices(&renderer.cameraMatrices.mappedMemory[renderer.currentFrame * 2]));
            camera.aspect = Camera::autoAspect;
        }
        else {
            TRY(camera.loadMatrices(&renderer.cameraMatrices.mappedMemory[renderer.currentFrame * 2]));
        }
        TRY(renderer.drawFrame(deltaTime));
        return success();
    }
}
