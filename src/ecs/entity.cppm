module;

#include <initializer_list>

export module entity;
import component;
import types;

namespace mars {
    export constexpr u64 maxEntities = 500;
    export constexpr ID nullID = -1;

    export using SignatureT = u32;
    export class Signature {
        SignatureT mBits;
        public:
        constexpr Signature() noexcept : mBits(0) {}
        explicit Signature(std::initializer_list<Component> comps) noexcept;
        constexpr Signature(const Signature& s) noexcept : mBits(s.mBits) {}
        bool has(const std::initializer_list<Component> comps) const noexcept;
        bool has(Component comp) const noexcept;
        bool operator==(std::initializer_list<Component> comps) const noexcept;
        bool operator!=(std::initializer_list<Component> comps) const noexcept;
        SignatureT getBits() const noexcept;
    };

    export class Entity {
        ID mID;
        Signature mSignature; 
        public:
        Entity() = default;
        constexpr Entity(ID id, const Signature& sig) noexcept : mID(id), mSignature(sig) {}
        ID id() const noexcept;
        Signature signature() const noexcept;
        friend bool operator==(Entity lhs, Entity rhs) noexcept;
        friend bool operator!=(Entity lhs, Entity rhs) noexcept;
    };
}
