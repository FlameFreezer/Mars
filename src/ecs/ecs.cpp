module;

#include <vector>

#include <glm/glm.hpp>

module ecs;

#define ALLOC_SYSTEM(component) mSystems[(component)] = new ComponentSystem<typename GetComp<(component)>::Type>()

namespace mars {
    ComponentSystemParent*& ComponentSystems::operator[](Component c) noexcept {
        return mSystems[static_cast<ComponentT>(c)];
    }

    const ComponentSystemParent* const& ComponentSystems::operator[](Component c) const noexcept {
        return mSystems[static_cast<ComponentT>(c)];
    }

    PhysicsManager::PhysicsManager(EntityManager& em) noexcept : 
        sysPhysics(em.system<Component::physics>()), 
        sysDynamics(em.system<Component::dynamics>()), 
        sysCollide(em.system<Component::collide>()), 
        sysTransform(em.system<Component::transform>()) {}
    
    bool PhysicsManager::checkCollision(ID id1, ID id2) const noexcept {
        //Entities cannot collide with themselves
        if(id1 == id2) return false;
        //Null entity produces no collisions
        if(id1 == nullID or id2 == nullID) return false;
        const Collide& s1 = sysCollide[id1];
        const Collide& s2 = sysCollide[id2];
        bool xWithin = false, yWithin = false;
        if(s1.boundingShape == BoundingShape::rectangle and s2.boundingShape == BoundingShape::rectangle) { 
            xWithin = s1.position.x + s1.scale.x >= s2.position.x and s2.position.x + s2.scale.x >= s1.position.x;
            yWithin = s1.position.y + s1.scale.y >= s2.position.y and s2.position.y + s2.scale.y >= s1.position.y;
        }
        else if(s1.boundingShape == BoundingShape::rectangle and s2.boundingShape == BoundingShape::circle) {}
        else if(s1.boundingShape == BoundingShape::circle and s2.boundingShape == BoundingShape::rectangle) {}
        else {}
        return xWithin and yWithin;
    }
    bool PhysicsManager::checkCollision(Entity e1, Entity e2) const noexcept {
        //Entities cannot collide with themselves
        if(e1 == e2) return false;
        //Null entity produces no collisions
        if(e1 == nullEntity or e2 == nullEntity) return false;
        const Collide& s1 = sysCollide[e1];
        const Collide& s2 = sysCollide[e2];
        bool xWithin = false, yWithin = false;
        if(s1.boundingShape == BoundingShape::rectangle and s2.boundingShape == BoundingShape::rectangle) { 
            xWithin = s1.position.x + s1.scale.x >= s2.position.x and s2.position.x + s2.scale.x >= s1.position.x;
            yWithin = s1.position.y + s1.scale.y >= s2.position.y and s2.position.y + s2.scale.y >= s1.position.y;
        }
        else if(s1.boundingShape == BoundingShape::rectangle and s2.boundingShape == BoundingShape::circle) {}
        else if(s1.boundingShape == BoundingShape::circle and s2.boundingShape == BoundingShape::rectangle) {}
        else {}
        return xWithin and yWithin;
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
            //Get the entity associated with the component
            Transform& t = sysTransform[id];
            //Update its position using the velocity
            t.position += p.velocity * deltaTime;
            //If it has a collision component, update that too
            if(sysCollide.has(id)) {
                Collide& c = sysCollide[id];
                c.position += p.velocity * deltaTime;
            }
        }
    }

    void PhysicsManager::getCollisions() noexcept {
        //Clear out all collision lists before going on to refill them
        for(auto [d, id] : sysDynamics) {
            d.collisions.clear();
        }
        //For every dynamic entity
        for(auto [d1, id1] : sysDynamics) {
            //For every entity with collision
            for(auto [c2, id2] : sysCollide) {
                if(checkCollision(id1, id2)) {
                    d1.collisions.push_back(id2);
                    //Add e1 to the collision list of e2 if it has Dynamics
                    if(sysDynamics.has(id2)) {
                        auto& d2 = sysDynamics[id2];
                        d2.collisions.push_back(id1);
                    }
                }
            }
        }
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

    void PhysicsManager::resolveCollisions() noexcept {
        for(auto [d, eid] : sysDynamics) {
            //Assume there's no floor. If there is a floor, it will get set later
            d.floorID = nullID;
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
    
    void PhysicsManager::doPhysicsStep(float deltaTime) noexcept {
        applyPhysics(deltaTime);
        getCollisions();
        resolveCollisions();
    }

    EntityManager::EntityManager() noexcept {
        //Skip the null entity
        for(ID i = 1; i < maxEntities; i++) {
            mIDQueue.push(i);
        }
        ALLOC_SYSTEM(Component::transform);
        ALLOC_SYSTEM(Component::physics);
        ALLOC_SYSTEM(Component::draw);
        ALLOC_SYSTEM(Component::collide);
        ALLOC_SYSTEM(Component::dynamics);
        createNullEntity();
    }

    EntityManager::~EntityManager() noexcept {
        delete[] mEntities;
        for(ComponentT i = 0; i < numComponents; i++) {
            delete mSystems.mSystems[i];
        }
    }

    void EntityManager::createNullEntity() noexcept {
        //The null entity takes up space in every component system
        for(u64 i = 0; i < numComponents; i++) {
            mSystems.mSystems[i]->reserve(nullID);
        }
    }

    Error<Entity> EntityManager::createEntity(Signature sig) noexcept {
        if(mIDQueue.empty()) {
            return fatal<Entity>("Tried to create an entity, but the maximum number of entities were made!");
        }
        const ID id = mIDQueue.front();
        mIDQueue.pop();
        //Number of the current bit we are checking in the signature
        //Corresponds to the component number for this bit
        ComponentT bitNum = 0;
        //For each bit in the signature
        for(SignatureT x = 1; x != 0; x <<= 1) {             
            if(sig.getBits() & x) {
                mSystems[static_cast<Component>(bitNum)]->reserve(id);
            }
            bitNum++;
        }
        Entity e(id, sig);
        mEntities[id] = e;
        return e;
    }

    void EntityManager::destroyEntity(Entity e) noexcept {
        //Never destroy the null entity
        if(e == nullEntity) return;
        ID id = e.id();        
        mIDQueue.push(id);
        Signature sig = e.signature();
        ComponentT bitNum = 0;
        for(SignatureT x = 1; x < (1 << 31); x <<= 1) {
            if(sig.getBits() & x) {
                mSystems[static_cast<Component>(bitNum++)]->erase(id);
            }
        }
        mEntities[id] = nullEntity;
    }

    Entity EntityManager::getEntity(ID id) const noexcept {
        return mEntities[id];
    }
};
