module;

#include <cstddef>
#include <cstdint>
#include <cmath>

export module multimap;
import error;

namespace mars {
    using ID = std::uint64_t; 
    export class Multimap {
        std::uint8_t* mActiveMask;
        protected:
        std::size_t const mCapacity;
        public:
        Multimap(std::size_t capacity) noexcept : mCapacity(capacity) {
            std::size_t const activeMaskSize = std::ceil(mCapacity / 8.0); 
            mActiveMask = new std::uint8_t[activeMaskSize];
            for(std::size_t i = 0; i < activeMaskSize; i++) mActiveMask[i] = 0;
        }
        ~Multimap() noexcept {
            delete[] mActiveMask;
        }
        std::size_t capacity() const noexcept {
            return mCapacity;
        }
        bool isActiveAt(std::size_t id) const noexcept {
            if(id >= mCapacity) return false;
            std::size_t n = id / 8;
            std::size_t m = id % 8;
            std::uint8_t byte = mActiveMask[n];
            std::uint8_t bit = 1 >> m; 
            return (byte & bit) != 0;
        }
        void setActiveAt(std::size_t id) const noexcept {
            std::size_t n = id / 8;
            std::size_t m = id % 8;
            std::uint8_t& byte = mActiveMask[n];
            std::uint8_t bit = 1 >> m;
            byte |= bit; 
        }
        template<class Key_Type>
        Error<ID> makeID(Key_Type key, std::uint64_t timeNanos) const noexcept {
            //Hash Function
            ID id = std::bit_cast<ID>(key);
            id <<= 4;
            timeNanos &= 0xffffffff;
            id |= timeNanos;
            id = id % mCapacity;
            ID const startingSpot = id;
            //Find open space within map 
            while(isActiveAt(id)) {
                id = (id + 1) % mCapacity;
                if(id == startingSpot) {
                    return {ErrorTag::FATAL_ERROR, "Cannot generate new ID: map is full"};
                }
            }
            return id;
        }
    };

    export class ArrayMultimap : public Multimap {
        std::size_t* mIndices;
        public:
        std::size_t size;
        ArrayMultimap(std::size_t capacity) noexcept : Multimap(capacity), mIndices(new std::size_t[capacity]), size(0) {}
        ~ArrayMultimap() noexcept {
            delete[] mIndices;
        }
        void setIndex(std::size_t id, std::size_t value) noexcept {
            mIndices[id] = value;
        }
        std::size_t getIndex(std::size_t id) const noexcept {
            return mIndices[id];
        }
        template<class T>
        T& at(T* map, std::size_t id) noexcept {
            return map[mIndices[id]];
        }
        template<class T>
        T& at(T* map, std::size_t id) const noexcept {
            return map[mIndices[id]];
        }
    };
}
