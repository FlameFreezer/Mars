module;

#include <format>

export module component_system;
import entity;
import component;
import types;
import error;

namespace mars {
    export class ComponentSystemParent {
        protected:
        ID mIDs[maxEntities];
        u64 mIndices[maxEntities];
        u64 mSize;
        public:
        ComponentSystemParent() noexcept : mSize(0) {}
        virtual ~ComponentSystemParent() noexcept {}
        u64 size() const noexcept {
            return mSize;
        }
        u64 reserve(ID id) noexcept {
            mIndices[id] = mSize;
            mIDs[mSize] = id;
            return mSize++;
        }
        virtual void erase(ID id) noexcept = 0;
    };
    export template<typename C>
    class ComponentSystem : public ComponentSystemParent {
        C mData[maxEntities];
        public:
        ComponentSystem() noexcept {}
        Error<const C&> at(ID id) const noexcept {
            if(id >= maxEntities) {
                return fatal(std::format("Invalid ID {} passed to function \"at\"", id));
            }
            return mData[mIndices[id]];
        }
        Error<C&> at(ID id) noexcept {
            if(id >= maxEntities) {
                return fatal(std::format("Invalid ID {} passed to function \"at\"", id));
            }
            return mData[mIndices[id]];
        }
        const C& operator[](ID id) const noexcept {
            return mData[mIndices[id]];
        }
        C& operator[](ID id) noexcept {
            return mData[mIndices[id]];
        }
        void insert(ID id, const C& comp) noexcept {
            u64 index = reserve(id);
            mData[index] = comp;
        }
        void erase(ID id) noexcept {
            //Swap the data at id for the data at the end
            mData[mIndices[id]] = mData[mSize];
            //Swap the index of the data at id for the data at end
            mIndices[mIDs[mSize]] = mIndices[id];
            --mSize;
        }
    };
}
