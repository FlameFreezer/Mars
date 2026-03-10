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

    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game"), flags(0), renderer(nullptr), gamepad(nullptr) {}
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name), flags(0), renderer(nullptr), gamepad(nullptr) {}
    Error<noreturn> Game::init() noexcept {
        TRY(initLibrary());
        renderer = new Renderer();
        TRY(renderer->init(appName, shapes.rectangle));

        int numGamepads;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
        if(numGamepads != 0) {
            gamepad = SDL_OpenGamepad(gamepads[0]);
        }
        time = std::chrono::steady_clock::now();
        return success();
    }
    Game::~Game() noexcept {
        delete renderer;
        SDL_CloseGamepad(gamepad);
        deinitLibrary();
    }
    void Game::setFlags(RendererFlags flag) noexcept {
        renderer->flags |= flag;
    }
    void Game::setFlags(GameFlags flag) noexcept {
        flags |= flag;
    }
    bool Game::hasFlags(RendererFlags flag) noexcept {
        return renderer->flags & flag;
    }
    bool Game::hasFlags(GameFlags flag) noexcept {
        return flags & flag;
    }
    std::chrono::steady_clock::time_point::rep Game::getFrameTime() const noexcept {
        return time.time_since_epoch().count();
    }
    std::chrono::nanoseconds::rep Game::getDeltaTime() const noexcept {
        return deltaTime.count();
    }
    std::chrono::duration<float, std::chrono::seconds::period>::rep Game::getDeltaTimeSeconds() const noexcept {
        return std::chrono::duration_cast<std::chrono::duration<float, std::chrono::seconds::period>>(deltaTime).count();
    }
    void Game::updateTime() noexcept {
        const auto now = std::chrono::steady_clock::now();
        deltaTime = now - time;
        time = now;
    }
    void Game::updateKeyState() noexcept {
        keyState = SDL_GetKeyboardState(nullptr);
    }
    Error<noreturn> Game::draw() noexcept {
        float aspect = 0.0f;
        if(camera.aspect == Camera::autoAspect) {
            camera.aspect = static_cast<float>(renderer->swapchainImageExtent.width) / static_cast<float>(renderer->swapchainImageExtent.height);
            aspect = camera.aspect;
            renderer->cameraMatrices.mappedMemory[1 + renderer->currentFrame] = camera.loadMatrices();
            camera.aspect = Camera::autoAspect;
        }
        else {
            renderer->cameraMatrices.mappedMemory[1 + renderer->currentFrame] = camera.loadMatrices();
            aspect = camera.aspect;
        }
        Renderer::Systems rendererSystems;
        rendererSystems.transform = &entityManager.system<Components::TRANSFORM>();
        rendererSystems.mesh = &entityManager.system<Components::MESH>();
        rendererSystems.texture = &entityManager.system<Components::TEXTURE>();
        TRY(renderer->drawFrame(deltaTime, camera.fov, aspect, rendererSystems));
        return success();
    }

    Error<ID> Game::loadMesh(const std::string& path) noexcept {
        return 0;
    }
    Error<ID> Game::loadTexture(const std::string& path) noexcept {
        return renderer->makeTexture(path);
    }
    bool Game::checkCollision(const Transform& t1, const Transform& t2) const noexcept {
        bool xWithin = std::max(t1.position.x, t2.position.x) <= std::min(t1.position.x + t1.scale.x, t2.position.x + t2.scale.x); 
        bool yWithin = std::max(t1.position.y, t2.position.y) <= std::min(t1.position.y + t1.scale.y, t2.position.y + t2.scale.y); 
        return xWithin and yWithin;
    }

    Transform& Game::getTransform(Entity e) noexcept {
        return entityManager.system<Components::TRANSFORM>()[e.id()];
    }
    glm::vec2& Game::getVelocity(Entity e) noexcept {
        return entityManager.system<Components::PHYSICS>()[e.id()].velocity;
    }

    void Game::setMesh(Entity e, ID id) noexcept {
        entityManager.system<Components::MESH>()[e.id()] = id;
    }
    void Game::setTexture(Entity e, ID id) noexcept {
        entityManager.system<Components::TEXTURE>()[e.id()] = id;
    }
}
