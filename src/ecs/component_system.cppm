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
            //Invalidate the ID for the removed element
            mIDs[mIndices[id]] = nullID;
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
        //Tells whether the ID has this component
        bool has(ID id) noexcept {
            return mIDs[mIndices[id]] == id;
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
                return {ErrorTag::fatalError, std::format("Invalid ID {} passed to function \"at\"", id)};
            }
            return &mData[mIndices[id]];
        }
        Error<C*> at(ID id) noexcept {
            if(id >= maxEntities) {
                return {ErrorTag::fatalError, std::format("Invalid ID {} passed to function \"at\"", id)};
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
        const C& operator[](Entity e) const noexcept {
            return mData[mIndices[e.id()]];
        }
        C& operator[](Entity e) noexcept {
            return mData[mIndices[e.id()]];
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
            //Refuse to erase the null entity
            if(id == nullID) return;
            //Swap the data at id for the data at the end
            mData[mIndices[id]] = mData[mSize];
            swapErase(id);
        }
        void swap(ID id1, ID id2) noexcept {
            //Don't swap the null entity or an entity with itself
            if(id1 == nullID or id2 == nullID or id1 == id2) return;
            const u64 index1 = mIndices[id1];
            const u64 index2 = mIndices[id2];

            const C tmpData = mData[index1];
            mData[index1] = mData[index2];
            mData[index2] = tmpData;

            const ID tmpID = mIDs[index1];
            mIDs[index1] = mIDs[index2];
            mIDs[index2] = tmpID;

            mIndices[id1] = index2;
            mIndices[id2] = index1;
        }
        struct Pair {
            C& data;
            ID id;
        };
        struct CPair {
            const C& data;
            ID id;
        };
        class Iterator {
            u64 mIndex;
            ComponentSystem<C>* mParent;
            Iterator(u64 index, ComponentSystem<C>* parent) noexcept : mIndex(index), mParent(parent) {}
            public:
            Iterator() = delete;
            friend class ComponentSystem<C>;
            Iterator(const Iterator& other) noexcept : mIndex(other.mIndex), mParent(other.mParent) {}
            Pair operator*() noexcept {
                return {mParent->mData[mIndex], mParent->mIDs[mIndex]};
            }
            C* operator->() noexcept {
                return &mParent->mData[mIndex];
            }
            Iterator& operator++(int) noexcept {
                ++mIndex;
                return *this;
            }
            Iterator operator++() noexcept {
                Iterator res = *this;
                mIndex++;
                return res;
            }
            bool operator==(const Iterator& other) noexcept {
                return mParent == other.mParent and mIndex == other.mIndex;
            }
            bool operator!=(const Iterator& other) noexcept {
                return mParent != other.mParent or mIndex != other.mIndex;
            }
            Iterator& operator=(const Iterator& rhs) noexcept {
                mIndex = rhs.mIndex;
                mParent = rhs.mParent;
            }
        };
        class CIterator {
            u64 mIndex;
            const ComponentSystem<C>& mParent;
            CIterator(u64 index, const ComponentSystem<C>& parent) noexcept : mIndex(index), mParent(parent) {}
            public:
            CIterator() = delete;
            friend class ComponentSystem<C>;
            CIterator(const CIterator& other) noexcept : mIndex(other.mIndex), mParent(other.mParent) {}
            CPair operator*() const noexcept {
                return {mParent.mData[mIndex], mParent.mIDs[mIndex]};
            }
            const C* operator->() const noexcept {
                return &mParent.mData[mIndex];
            }
            CIterator& operator++(int) noexcept {
                ++mIndex;
                return *this;
            }
            CIterator operator++() noexcept {
                CIterator res = *this;
                mIndex++;
                return res;
            }
            bool operator==(const CIterator& other) noexcept {
                return &mParent == &other.mParent and mIndex == other.mIndex;
            }
            bool operator!=(const CIterator& other) noexcept {
                return &mParent != &other.mParent or mIndex != other.mIndex;
            }
        };

        //Iterators start at 1 to skip the null entity
        Iterator begin() noexcept {
            return {1, this};
        }
        Iterator end() noexcept {
            return {mSize, this};
        }

        CIterator begin() const noexcept {
            return {1, *this};
        }
        CIterator end() const noexcept {
            return {mSize, *this};
        }
    };
}