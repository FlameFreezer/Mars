module;

#include <format>

#include <vulkan/vulkan.h>

export module component_system;
import entity;
import component;
import types;
import error;
import gpubuffer;

namespace mars {
    //Abstract parent class for component systems - to automate initialization and deinitialization
    // within the EntityManager
    export class ComponentSystemParent {
        protected:
        ID mIDs[maxEntities];
        u64 mIndices[maxEntities];
        u64 mSize;
        void swapErase(ID id) noexcept {
            //Swap the index of the data at id for the data at end
            mIndices[mIDs[mSize]] = mIndices[id];
            //Decrement size
            --mSize;
        }
        public:
        ComponentSystemParent() noexcept : mSize(0) {}
        virtual ~ComponentSystemParent() noexcept {}
        u64 size() const noexcept {
            return mSize;
        }
        //Gets the internal index associated with the ID
        u64 index(ID id) const noexcept {
            return mIndices[id];
        }
        //Gets the array of IDs
        ID* getIDs() noexcept {
            return mIDs;
        }
        const ID* getIDs() const noexcept {
            return mIDs;
        }
        //Writes the ID to correct spaces of constituent arrays, then increments size
        u64 reserve(ID id) noexcept {
            mIndices[id] = mSize;
            mIDs[mSize] = id;
            return mSize++;
        }
        //Abstract function which should swap the data at id with the data at the end, then calls
        // swapErase
        virtual void erase(ID id) noexcept = 0;

    };
    export template<typename C>
    class ComponentSystem : public ComponentSystemParent {
        C mData[maxEntities];
        public:
        //Safety checked random access of data
        Error<const C*> at(ID id) const noexcept {
            if(id >= maxEntities) {
                return {ErrorTag::FATAL_ERROR, std::format("Invalid ID {} passed to function \"at\"", id)};
            }
            return &mData[mIndices[id]];
        }
        Error<C*> at(ID id) noexcept {
            if(id >= maxEntities) {
                return {ErrorTag::FATAL_ERROR, std::format("Invalid ID {} passed to function \"at\"", id)};
            }
            return &mData[mIndices[id]];
        }
        //Unchecked random access of data
        const C& operator[](ID id) const noexcept {
            return mData[mIndices[id]];
        }
        C& operator[](ID id) noexcept {
            return mData[mIndices[id]];
        }
        //Getter for internal data array
        C* data() noexcept {
            return mData;
        }
        const C* data() const noexcept {
            return mData;
        }
        void insert(ID id, const C& comp) noexcept {
            u64 index = reserve(id);
            mData[index] = comp;
        }
        void erase(ID id) noexcept {
            //Swap the data at id for the data at the end
            mData[mIndices[id]] = mData[mSize];
            swapErase(id);
        }
    };
}
