module;

export module physics_manager;
import types;
import entity;
import component;

namespace mars {
    export class PhysicsManager {
        ComponentSystem<Physics>& sysPhysics;
        ComponentSystem<Dynamics>& sysDynamics;
        ComponentSystem<Transform>& sysTransform;
        ComponentSystem<Collide>& sysCollide;
        public:
        PhysicsManager(ComponentManager& cm) noexcept;
        bool checkCollision(ID id1, ID id2) const noexcept;
        bool checkCollision(Entity e1, Entity e2) const noexcept;
        void applyPhysics(float deltaTime) noexcept;
        void getCollisions() noexcept;
        void resolveCollisions() noexcept;
    };

}