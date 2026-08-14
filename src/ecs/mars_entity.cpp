#include <ecs/mars_entity.h>

namespace mars {
    ID Entity::id() const noexcept {
        return mID;
    }

    Signature Entity::signature() const noexcept {
        return mSignature;
    }

    bool operator==(Entity lhs, Entity rhs) noexcept {
        return lhs.mID == rhs.mID;
    }

    bool operator!=(Entity lhs, Entity rhs) noexcept {
        return lhs.mID != rhs.mID;
    }
}
