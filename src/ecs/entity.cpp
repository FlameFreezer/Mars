module;

#include <initializer_list>

module entity;

namespace mars {
    Signature::Signature(std::initializer_list<Components> comps) noexcept : mBits(0) {
        u32 currentBit;
        for(Components c : comps) {
            currentBit = 1 << static_cast<u32>(c);
            mBits |= currentBit;
        }
    }

    Signature::Signature(const Signature& s) noexcept : mBits(s.mBits) {}
    bool Signature::has(std::initializer_list<Components> comps) const noexcept {
        u32 currentBit;
        u32 sig = 0;
        for(Components c : comps) {
            currentBit = 1 << static_cast<u32>(c);
            sig |= currentBit;
        }
        return mBits & sig;
    }

    bool Signature::has(Components comp) const noexcept {
        u32 bit = 1 << static_cast<u32>(comp);
        return mBits & bit;
    }

    u32 Signature::getBits() const noexcept {
        return mBits;
    }

    bool Signature::operator==(std::initializer_list<Components> comps) const noexcept {
        return has(comps);
    }

    bool Signature::operator!=(std::initializer_list<Components> comps) const noexcept {
        return !has(comps);
    }

    Entity::Entity(ID id, Signature sig) noexcept : mID(id), mSignature(sig) {}

    ID Entity::id() const noexcept {
        return mID;
    }

    Signature Entity::signature() const noexcept {
        return mSignature;
    }
}
