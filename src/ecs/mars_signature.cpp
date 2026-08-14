#include <ecs/mars_signature.h>

#include <utility>

namespace mars {

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