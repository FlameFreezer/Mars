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

constexpr float defaultPixelsPerMeter = 64.0f;

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

    Game::Game() noexcept : 
        mWindowName("My Mars Game"), 
        mAppName("My Mars Game"), 
        mFlags(0), 
        mRenderer(nullptr), 
        gamepad(nullptr),
        pixelsPerMeter(defaultPixelsPerMeter) {}

    Game::Game(const std::string& name) noexcept : 
        mWindowName(name), 
        mAppName(name), 
        mFlags(0), 
        mRenderer(nullptr), 
        gamepad(nullptr),
        pixelsPerMeter(defaultPixelsPerMeter) {}

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
        mRendererSystems.transform = &entityManager.system<Component::TRANSFORM>();
        mRendererSystems.draw = &entityManager.system<Component::DRAW>();
        TRY(mRenderer->drawFrame(mDeltaTime, camera.fov, aspect, pixelsPerMeter, mRendererSystems));
        return success();
    }

    Error<ID> Game::loadMesh(const std::string& path) noexcept {
        return 0;
    }

    Error<ID> Game::loadTexture(const std::string& path) noexcept {
        return mRenderer->makeTexture(path);
    }

    bool Game::checkCollision(Entity e1, Entity e2) const noexcept {
        //Entities cannot collide with themselves
        if(e1 == e2) return false;
        //Null entity produces no collisions
        if(e1 == EntityManager::nullEntity or e2 == EntityManager::nullEntity) return false;
        const Collide& s1 = *collide(e1).data();
        const Collide& s2 = *collide(e2).data();
        bool xWithin = false, yWithin = false;
        if(s1.boundingShape == BoundingShape::RECTANGLE and s2.boundingShape == BoundingShape::RECTANGLE) { 
            xWithin = s1.position.x + s1.scale.x >= s2.position.x and s2.position.x + s2.scale.x >= s1.position.x;
            yWithin = s1.position.y + s1.scale.y >= s2.position.y and s2.position.y + s2.scale.y >= s1.position.y;
        }
        else if(s1.boundingShape == BoundingShape::RECTANGLE and s2.boundingShape == BoundingShape::CIRCLE) {}
        else if(s1.boundingShape == BoundingShape::CIRCLE and s2.boundingShape == BoundingShape::RECTANGLE) {}
        else {}
        return xWithin and yWithin;
    }

    Entity Game::getCollision(Entity e) const noexcept {
        const auto sysCollide = entityManager.system<Component::COLLIDE>();
        const ID* ids = sysCollide.getIDs();
        for(u64 i = 0; i < sysCollide.size(); i++) {
            Entity other = entityManager.getEntity(ids[i]);
            if(checkCollision(e, other)) {
                return other;
            }
        }
        return EntityManager::nullEntity;
    }

    Entity Game::getFloor(Entity e) noexcept {
        Collide* s = collide(e).data();
        static constexpr float snappingRange = 0.05f;
        s->position.y += snappingRange;
        Entity floor = getCollision(e);
        s->position.y -= snappingRange;
        return floor;
    }

    Error<Transform*> Game::transform(Entity e) noexcept {
        if(!e.signature().has(Component::TRANSFORM)) {
            return fatal<Transform*>(std::format("Tried to get Transform for the Entity with ID {}, which does not have a Transform component", e.id()));
        }
        return &entityManager.system<Component::TRANSFORM>()[e.id()];
    }
    Error<const Transform*> Game::transform(Entity e) const noexcept {
        if(!e.signature().has(Component::TRANSFORM)) {
            return fatal<const Transform*>(std::format("Tried to get Transform for the Entity with ID {}, which does not have a Transform component", e.id()));
        }
        return &entityManager.system<Component::TRANSFORM>()[e.id()];
    }

    Error<Physics*> Game::physics(Entity e) noexcept {
        if(!e.signature().has(Component::PHYSICS)) {
            return fatal<Physics*>(std::format("Tried to get Physics for the Entity with ID {}, which does not have a Physics component", e.id()));
        }
        return &entityManager.system<Component::PHYSICS>()[e.id()];
    }
    Error<const Physics*> Game::physics(Entity e) const noexcept {
        if(!e.signature().has(Component::PHYSICS)) {
            return fatal<const Physics*>(std::format("Tried to get Physics for the Entity with ID {}, which does not have a Physics component", e.id()));
        }
        return &entityManager.system<Component::PHYSICS>()[e.id()];
    }

    Error<Collide*> Game::collide(Entity e) noexcept {
        if(!e.signature().has(Component::COLLIDE)) {
            return fatal<Collide*>(std::format("Tried to get Collide for the Entity with ID {}, which does not have a Collide component", e.id()));
        }
        return &entityManager.system<Component::COLLIDE>()[e.id()];
    }

    Error<const Collide*> Game::collide(Entity e) const noexcept {
        if(!e.signature().has(Component::COLLIDE)) {
            return fatal<const Collide*>(std::format("Tried to get Collide for the Entity with ID {}, which does not have a Collide component", e.id()));
        }
        return &entityManager.system<Component::COLLIDE>()[e.id()];
    }

    void Game::setMesh(Entity e, ID id) noexcept {
        entityManager.system<Component::DRAW>()[e.id()].meshID = id;
    }
    void Game::setTexture(Entity e, ID id) noexcept {
        entityManager.system<Component::DRAW>()[e.id()].textureID = id;
    }
}
