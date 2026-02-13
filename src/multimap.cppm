module;

#include <cstddef>
#include <cstdint>
#include <cstring>

export module multimap;

#define INITIAL_MAP_CAPACITY 50
#define GROWTH_FACTOR 2

namespace mars {
    export using ID = std::uint64_t; 
    export class ArrayMultimap {
        ID* mIndices;
        std::size_t mSize;
        std::size_t mCapacity;

        protected:
        void realloc() noexcept {
            ID* data = new ID[mCapacity * GROWTH_FACTOR];
            std::memcpy(data, mIndices, mCapacity);
            mCapacity *= GROWTH_FACTOR;
            delete[] mIndices;
            mIndices = data;
        }

        void setIndexForID(ID id, ID index) noexcept {
           mIndices[id] = index;
            --mSize;
        }

        public:
        ArrayMultimap() noexcept : mIndices(new ID[INITIAL_MAP_CAPACITY]), mSize(0), mCapacity(INITIAL_MAP_CAPACITY) {}
        ArrayMultimap(std::size_t inCapacity) noexcept : mIndices(new ID[inCapacity]), mSize(0), mCapacity(inCapacity) {}
        ~ArrayMultimap() noexcept {
            delete[] mIndices;
            mSize = 0;
            mCapacity = 0;
            mIndices = nullptr;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        std::size_t capacity() const noexcept {
            return mCapacity;
        }
        ID getIndex(ID key) const noexcept {
            return mIndices[key];
        }
        ID append() noexcept {
            mIndices[mSize] = mSize;
            return mSize++;
        }
        template<typename T>
        T& at(T* map, ID id) noexcept {
            return map[mIndices[id]];
        }
        template<typename T>
        T& at(T* map, ID id) const noexcept {
            return map[mIndices[id]];
        }
    };
}
