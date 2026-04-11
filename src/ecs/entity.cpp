module;

#include <initializer_list>

module entity;

namespace mars {

    constexpr SignatureT componentToBit(Component c) noexcept {
        return 1 << static_cast<SignatureT>(c);
    }

    void implyComponents(Component c, SignatureT& bits) noexcept {
        //If we already have this component (and thus all of its implied components), exit
        if(bits & componentToBit(c)) return;
        bits |= componentToBit(c);
        switch(c) {
        case Component::physics:
            implyComponents(Component::transform, bits); 
            break;
        case Component::draw:
            implyComponents(Component::transform, bits);
            break;
        case Component::dynamics:
            implyComponents(Component::physics, bits); 
            implyComponents(Component::collide, bits);
            break;
        case Component::ledgeGrab:
            implyComponents(Component::dynamics, bits);
            break;
        default: break;
        }
    }

    Signature::Signature(std::initializer_list<Component> comps) noexcept : mBits(0) {
        for(Component c : comps) {
            implyComponents(c, mBits);
        }
    }

    bool Signature::has(const std::initializer_list<Component> comps) const noexcept {
        SignatureT sig = 0;
        for(Component c : comps) {
            sig |= componentToBit(c);
        }
        return mBits & sig;
    }

    bool Signature::has(Component comp) const noexcept {
        return mBits & componentToBit(comp);
    }

    SignatureT Signature::getBits() const noexcept {
        return mBits;
    }

    bool Signature::operator==(std::initializer_list<Component> comps) const noexcept {
        return has(comps);
    }

    bool Signature::operator!=(std::initializer_list<Component> comps) const noexcept {
        return !has(comps);
    }

    ID Entity::id() const noexcept {
        return mID;
    }

    Signature Entity::signature() const noexcept {
        return mSignature;
    }

    bool Entity::has(const std::initializer_list<Component> comps) const noexcept {
        return mSignature.has(comps);
    }

    bool Entity::has(Component comp) const noexcept {
        return mSignature.has(comp);
    }

    bool operator==(Entity lhs, Entity rhs) noexcept {
        return lhs.mID == rhs.mID;
    }

    bool operator!=(Entity lhs, Entity rhs) noexcept {
        return lhs.mID != rhs.mID;
    }
}
