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

#define OBJECT_CAPACITY 50

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

    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game"), flags(0), renderer(nullptr), objects(OBJECT_CAPACITY), gamepad(nullptr) {}
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name), flags(0), renderer(nullptr), objects(OBJECT_CAPACITY), gamepad(nullptr) {}
    Error<noreturn> Game::init() noexcept {
        TRY(initLibrary());
        renderer = new Renderer();
        TRY(renderer->init(appName));

        int numGamepads;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
        if(gamepads != nullptr) {
            gamepad = SDL_OpenGamepad(gamepads[0]);
        }
        time = std::chrono::steady_clock::now();
        objects.size = 0;
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
        auto const now = std::chrono::steady_clock::now();
        deltaTime = now - time;
        time = now;
    }
    void Game::updateKeyState() noexcept {
        keyState = SDL_GetKeyboardState(nullptr);
    }
    Error<noreturn> Game::draw() noexcept {
        if(camera.aspect == Camera::autoAspect) {
            camera.aspect = static_cast<float>(renderer->swapchainImageExtent.width) / static_cast<float>(renderer->swapchainImageExtent.height);
            renderer->cameraMatrices.mappedMemory[renderer->currentFrame] = camera.loadMatrices();
            camera.aspect = Camera::autoAspect;
        }
        else {
            renderer->cameraMatrices.mappedMemory[renderer->currentFrame] = camera.loadMatrices();
        }
        glm::mat4* modelMatrices = new glm::mat4[objects.size];
        objects.getModelMatrices(modelMatrices);
        Renderer::Objects rendererObjects{modelMatrices, objects.meshIDs, objects.textureIDs, objects.size};
        TRY(renderer->drawFrame(deltaTime, rendererObjects));
        delete[] modelMatrices;
        return success();
    }

    Error<ID> Game::loadMesh(std::string const& path) noexcept {
        if(path.compare("CUBE") == 0) {
            return renderer->makeMesh({Cube::vertices.data(), Cube::vertices.max_size()}, {Cube::indices.data(), Cube::indices.max_size()}, getFrameTime());
        }
        return 0;
    }
    Error<ID> Game::loadTexture(std::string const& path) noexcept {
        return renderer->createTexture(path, getFrameTime());
    }
    Error<ID> Game::createObject(ID meshID, ID textureID, glm::vec3 const& pos, glm::vec3 const& scale) noexcept {
        objects.meshIDs[objects.size] = meshID;
        objects.textureIDs[objects.size] = textureID;
        objects.positions[objects.size] = pos;
        objects.scales[objects.size] = scale;

        Error<ID> id = objects.makeID(static_cast<double>(pos.x + pos.y + pos.z), getFrameTime());
        if(!id) return id;

        objects.setActiveAt(id);
        objects.setIndex(id, objects.size++);
        return id;
    }
    Rect2D Game::getWindowDimensions() const noexcept {
        int w, h;
        SDL_GetWindowSize(renderer->window, &w, &h);
        return {static_cast<std::uint64_t>(w), static_cast<std::uint64_t>(h)};
    }
    void Game::resizeWindow(std::uint32_t width, std::uint32_t height) noexcept {
        int w = static_cast<int>(width), h = static_cast<int>(height);
        SDL_SetWindowSize(renderer->window, w, h);
        renderer->flags |= flagBits::recreateSwapchain;
    }
}
