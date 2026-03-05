module;

#include <cstddef>
#include <format>

export module heap_array;
import error;

namespace mars {
    //Essentially implements the "fat pointer" from zig
    export template <class T>
    class HeapArray {
        private:
        T* mPtr;
        std::size_t mSize;
        public:
        HeapArray() noexcept : mPtr(nullptr), mSize(0) {}
        HeapArray(std::size_t inSize) noexcept : mPtr(new T[inSize]), mSize(inSize) {}
        HeapArray(std::size_t inSize, T initial) noexcept : mPtr(new T[inSize]), mSize(inSize) {
            for(std::size_t i = 0; i < mSize; i++) mPtr[i] = initial;
        }
        ~HeapArray() noexcept {
            delete[] mPtr;
            mSize = 0;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        const T& operator[](std::size_t index) const noexcept {
            return mPtr[index];
        }
        T& operator[](std::size_t index) noexcept {
            return mPtr[index];
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
                if(mPtr == nullptr) delete[] mPtr;
                mPtr = new T[rhs.mSize];
                for(std::size_t i = 0; i < rhs.mSize; i++) mPtr[i] = rhs.mPtr[i];
                mSize = rhs.mSize;
            }
            return *this;
        }
        T* data() noexcept {
            return mPtr;
        }
        const T* data() const noexcept {
            return mPtr;
        }
        void clear() noexcept {
            if(mPtr == nullptr) delete[] mPtr;
            mSize = 0;
        }
        void resize(std::size_t size) noexcept {
            if(mPtr == nullptr) delete[] mPtr;
            mPtr = new T[size];
            mSize = size;
        }
        void init(std::size_t size, const T& value) noexcept {
            resize(size);
            for(std::size_t i = 0; i < size; i++) mPtr[i] = value;
        }

        class Iterator {
            private:
            T* mPtr;
            public:
            Iterator() = delete;
            Iterator(T* inPtr) noexcept : mPtr(inPtr) {}
            Iterator operator++(int) noexcept {
                return Iterator(mPtr++);
            }
            Iterator& operator++() noexcept {
                mPtr++;
                return *this;
            }
            bool operator==(const Iterator& rhs) const noexcept {
                return mPtr == rhs.mPtr;
            }
            bool operator!=(const Iterator& rhs) const noexcept {
                return mPtr != rhs.mPtr;
            }
            T& operator*() noexcept {
                return *mPtr;
            }
            T* operator->() noexcept {
                return mPtr;
            }
        };
        Iterator begin() const noexcept {
            return Iterator(mPtr);
        }
        Iterator end() const noexcept {
            return Iterator(mPtr + mSize);
        }
    };

    //A non-owning reference to a range within a heap-allocated array
    export template <class T>
    class Slice {
        private:
        T* mPtr;
        std::size_t mSize;
        public:
        Slice() noexcept : mPtr(nullptr), mSize(0) {}
        Slice(T* ptr, std::size_t size) noexcept : mPtr(ptr), mSize(size) {}
        Slice(const HeapArray<T>& array) noexcept : mPtr(array.data()), mSize(array.size()) {}
        template<std::size_t size>
        Slice(const std::array<T, size>& array) noexcept : mPtr(array.data()), mSize(size) {}
        static Error<Slice<T>> make(HeapArray<T>& array, std::size_t start) noexcept {
            if(start >= array.size()) {
                return {ErrorTag::FATAL_ERROR, std::format("Index out of bounds: {} is greater than or equal to array size {}", start, array.size())};
            }
            return Slice{&array[start], array.size() - start};
        }
        static Error<Slice<T>> make(HeapArray<T>& array, std::size_t start, std::size_t count) noexcept {
            if(start + count > array.size()) {
                return {ErrorTag::FATAL_ERROR, std::format("Array out of bounds: {} + {} = {}, which is greater than or equal to array size {}", start, count, start + count, array.size())};
            }
            return Slice{&array[start], count};
        }
        T* data() noexcept {
            return mPtr;
        }
        const T* data() const noexcept {
            return mPtr;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        T& operator[](std::size_t index) noexcept {
            return mPtr[index];
        }
        T& operator[](std::size_t index) const noexcept {
            return mPtr[index];
        }

        class Iterator {
            private:
            T* mPtr;
            public:
            Iterator() = delete;
            Iterator(T* inPtr) noexcept : mPtr(inPtr) {}
            Iterator operator++(int) noexcept {
                return Iterator(mPtr++);
            }
            Iterator& operator++() noexcept {
                mPtr++;
                return *this;
            }
            bool operator==(const Iterator& rhs) const noexcept {
                return mPtr == rhs.mPtr;
            }
            bool operator!=(const Iterator& rhs) const noexcept {
                return mPtr != rhs.mPtr;
            }
            T& operator*() noexcept {
                return *mPtr;
            }
            T* operator->() noexcept {
                return mPtr;
            }
        };
        Iterator begin() const noexcept {
            return Iterator(mPtr);
        }
        Iterator end() const noexcept {
            return Iterator(mPtr + mSize);
        }
    };
    export template <class T>
    class ConstSlice {
        private:
        const T* mPtr;
        std::size_t mSize;
        public:
        ConstSlice(const T* ptr, std::size_t size) noexcept : mPtr(ptr), mSize(size) {}
        ConstSlice(const HeapArray<T>& array) noexcept : mPtr(array.data()), mSize(array.size()) {}
        template<std::size_t size>
        ConstSlice(const std::array<T, size>& array) noexcept : mPtr(array.data()), mSize(size) {}
        static Error<ConstSlice<T>> make(const HeapArray<T>& array, std::size_t start) noexcept {
            if(start >= array.size()) {
                return {ErrorTag::FATAL_ERROR, std::format("Index out of bounds: {} is greater than or equal to array size {}", start, array.size())};
            }
            return ConstSlice{&array[start], array.size() - start};
        }
        static Error<ConstSlice<T>> make(const HeapArray<T>& array, std::size_t start, std::size_t count) noexcept {
            if(start + count > array.size()) {
                return {ErrorTag::FATAL_ERROR, std::format("Array out of bounds: {} + {} = {}, which is greater than or equal to array size {}", start, count, start + count, array.size())};
            }
            return ConstSlice{&array[start], count};
        }
        const T* data() const noexcept {
            return mPtr;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        const T& operator[](std::size_t index) const noexcept {
            return mPtr[index];
        }

        class Iterator {
            private:
            const T* mPtr;
            public:
            Iterator() = delete;
            Iterator(const T* inPtr) noexcept : mPtr(inPtr) {}
            Iterator operator++(int) noexcept {
                return Iterator(mPtr++);
            }
            Iterator& operator++() noexcept {
                mPtr++;
                return *this;
            }
            bool operator==(const Iterator& rhs) const noexcept {
                return mPtr == rhs.mPtr;
            }
            bool operator!=(const Iterator& rhs) const noexcept {
                return mPtr != rhs.mPtr;
            }
            const T& operator*() const noexcept {
                return *mPtr;
            }
            const T* operator->() const noexcept {
                return mPtr;
            }
        };
        Iterator begin() const noexcept {
            return Iterator(mPtr);
        }
        Iterator end() const noexcept {
            return Iterator(mPtr + mSize);
        }
    };

}
