module;

#include <utility>

export module component;
export import components;
export import component_system;
import types;
import error;
import entity;

#define DEFINE_COMPONENT_GETTER(component) const GetComp<Component::component>::Type& component(Entity e) const noexcept{\
    return system<Component::component>()[e];\
}\
GetComp<Component::component>::Type& component(Entity e) noexcept {\
    return system<Component::component>()[e];\
}\
const GetComp<Component::component>::Type& component(ID id) const noexcept{\
    return system<Component::component>()[id];\
}\
GetComp<Component::component>::Type& component(ID id) noexcept {\
    return system<Component::component>()[id];\
}

namespace mars {
    export class ComponentManager {
        ComponentSystemParent* mSystems[numComponents];
        Signature* mSignatures = new Signature[maxEntities];
        public:
        ComponentManager() noexcept;
        ~ComponentManager() noexcept;
        ComponentManager(const ComponentManager&) = delete;
        ComponentManager(ComponentManager&&) = delete;
        void reserveFor(ID id, Signature s) noexcept;
        void freeFor(ID id) noexcept;
        Signature getSignature(ID id) const noexcept;
        template<Component c>
        ComponentSystem<typename GetComp<c>::Type>& system() noexcept {
            return *reinterpret_cast<ComponentSystem<typename GetComp<c>::Type>*>(mSystems[std::to_underlying(c)]);
        }
        template<Component c>
        const ComponentSystem<typename GetComp<c>::Type>& system() const noexcept {
            return *reinterpret_cast<const ComponentSystem<typename GetComp<c>::Type>*>(mSystems[std::to_underlying(c)]);
        }
        Position position(Entity e) noexcept;
        DEFINE_COMPONENT_GETTER(physics)
        DEFINE_COMPONENT_GETTER(collide)
        DEFINE_COMPONENT_GETTER(dynamics)
        DEFINE_COMPONENT_GETTER(ledgeGrab)
    };
}
