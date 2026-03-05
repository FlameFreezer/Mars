module;

module entity;

namespace mars {
    Entity::Entity(ID id, Signature sig) noexcept : mID(id), mSignature(sig) {}
    ID Entity::getID() const noexcept {
        return mID;
    }
    Signature Entity::getSignature() const noexcept {
        return mSignature;
    }
}
