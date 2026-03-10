module;

#include <initializer_list>

export module entity;
import component;
import types;

namespace mars {
    export constexpr u64 maxEntities = 5000;

    export class Signature {
        u32 mBits;
        public:
        explicit Signature(std::initializer_list<Components> comps) noexcept;
        Signature(const Signature& s) noexcept;
        bool has(std::initializer_list<Components> comps) const noexcept;
        bool has(Components comp) const noexcept;
        bool operator==(std::initializer_list<Components> comps) const noexcept;
        bool operator!=(std::initializer_list<Components> comps) const noexcept;
        u32 getBits() const noexcept;
    };

    export class Entity {
        ID mID;
        Signature mSignature; 
        public:
        Entity(ID id, Signature sig) noexcept;
        ID id() const noexcept;
        Signature signature() const noexcept;
    };
}
