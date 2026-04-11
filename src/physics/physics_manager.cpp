module;

#include <glm/glm.hpp>

module physics_manager;
import ecs;

namespace mars {
    ComponentSystem<Physics>& sysPhysics        = ECS::get().componentManager.system<Component::physics>();
    ComponentSystem<Dynamics>& sysDynamics      = ECS::get().componentManager.system<Component::dynamics>();
    ComponentSystem<Transform>& sysTransform    = ECS::get().componentManager.system<Component::transform>();
    ComponentSystem<Collide>& sysCollide        = ECS::get().componentManager.system<Component::collide>();
    ComponentSystem<LedgeGrab>& sysLedgeGrab    = ECS::get().componentManager.system<Component::ledgeGrab>();

    PhysicsManager& PhysicsManager::get() noexcept {
        static PhysicsManager instance;
        return instance;
    }

    Position PhysicsManager::position(ID id) noexcept {
        return {
            sysTransform[id].position, sysCollide[id].position
        };
    }

    bool doCollisionCheck(const Collide& c1, const Collide& c2) noexcept {
        bool xWithin = false, yWithin = false;
        if(c1.boundingShape == BoundingShape::rectangle and c2.boundingShape == BoundingShape::rectangle) { 
            xWithin = c1.position.x + c1.scale.x > c2.position.x and c2.position.x + c2.scale.x > c1.position.x;
            yWithin = c1.position.y + c1.scale.y > c2.position.y and c2.position.y + c2.scale.y > c1.position.y;
        }
        else if(c1.boundingShape == BoundingShape::rectangle and c2.boundingShape == BoundingShape::circle) {}
        else if(c1.boundingShape == BoundingShape::circle and c2.boundingShape == BoundingShape::rectangle) {}
        else {}
        return xWithin and yWithin;
    }
    
