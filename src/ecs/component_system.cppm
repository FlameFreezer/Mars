module;

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

export module component_system;
import components;
import entity;
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
            return mIDs[index(id)] == id;
        }
        //Abstract function which should swap the data at id with the data at the end, then calls
        // swapErase
        virtual void erase(ID id) noexcept = 0;

    };
    export template<typename C>
    class ComponentSystem : public ComponentSystemParent {
        C mData[maxEntities];
        public:
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
    export template<>
    class ComponentSystem<Draw> : public ComponentSystemParent {
        glm::vec2 mPositions[maxEntities];
        glm::vec2 mScales[maxEntities];
        float mAngles[maxEntities];
        float mZLayers[maxEntities];
        ID mMeshIDs[maxEntities];
        ID mTextureIDs[maxEntities];
        public:
        glm::vec2& position(ID id) noexcept {
            return mPositions[index(id)];
        }
        const glm::vec2& position(ID id) const noexcept {
            return mPositions[index(id)];
        }
        glm::vec2& scale(ID id) noexcept {
            return mScales[index(id)];
        }
        const glm::vec2& scale(ID id) const noexcept {
            return mScales[index(id)];
        }
        float& angle(ID id) noexcept {
            return mAngles[index(id)];
        }
        const float& angle(ID id) const noexcept {
            return mAngles[index(id)];
        }
        float& zLayer(ID id) noexcept {
            return mZLayers[index(id)];
        }
        const float& zLayer(ID id) const noexcept {
            return mZLayers[index(id)];
        }
        ID& meshID(ID id) noexcept {
            return mMeshIDs[index(id)];
        }
        const ID& meshID(ID id) const noexcept {
            return mMeshIDs[index(id)];
        } 
        ID& textureID(ID id) noexcept {
            return mTextureIDs[index(id)];
        }
        const ID& textureID(ID id) const noexcept {
            return mTextureIDs[index(id)];
        }
        glm::vec2* positions() noexcept {
            return mPositions;
        }
        const glm::vec2* positions() const noexcept {
            return mPositions;
        }
        glm::vec2* scales() noexcept {
            return mScales;
        }
        const glm::vec2* scales() const noexcept {
            return mScales;
        }
        float* zLayers() noexcept {
            return mZLayers;
        }
        const float* zLayers() const noexcept {
            return mZLayers;
        }
        ID* meshIDs() noexcept {
            return mMeshIDs;
        }
        const ID* meshIDs() const noexcept {
            return mMeshIDs;
        }
        ID* textureIDs() noexcept {
            return mTextureIDs;
        }
        const ID* textureIDs() const noexcept {
            return mTextureIDs;
        }
        void erase(ID id) noexcept {
            //Refuse to erase the null entity
            if(id == nullID) return;
            //Swap the data at id for the data at the end
            const u64 index = mIndices[id];
            mPositions[index] = mPositions[mSize];
            mScales[index] = mScales[mSize];
            mAngles[index] = mAngles[mSize];
            mZLayers[index] = mZLayers[mSize];
            mMeshIDs[index] = mMeshIDs[mSize];
            mTextureIDs[index] = mTextureIDs[mSize]; 
            swapErase(id);
        }
        class PositionIterator {
            u64 mIndex;
            ComponentSystem<Draw>* mParent;
            PositionIterator(u64 index, ComponentSystem<Draw>* parent) noexcept : mIndex(index), mParent(parent) {}
            public:
            friend class ComponentSystem<Draw>;
            PositionIterator() noexcept = default;
            PositionIterator(const PositionIterator& other) noexcept : mIndex(other.mIndex), mParent(other.mParent) {}
            glm::vec2& operator*() noexcept {
                return mParent->mPositions[mIndex];
            }
            PositionIterator& operator++(int) noexcept {
                mIndex++;
                return *this;
            }
            PositionIterator operator++() noexcept {
                return {mIndex++, mParent};
            }
            bool operator==(PositionIterator rhs) const noexcept {
                return mParent == rhs.mParent and mIndex == rhs.mIndex;
            }
            bool operator!=(PositionIterator rhs) const noexcept {
                return mParent != rhs.mParent or mIndex != rhs.mIndex;
            }
        };
        PositionIterator begin() noexcept {
            //Start at 1 to skip the null entity
            return {1, this};
        }
        PositionIterator end() noexcept {
            return {mSize, this};
        }
    };
}
