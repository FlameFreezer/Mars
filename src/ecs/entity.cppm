module;

#include <initializer_list>
#include <limits>

export module entity;
import component;
import types;

namespace mars {
    export constexpr u64 maxEntities = 512;

    export using SignatureT = u32;
    //Null Signature has every component such that storage in component systems is always reserved
    export constexpr SignatureT nullSignature = std::numeric_limits<SignatureT>::max();
    export class Signature {
        SignatureT mBits = nullSignature;
        public:
        constexpr Signature() noexcept = default;
        Signature(std::initializer_list<Component> comps) noexcept;
        constexpr Signature(const Signature& s) noexcept : mBits(s.mBits) {}
        bool has(const std::initializer_list<Component> comps) const noexcept;
        bool has(Component comp) const noexcept;
        bool operator==(std::initializer_list<Component> comps) const noexcept;
        bool operator!=(std::initializer_list<Component> comps) const noexcept;
        SignatureT getBits() const noexcept;
    };

    export class Entity {
        ID mID = nullID;
        Signature mSignature; 
        public:
        constexpr Entity() = default;
        constexpr Entity(ID id, const Signature& sig) noexcept : mID(id), mSignature(sig) {}
        ID id() const noexcept;
        Signature signature() const noexcept;
        friend bool operator==(Entity lhs, Entity rhs) noexcept;
        friend bool operator!=(Entity lhs, Entity rhs) noexcept;
    };

    export constexpr Entity nullEntity;
}
