module;

export module entity;
import component;
import types;

namespace mars {
    export using ID = u64;
    export constexpr u64 maxEntities = 5000;

    export class Entity {
        ID mID;
        Signature mSignature; 
        public:
        Entity(ID id, Signature sig) noexcept;
        ID getID() const noexcept;
        Signature getSignature() const noexcept;
    };
}
