module;

#include <memory>

export module array;

namespace mars {
    export template <class T>
    class Array {
        private:
        std::unique_ptr<T[]> mPtr;
        std::size_t mSize;
        public:
        Array() noexcept : mSize(0) {}
        Array(std::size_t inSize) noexcept : mPtr(std::make_unique<T[]>(inSize)), mSize(inSize) {}
        Array(std::size_t inSize, T initial) noexcept : mPtr(std::make_unique<T[]>(inSize)), mSize(inSize) {
            for(int i = 0; i < mSize; i++) mPtr[i] = initial;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        T& operator[](std::size_t index) noexcept {
            return mPtr[index];
        }
        Array<T>& operator=(Array<T>&& rhs) noexcept {
            if(this != &rhs) {
                mPtr = std::move(rhs.mPtr);
                mSize = rhs.mSize;
                rhs.mSize = 0;
            }
            return *this;
        }
        T* data() const noexcept {
            return mPtr.get();
        }

        class Iterator {
            private:
            Array<T> const& mArray;
            std::size_t mIndex;
            public:
            friend class Array<T>;
            Iterator() = delete;
            Iterator(Array<T> const& arr, std::size_t index) noexcept : mArray(arr), mIndex(index) {}
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
}