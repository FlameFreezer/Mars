module;

#include <cstring>
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
    std::chrono::steady_clock::time_point Time::frameTime() const noexcept {
        return mTime;
    }
    std::chrono::nanoseconds::rep Time::deltaNanos() const noexcept {
        return mDelta.count();
    }
    float Time::delta() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mDelta).count();
    }
    void Time::update() noexcept {
        const auto now = std::chrono::steady_clock::now();
        mDelta = now - mTime;
        mTime = now;
    }


    Error<noreturn> initLibrary() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
            return {ErrorTag::fatalError, SDL_GetError()};
        }
        return success();
    }

    void deinitLibrary() noexcept {
        SDL_Quit();
    }

    Game::Game() noexcept : 
        mWindowName("My Mars Game"), 
        mAppName("My Mars Game"), 
        mFlags(0), 
        mRenderer(nullptr), 
        pixelsPerMeter(defaultPixelsPerMeter),
        physicsManager(ecs.componentManager) {}

    Game::Game(const std::string& name) noexcept : 
        mWindowName(name), 
        mAppName(name), 
        mFlags(0), 
        mRenderer(nullptr), 
        pixelsPerMeter(defaultPixelsPerMeter),
        physicsManager(ecs.componentManager) {}

    Error<noreturn> Game::init() noexcept {
        TRY(initLibrary());
        mRenderer = new Renderer();
        TRY(mRenderer->init(mAppName, shapes.rectangle));

        time.update();

        return success();
    }

    Game::~Game() noexcept {
        delete mRenderer;
        deinitLibrary();
    }

    void Game::setFlags(RendererFlags flag) noexcept {
        mRenderer->flags |= flag;
    }

    void Game::setFlags(GameFlags flag) noexcept {
        mFlags |= flag;
    }

    bool Game::hasFlags(RendererFlags flag) noexcept {
        return mRenderer->flags & flag;
    }

    bool Game::hasFlags(GameFlags flag) noexcept {
        return mFlags & flag;
    }

    Error<noreturn> Game::draw() noexcept {
        float aspect = 0.0f;
        if(camera.aspect == Camera::autoAspect) {
            camera.aspect = static_cast<float>(mRenderer->swapchainImageExtent.width) 
                / static_cast<float>(mRenderer->swapchainImageExtent.height);
            aspect = camera.aspect;
            mRenderer->cameraMatrices.mappedMemory[1 + mRenderer->currentFrame] = camera.loadMatrices();
            camera.aspect = Camera::autoAspect;
        }
        else {
            mRenderer->cameraMatrices.mappedMemory[1 + mRenderer->currentFrame] = camera.loadMatrices();
            aspect = camera.aspect;
        }
        Renderer::Systems rendererSystems{
            ecs.componentManager.system<Component::transform>(), ecs.componentManager.system<Component::draw>()
        };
        TRY(mRenderer->drawFrame(camera.fov, aspect, pixelsPerMeter, rendererSystems));
        return success();
    }

    Error<ID> Game::loadMesh(const std::string& path) noexcept {
        return 0;
    }

    Error<ID> Game::loadTexture(const std::string& path) noexcept {
        return mRenderer->makeTexture(path);
    }
}
