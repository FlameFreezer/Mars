module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

export module multimap;

namespace mars {
    export using ID = std::uint64_t; 
    export class ArrayMultimap {
        static const std::size_t initialMapCapacity = 50;
        static const std::size_t growthFactor = 2;
        ID* mIndices;
        std::size_t mSize;
        std::size_t mCapacity;

        protected:
        void realloc() noexcept {
            ID* data = new ID[mCapacity * growthFactor];
            std::memcpy(data, mIndices, mCapacity);
            mCapacity *= growthFactor;
            delete[] mIndices;
            mIndices = data;
        }

        public:
        static const std::size_t npos = std::numeric_limits<std::size_t>::max();
        ArrayMultimap() noexcept : mIndices(new ID[initialMapCapacity]), mSize(0), mCapacity(initialMapCapacity) {}
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
