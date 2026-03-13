module;

#include <string>
#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

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

    Game::Game() noexcept : mWindowName("My Mars Game"), mAppName("My Mars Game"), mFlags(0), mRenderer(nullptr), gamepad(nullptr) {}
    Game::Game(const std::string& name) noexcept : mWindowName(name), mAppName(name), mFlags(0), mRenderer(nullptr), gamepad(nullptr) {}
    Error<noreturn> Game::init() noexcept {
        TRY(initLibrary());
        mRenderer = new Renderer();
        TRY(mRenderer->init(mAppName, shapes.rectangle));

        int numGamepads;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
        if(numGamepads != 0) {
            gamepad = SDL_OpenGamepad(gamepads[0]);
        }
        mTime = std::chrono::steady_clock::now();
        return success();
    }
    Game::~Game() noexcept {
        delete mRenderer;
        SDL_CloseGamepad(gamepad);
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
    std::chrono::steady_clock::time_point::rep Game::frameTime() const noexcept {
        return mTime.time_since_epoch().count();
    }
    std::chrono::nanoseconds::rep Game::deltaTimeNanoseconds() const noexcept {
        return mDeltaTime.count();
    }
    std::chrono::duration<float, std::chrono::seconds::period>::rep Game::deltaTime() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(mDeltaTime).count();
    }
    void Game::updateTime() noexcept {
        const auto now = std::chrono::steady_clock::now();
        mDeltaTime = now - mTime;
        mTime = now;
    }
    void Game::updateKeyState() noexcept {
        keyState = SDL_GetKeyboardState(nullptr);
    }
    Error<noreturn> Game::draw() noexcept {
        float aspect = 0.0f;
        if(camera.aspect == Camera::autoAspect) {
            camera.aspect = static_cast<float>(mRenderer->swapchainImageExtent.width) / static_cast<float>(mRenderer->swapchainImageExtent.height);
            aspect = camera.aspect;
            mRenderer->cameraMatrices.mappedMemory[1 + mRenderer->currentFrame] = camera.loadMatrices();
            camera.aspect = Camera::autoAspect;
        }
        else {
            mRenderer->cameraMatrices.mappedMemory[1 + mRenderer->currentFrame] = camera.loadMatrices();
            aspect = camera.aspect;
        }
        Renderer::Systems mRendererSystems;
        mRendererSystems.transform = &entityManager.system<Components::TRANSFORM>();
        mRendererSystems.mesh = &entityManager.system<Components::MESH>();
        mRendererSystems.texture = &entityManager.system<Components::TEXTURE>();
        TRY(mRenderer->drawFrame(mDeltaTime, camera.fov, aspect, mRendererSystems));
        return success();
    }

    Error<ID> Game::loadMesh(const std::string& path) noexcept {
        return 0;
    }
    Error<ID> Game::loadTexture(const std::string& path) noexcept {
        return mRenderer->makeTexture(path);
    }
    bool Game::checkCollision(const Transform& t1, const Transform& t2) const noexcept {
        bool xWithin = std::max(t1.position.x, t2.position.x) <= std::min(t1.position.x + t1.scale.x, t2.position.x + t2.scale.x); 
        bool yWithin = std::max(t1.position.y, t2.position.y) <= std::min(t1.position.y + t1.scale.y, t2.position.y + t2.scale.y); 
        return xWithin and yWithin;
    }

    Error<Transform*> Game::transform(Entity e) noexcept {
        if(!e.signature().has(Components::TRANSFORM)) {
            return fatal<Transform*>(std::format("Tried to get Transform for the Entity with ID {}, which does not have a Transform component", e.id()));
        }
        return &entityManager.system<Components::TRANSFORM>()[e.id()];
    }
    Error<Physics*> Game::physics(Entity e) noexcept {
        if(!e.signature().has(Components::PHYSICS)) {
            return fatal<Physics*>(std::format("Tried to get Physics for the Entity with ID {}, which does not have a Physics component", e.id()));
        }
        return &entityManager.system<Components::PHYSICS>()[e.id()];
    }

    void Game::setMesh(Entity e, ID id) noexcept {
        entityManager.system<Components::MESH>()[e.id()] = id;
    }
    void Game::setTexture(Entity e, ID id) noexcept {
        entityManager.system<Components::TEXTURE>()[e.id()] = id;
    }
}
