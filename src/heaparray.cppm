module;

#include <cstddef>
#include <format>

export module heap_array;
import error;

namespace mars {
    //Essentially implements the "fat pointer" from zig
    export template <class T>
    class HeapArray {
        protected:
        T* mPtr;
        std::size_t mSize;
        HeapArray(T* inPtr, std::size_t inSize) noexcept : mPtr(inPtr), mSize(inSize) {}
        public:
        HeapArray() noexcept : mPtr(nullptr), mSize(0) {}
        HeapArray(std::size_t inSize) noexcept : mPtr(new T[inSize]), mSize(inSize) {}
        HeapArray(std::size_t inSize, T initial) noexcept : mPtr(new T[inSize]), mSize(inSize) {
            for(std::size_t i = 0; i < mSize; i++) mPtr[i] = initial;
        }
        virtual ~HeapArray() noexcept {
            delete[] mPtr;
            mSize = 0;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        T& operator[](std::size_t index) const noexcept {
            return mPtr[index];
        }
        Error<T&> at(std::size_t index) const noexcept {
            if(index < mSize) return mPtr[index];
            else return {ErrorTag::FATAL_ERROR, std::format("Index out of bounds: {} >= {}", index, mSize)};
        }
        HeapArray<T>& operator=(HeapArray<T>&& rhs) noexcept {
            if(this != &rhs) {
                mPtr = rhs.mPtr;
                mSize = rhs.mSize;
                rhs.mPtr = 0;
                rhs.mSize = 0;
            }
            return *this;
        }
        HeapArray<T>& operator=(HeapArray<T> const& rhs) noexcept {
            if(this != &rhs) {
                delete[] mPtr;
                mPtr = new T[rhs.mSize];
                for(std::size_t i = 0; i < rhs.mSize; i++) mPtr[i] = rhs.mPtr[i];
                mSize = rhs.mSize;
            }
            return *this;
        }
        T* data() const noexcept {
            return mPtr;
        }

        virtual void clear() noexcept {
            delete[] mPtr;
            mSize = 0;
        }

        class Iterator {
            private:
            HeapArray<T> const& mArray;
            std::size_t mIndex;
            public:
            friend class HeapArray<T>;
            Iterator() = delete;
            Iterator(HeapArray<T> const& arr, std::size_t index) noexcept : mArray(arr), mIndex(index) {}
            Iterator operator++(int) noexcept {
                return Iterator(mArray, mIndex++);
            }
            Iterator& operator++() noexcept {
                mIndex++;
                return *this;
            }
            bool operator==(Iterator const& rhs) const noexcept {
                return mIndex == rhs.mIndex and &mArray == &rhs.mArray;
            }
            bool operator!=(Iterator const& rhs) const noexcept {
                return mIndex != rhs.mIndex or &mArray != &rhs.mArray;
            }
            T& operator*() noexcept {
                return mArray.mPtr[mIndex];
            }
            T* operator->() noexcept {
                return &mArray.mPtr[mIndex];
            }
        };
        Iterator begin() const noexcept {
            return Iterator(*this, 0);
        }
        Iterator end() const noexcept {
            return Iterator(*this, mSize);
        }
    };

    //A non-owning reference to a range within a heap-allocated array
    export template <class T>
    class Slice : public HeapArray<T> {
        public:
        Slice() noexcept {}
        Slice(T* ptr, std::size_t size) noexcept : HeapArray<T>(ptr, size) {}
        Slice(HeapArray<T> const& array) noexcept : HeapArray<T>(array.data(), array.size()) {}
        Slice(HeapArray<T> const& array, std::size_t start) noexcept : HeapArray<T>(array.data() + start, array.size() - start) {
            if(start >= array.size()) {
                this->mSize = 0;
                this->mPtr = nullptr;
            }
        }
        Slice(HeapArray<T> const& array, std::size_t start, std::size_t count) noexcept : HeapArray<T>(array.data() + start, count) {
            if(start + count >= array.size() or start >= array.size()) {
                this->mSize = 0;
                this->mPtr = nullptr;
            }
        }
        Slice(HeapArray<T>&& array) noexcept = delete;
        Slice(Slice<T> const& other) noexcept : HeapArray<T>(other.mPtr, other.mSize) {}
        Slice(Slice<T>&& other) noexcept : HeapArray<T>(other.mPtr, other.mSize) {
            other.mPtr = nullptr;
            other.mSize = 0;
        }
        ~Slice() noexcept override {
            this->mPtr = nullptr;
            this->mSize = 0;
        }
        //Override behavior of copy assignment operator so that slices do not construct new arrays
        Slice<T>& operator=(HeapArray<T> const& rhs) noexcept {
            if(this != &rhs) {
                this->mPtr = rhs.data();
                this->mSize = rhs.size();
            }
            return *this;
        }
        //A slice can never be assigned to a temporary array
        Slice<T>& operator=(HeapArray<T>&& rhs) = delete;
        Slice<T>& operator=(Slice<T> const& rhs) noexcept {
            if(this != &rhs) {
                this->mPtr = rhs.mPtr;
                this->mSize = rhs.mSize;
            }
            return *this;
        }
        Slice<T>& operator=(Slice<T>&& rhs) noexcept {
            if(this != &rhs) {
                this->mPtr = rhs.mPtr;
                this->mSize = rhs.mSize;
                rhs.mPtr = nullptr;
                rhs.mSize = 0;
            }
            return *this;
        }
        void clear() noexcept override {
            this->mPtr = nullptr;
            this->mSize = 0;
        }
    };
}