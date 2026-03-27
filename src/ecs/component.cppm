module;

#include <glm/glm.hpp>

export module component;
export import components;
export import component_system;
import types;
import error;
import entity;
import position;

#define DEFINE_COMPONENT_GETTER(component) const GetComp<Component::component>::Type& component(Entity e) const noexcept{\
    return system<Component::component>()[e];\
}\
GetComp<Component::component>::Type& component(Entity e) noexcept {\
    return system<Component::component>()[e];\
}\

namespace mars {
    export class ComponentManager {
        ComponentSystemParent* mSystems[numComponents];
        Signature* mSignatures = new Signature[maxEntities];
        public:
        ComponentManager() noexcept;
        ~ComponentManager() noexcept;
        void reserveFor(ID id, Signature s) noexcept;
        void freeFor(ID id) noexcept;
        Signature getSignature(ID id) const noexcept;
        template<Component c>
        ComponentSystem<typename GetComp<c>::Type>& system() noexcept {
            return *reinterpret_cast<ComponentSystem<typename GetComp<c>::Type>*>(mSystems[static_cast<ComponentT>(c)]);
        }
        template<Component c>
        const ComponentSystem<typename GetComp<c>::Type>& system() const noexcept {
            return *reinterpret_cast<const ComponentSystem<typename GetComp<c>::Type>*>(mSystems[static_cast<ComponentT>(c)]);
        }
        Position position(Entity e) noexcept;
        DEFINE_COMPONENT_GETTER(transform)
        DEFINE_COMPONENT_GETTER(physics)
        DEFINE_COMPONENT_GETTER(draw)
        DEFINE_COMPONENT_GETTER(collide)
        DEFINE_COMPONENT_GETTER(dynamics)
    };
}