module;

#include <initializer_list>

module entity;

namespace mars {
    Signature::Signature(std::initializer_list<Component> comps) noexcept : mBits(0) {
        u32 currentBit;
        for(Component c : comps) {
            currentBit = 1 << static_cast<u32>(c);
            mBits |= currentBit;
        }
    }

    bool Signature::has(std::initializer_list<Component> comps) const noexcept {
        u32 currentBit;
        u32 sig = 0;
        for(Component c : comps) {
            currentBit = 1 << static_cast<u32>(c);
            sig |= currentBit;
        }
        return mBits & sig;
    }

    bool Signature::has(Component comp) const noexcept {
        u32 bit = 1 << static_cast<u32>(comp);
        return mBits & bit;
    }

    u32 Signature::getBits() const noexcept {
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
