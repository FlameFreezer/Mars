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
        if(e1 == nullEntity or e2 == nullEntity) return false;
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
        return nullEntity;
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

    std::vector<ComponentSystem<Collide>::Pair> filterSolids(ComponentSystem<Collide>& sysCollide, const ComponentSystem<Physics>& sysPhysics, ID id) noexcept {
        std::vector<ComponentSystem<Collide>::Pair> filtered;
        const Physics& p = sysPhysics[id];
        const Collide& c = sysCollide[id];
        for(const auto pair : sysCollide) {
            const Collide& cCurr = pair.data;

            //If we're moving left, but its left edge is right of our right edge, continue
            if(p.velocity.x < 0.0f and cCurr.position.x > c.position.x + c.scale.x) continue;
            //If we're moving right, but its right edge is left of our left edge, continue
            else if(p.velocity.x > 0.0f and cCurr.position.x + cCurr.scale.x < c.position.x) continue;
            //If we're not going right or left, but it is beyond either of our edges, continue
            else if(p.velocity.x == 0.0f and (cCurr.position.x + cCurr.scale.x < c.position.x 
                or c.position.x + c.scale.x < cCurr.position.x)) continue;

            //If we're moving up, but its top edge is under our bottom edge, continue
            if(p.velocity.y < 0.0f and cCurr.position.y > c.position.y + c.scale.y) continue;
            //If we're moving down, but its bottom edge is above our top edge, continue
            else if(p.velocity.y > 0.0f and cCurr.position.y + cCurr.scale.y < c.position.y) continue;
            //If we're not going up or down, but it is beyond either of our edges, continue
            else if(p.velocity.y == 0.0f and (cCurr.position.y + cCurr.scale.y < c.position.y 
                or c.position.y + c.scale.y < cCurr.position.y)) continue;

            filtered.push_back(pair);
        }
        return filtered;
    }

    glm::vec2 getR(const Collide& c1, const Collide& c2) noexcept {
        //Position is the rightmost/bottommost of the left/top edges
        float xPos = std::max(c1.position.x, c2.position.x);
        float yPos = std::max(c1.position.y, c2.position.y);
        //Scale comes from the distance to the leftmost/topmost of the right/bottom edges
        float xScale = std::min(c1.position.x + c1.scale.x, c2.position.x + c2.scale.x) - xPos;
        float yScale = std::min(c1.position.y + c1.scale.y, c2.position.y + c2.scale.y) - yPos;
        //r comes from the smaller axis of the dimensions of the region, or both if the region is square
        //doing both for squares ensures that corner collisions are always resolved
        glm::vec2 r(0.0f);
        if(xScale <= yScale) {
            r.x = xScale;
        }
        if(yScale <= xScale) {
            r.y = yScale;
        }
        return r;
    }

    void Game::resolveCollisions() noexcept {
        auto& sysPhysics = entityManager.system<Component::PHYSICS>();
        auto& sysCollide = entityManager.system<Component::COLLIDE>();
        auto& sysTransform = entityManager.system<Component::TRANSFORM>();
        auto& sysDynamics = entityManager.system<Component::DYNAMICS>();
        for(auto [d, eid] : sysDynamics) {
            //Assume there's no floor. If there is a floor, it will get set later
            d.floorID = nullID;
            const Entity e = entityManager.getEntity(eid);
            for(ID id : d.collisions) {
                if(!sysCollide[id].isSolid) continue;
                glm::vec2 r = getR(sysCollide[eid], sysCollide[id]);
                Physics& p = sysPhysics[eid];
                //Give r its proper sign based on the entity's velocity
                r.x *= glm::sign(p.velocity.x);
                r.y *= glm::sign(p.velocity.y);
                //Subtract r to take the entity out of the wall
                sysCollide[eid].position -= r;
                sysTransform[eid].position -= r;
                //Set the speeds that were into the wall to zero
                if(r.x != 0.0f) p.velocity.x = 0.0f;
                if(r.y != 0.0f) p.velocity.y = 0.0f;
                //Report the floor
                if(glm::sign(r.y) == glm::sign(p.gravity.y)) {
                    d.floorID = id;
                }
            }
        }
    }

    void sortCollidesP(ComponentSystem<Collide>& sysCollide, u64 start, u64 end) noexcept {
        if(start >= end) return;
        ID* IDs = sysCollide.getIDs();
        u64 i = 0, j = 0;
        while(j < end) {
            if(sysCollide[j].position.y < sysCollide[end].position.y) {
                if(i != j) {
                    sysCollide.swap(IDs[i], IDs[j]);
                }
                i++;
            }
            j++;
        }
        sysCollide.swap(IDs[i], IDs[end]);
        sortCollidesP(sysCollide, start, i - 1);
        sortCollidesP(sysCollide, i + 1, end);
    }

    //Quicksorts the collides based on the x-coordinate of their positions
    void sortCollides(ComponentSystem<Collide>& sysCollide) noexcept {
        sortCollidesP(sysCollide, 0, sysCollide.size() - 1);
    }

    void Game::applyPhysics() noexcept {
        const auto& sysPhysics = entityManager.system<Component::PHYSICS>();
        //For every physics component
        for(auto [p, id] : sysPhysics) {
            //Get the entity associated with the component
            const Entity e = entityManager.getEntity(id);
            Transform& t = transform(e);
            //Update its position using the velocity
            t.position += p.velocity * deltaTime();
            //If it has a collision component, update that too
            if(e.signature().has(Component::COLLIDE)) {
                Collide& c = collide(e);
                c.position += p.velocity * deltaTime();
            }
        }
    }

    void Game::findCollisions() noexcept {
        auto& sysDynamics = entityManager.system<Component::DYNAMICS>();
        auto& sysCollide = entityManager.system<Component::COLLIDE>();
        //Clear out all collision lists before going on to refill them
        for(auto [d, id] : sysDynamics) {
            d.collisions.clear();
        }
        //For every dynamic entity
        for(auto [d1, id1] : sysDynamics) {
            const Entity e1 = entityManager.getEntity(id1);
            //For every entity with collision
            for(auto [c2, id2] : sysCollide) {
                const Entity e2 = entityManager.getEntity(id2);
                if(checkCollision(e1, e2)) {
                    d1.collisions.push_back(id2);
                    //Add e1 to the collision list of e2 if it has Dynamics
                    if(e2.signature().has(Component::DYNAMICS)) {
                        auto& d2 = sysDynamics[id2];
                        d2.collisions.push_back(e1.id());
                    }
                }
            }
        }
    }
}
