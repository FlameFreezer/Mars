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
        const Collide& s1 = collide(e1);
        const Collide& s2 = collide(e2);
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
        auto& s = collide(e);
        static constexpr float snappingRange = 0.05f;
        s.position.y += snappingRange;
        Entity floor = getCollision(e);
        s.position.y -= snappingRange;
        return floor;
    }

    Transform& Game::transform(Entity e) noexcept {
        return entityManager.system<Component::TRANSFORM>()[e.id()];
    }
    const Transform& Game::transform(Entity e) const noexcept {
        return entityManager.system<Component::TRANSFORM>()[e.id()];
    }

    Physics& Game::physics(Entity e) noexcept {
        return entityManager.system<Component::PHYSICS>()[e.id()];
    }
    const Physics& Game::physics(Entity e) const noexcept {
        return entityManager.system<Component::PHYSICS>()[e.id()];
    }

    Collide& Game::collide(Entity e) noexcept {
        return entityManager.system<Component::COLLIDE>()[e.id()];
    }

    const Collide& Game::collide(Entity e) const noexcept {
        return entityManager.system<Component::COLLIDE>()[e.id()];
    }

    Dynamics& Game::dynamics(Entity e) noexcept {
        return entityManager.system<Component::DYNAMICS>()[e.id()];
    }

    const Dynamics& Game::dynamics(Entity e) const noexcept {
        return entityManager.system<Component::DYNAMICS>()[e.id()];
    }

    void Game::setMesh(Entity e, ID id) noexcept {
        entityManager.system<Component::DRAW>()[e.id()].meshID = id;
    }
    void Game::setTexture(Entity e, ID id) noexcept {
        entityManager.system<Component::DRAW>()[e.id()].textureID = id;
    }
    void Game::applyPhysics() noexcept {
        auto& sysPhysics = entityManager.system<Component::PHYSICS>();
        //For every physics component
        for(u64 i = 0; i < sysPhysics.size(); i++) {
            //Get the entity associated with the component
            const Entity e = entityManager.getEntity(sysPhysics.getIDs()[i]);
            const Physics& p = sysPhysics.data()[i];
            Transform& t = transform(e);
            //Update its position using the velocity
            t.position += glm::vec3(p.velocity, 0.0f) * deltaTime();
            //If it has a collision component, update that too
            if(e.signature().has(Component::COLLIDE)) {
                Collide& c = collide(e);
                c.position = t.position;
            }
        }
    }
    void Game::findCollisions() noexcept {
        auto& sysDynamics = entityManager.system<Component::DYNAMICS>();
        auto& sysCollide = entityManager.system<Component::COLLIDE>();
        //Clear out all collision lists before going on to refill them
        for(u64 i = 0; i < sysDynamics.size(); i++) {
            sysDynamics.data()[i].collisions.clear();
        }
        //For every dynamic entity
        for(u64 i = 0; i < sysDynamics.size(); i++) {
            const Entity e1 = entityManager.getEntity(sysDynamics.getIDs()[i]);
            auto& d1 = sysDynamics.data()[i];
            //For every entity with collision
            for(u64 j = 0; j < sysCollide.size(); j++) {
                const Entity e2 = entityManager.getEntity(sysCollide.getIDs()[j]);
                if(checkCollision(e1, e2)) {
                    d1.collisions.push_back(e2.id());
                    //Add e1 to the collision list of e2 if it has Dynamics
                    if(e2.signature().has(Component::DYNAMICS)) {
                        auto& d2 = sysDynamics[e2.id()];
                        d2.collisions.push_back(e1.id());
                    }
                }
            }
        }
    }
}
