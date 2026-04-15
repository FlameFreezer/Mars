module;

export module physics_manager;
import types;
import entity;
import component;
import position;

namespace mars {
    export class PhysicsManager {
        Position position(ID id) noexcept;
        PhysicsManager() = default;
        PhysicsManager(const PhysicsManager& other) = delete;
        PhysicsManager(PhysicsManager&& other) = delete;
        public:
        static PhysicsManager& get() noexcept;
        bool checkCollision(ID id1, ID id2) const noexcept;
        bool checkCollision(Entity e1, Entity e2) const noexcept;
        void applyPhysics(float deltaTime) noexcept;
        void getCollisions() noexcept;
        void resolveCollisions() noexcept;
    };

}
