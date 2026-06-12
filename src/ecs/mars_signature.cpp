#include "mars_signature.h"

#include <utility>

#include "mars_components.h"

namespace mars {
    static constexpr SignatureT componentToBit(Component c) noexcept {
        return 1 << std::to_underlying(c);
    }

    static void implyComponents(Component c, SignatureT& bits) noexcept {
        //If we already have this component (and thus all of its implied components), exit
        if(bits & componentToBit(c)) return;
        bits |= componentToBit(c);
        switch(c) {
        case Component::dynamics:
            implyComponents(Component::physics, bits); 
            implyComponents(Component::collide, bits);
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

}