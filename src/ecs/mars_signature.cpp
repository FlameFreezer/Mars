#include "mars_signature.h"

#include <utility>

#include "mars_components.h"

namespace mars {
    static constexpr SignatureT componentToBit(Component c) noexcept {
        return 1 << std::to_underlying(c);
    }

    Signature::Signature(std::initializer_list<Component> comps) noexcept : mBits(0) {
        for(Component c : comps) {
            if (mBits & componentToBit(c)) continue;
            mBits |= componentToBit(c);
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

}