    bool PhysicsManager::checkCollision(ID id1, ID id2) const noexcept {
        //Entities cannot collide with themselves
        if(id1 == id2) return false;
        //Null entity produces no collisions
        if(id1 == nullID or id2 == nullID) return false;
        const Collide& c1 = sysCollide[id1];
        const Collide& c2 = sysCollide[id2];
        return doCollisionCheck(sysCollide[id1], sysCollide[id2]);
    }
    bool PhysicsManager::checkCollision(Entity e1, Entity e2) const noexcept {
        //Entities cannot collide with themselves
        if(e1 == e2) return false;
        //Null entity produces no collisions
        if(e1 == nullEntity or e2 == nullEntity) return false;
        return doCollisionCheck(sysCollide[e1], sysCollide[e2]);
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

    void PhysicsManager::applyPhysics(float deltaTime) noexcept {
        //For every physics component
        for(auto [p, id] : sysPhysics) {
            //Apply physics
            p.velocity += p.gravity * deltaTime;
            //Get the entity associated with the component
            Transform& t = sysTransform[id];
            //Update its position using the velocity
            t.position += p.velocity * deltaTime;
            //If it has a collision component, update that too
            if(sysCollide.has(id)) {
                Collide& c = sysCollide[id];
                c.position += p.velocity * deltaTime;
            }
            static constexpr float delta = 0.02f;
            if(std::abs(p.velocity.x) <= delta) p.velocity.x = 0.0f;
            if(std::abs(p.velocity.y) <= delta) p.velocity.y = 0.0f;
        }
    }

    static glm::vec2 getHorizontal(const glm::vec2& g) noexcept {
        static constexpr glm::vec3 z(0.0f, 0.0f, 1.0f);
        const glm::vec3 gFixed(std::abs(g.x), std::abs(g.y), 0.0f);
        const glm::vec3 h = glm::cross(gFixed, z);
        return glm::normalize(glm::vec2(h.x, h.y));
    }

    void PhysicsManager::getCollisions() noexcept {
        //Clear out all collision lists before going on to refill them
        for(auto [d, id] : sysDynamics) {
            d.collisions.clear();
            d.wallID = nullID;
            d.floorID = nullID;
            if(sysLedgeGrab.has(id)) sysLedgeGrab[id].ledgeID = nullID;
        }
        //For every dynamic entity
        for(auto [d1, id1] : sysDynamics) {
            auto& p = sysPhysics[id1];
            auto& c = sysCollide[id1];
            auto& l = sysLedgeGrab[id1];
            //Ledge grabbing disabled if gravity isn't in a cardinal direction
            const bool gravCardinal = p.gravity.x == 0.0f or p.gravity.y == 0.0f;
            const bool canGrabLedges = sysLedgeGrab.has(id1) and gravCardinal;
            //For every entity with collision
            for(auto [c2, id2] : sysCollide) {
                //Report collisions
                if(checkCollision(id1, id2)) {
                    d1.collisions.push_back(id2);
                    //Add e1 to the collision list of e2 if it has Dynamics
                    if(sysDynamics.has(id2)) {
                        auto& d2 = sysDynamics[id2];
                        d2.collisions.push_back(id1);
                    }
                }
                //Report ledges
                if(!canGrabLedges) continue;
                const glm::vec2 oldpos = c.position;
                const glm::vec2 h = getHorizontal(p.gravity);
                //Project distance vector onto the vertical
                const float vDifference = glm::dot((c2.position - c.position), glm::normalize(p.gravity));
                c.position += h * l.ledgeGrabRange * glm::sign(glm::dot(p.velocity, h));
                //Must be moving downwards
                if(glm::dot(p.velocity, p.gravity) <= 0) goto end;            
                //Candidate must be in range (collides after moving to range)
                if(!checkCollision(id1, id2)) goto end;
                //Candidate's top must be within our collision box
                if(p.gravity.y > 0.0f and (c2.position.y < c.position.y or c2.position.y > c.position.y + c.scale.y)) goto end;
                if(p.gravity.y < 0.0f and (c2.position.y + c2.scale.y > c.position.y + c.scale.y or c2.position.y + c2.scale.y < c.position.y)) goto end;
                if(p.gravity.x > 0.0f and (c2.position.x < c.position.x or c2.position.x > c.position.x + c.scale.x)) goto end;
                if(p.gravity.x < 0.0f and (c2.position.x + c2.scale.x > c.position.x + c.scale.x or c2.position.x + c2.scale.x < c.position.x)) goto end;
                //Ledge candidate
                if(l.ledgeID == nullID) {
                    l.ledgeID = id2;
                }
                //TODO: pick best ledge (optional? may prevent this through design)
                end:
                c.position = oldpos;
            }
        }
    }

    glm::vec2 getR(const Collide& c1, const Collide& c2) noexcept {
        //Position is the rightmost/bottommost of the left/top edges
        const float xPos = std::max(c1.position.x, c2.position.x);
        const float yPos = std::max(c1.position.y, c2.position.y);
        //Scale comes from the distance to the leftmost/topmost of the right/bottom edges
        const float xScale = std::min(c1.position.x + c1.scale.x, c2.position.x + c2.scale.x) - xPos;
        const float yScale = std::min(c1.position.y + c1.scale.y, c2.position.y + c2.scale.y) - yPos;
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

    void PhysicsManager::resolveCollisions() noexcept {
        for(auto [d, eid] : sysDynamics) {
            for(ID id : d.collisions) {
                if(!sysCollide[id].isSolid) continue;
                glm::vec2 r = getR(sysCollide[eid], sysCollide[id]);
                Physics& p = sysPhysics[eid];
                //Give r its proper sign based on the entity's velocity
                r.x *= glm::sign(p.velocity.x);
                r.y *= glm::sign(p.velocity.y);
                //Subtract r to take the entity out of the wall
                position(eid) -= r;
                //Unreport the ledge if we were moving away from the ledge
                if(sysLedgeGrab.has(eid)) {
                    auto& l = sysLedgeGrab[eid];
                    const glm::vec2 h = getHorizontal(p.gravity);
                    if(l.ledgeID == id and glm::dot(h, r) > 0.0f) {
                        l.ledgeID = nullID;
                    }
                }
                //Report the floor
                if(glm::sign(r.y) == glm::sign(p.gravity.y)) {
                    d.floorID = id;
                }
                //Report the wall
                if(r != glm::vec2(0.0f) and glm::dot(r, p.gravity) == 0.0f) {
                    d.wallID = id;
                }
                //Set the speeds that were into the wall to zero
                if(r.x != 0.0f) p.velocity.x = 0.0f;
                if(r.y != 0.0f) p.velocity.y = 0.0f;
            }
        }
    }
}
