#pragma once

#include <initializer_list>
#include <limits>

#include "mars_types.h"
#include "mars_components.h"

namespace mars {
    using SignatureT = u32;
    class Signature {
		//Null Signature has every component such that storage in component systems is always reserved
        static constexpr SignatureT nullSignatureBits = std::numeric_limits<SignatureT>::max();
        SignatureT mBits = nullSignatureBits;
        public:
        constexpr Signature() noexcept = default;
        Signature(std::initializer_list<Component> comps) noexcept;
        bool has(std::initializer_list<Component> comps) const noexcept;
        bool has(Component comp) const noexcept;
        bool operator==(std::initializer_list<Component> comps) const noexcept;
        bool operator!=(std::initializer_list<Component> comps) const noexcept;
        SignatureT getBits() const noexcept;
    };
    constexpr inline Signature nullSignature{};
}
