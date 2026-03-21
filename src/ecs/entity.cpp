module;

#include <initializer_list>

module entity;

namespace mars {

    constexpr SignatureT componentToBit(Component c) noexcept {
        return 1 << static_cast<SignatureT>(c);
    }

    SignatureT implyComponents(Component c) noexcept {
        SignatureT result = componentToBit(c);
        switch(c) {
        case Component::PHYSICS:
            result |= implyComponents(Component::TRANSFORM); 
            break;
        case Component::DRAW:
            result |= implyComponents(Component::TRANSFORM);
            break;
        case Component::DYNAMICS:
            result |= implyComponents(Component::PHYSICS) | implyComponents(Component::COLLIDE);
            break;
        default: break;
        }
        return result;
    }

    Signature::Signature(std::initializer_list<Component> comps) noexcept : mBits(0) {
        for(Component c : comps) {
            mBits |= implyComponents(c);
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

    bool operator==(Entity lhs, Entity rhs) noexcept {
        return lhs.mID == rhs.mID;
    }

    bool operator!=(Entity lhs, Entity rhs) noexcept {
        return lhs.mID != rhs.mID;
    }
}
