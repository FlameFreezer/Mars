#pragma once

#include "mars_types.h"
#include "mars_signature.h"

namespace mars {
    class Entity {
        ID mID = nullID;
        Signature mSignature{};
        public:
        constexpr Entity() = default;
        constexpr Entity(ID id, Signature sig) noexcept : mID(id), mSignature(sig) {}
        ID id() const noexcept;
        Signature signature() const noexcept;
        friend bool operator==(Entity lhs, Entity rhs) noexcept;
        friend bool operator!=(Entity lhs, Entity rhs) noexcept;
    };

    constexpr inline Entity nullEntity{};
}
