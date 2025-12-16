module;

#include <cstddef>
#include <initializer_list>

export module array;

namespace mars {
    //Essentially implements the "fat pointer" from zig
    export template <class T>
    class Array {
        protected:
        T* mPtr;
        std::size_t mSize;
        Array(T* inPtr, std::size_t inSize) noexcept : mPtr(inPtr), mSize(inSize) {}
        public:
        Array() noexcept : mPtr(nullptr), mSize(0) {}
        Array(std::size_t inSize) noexcept : mPtr(new T[inSize]), mSize(inSize) {}
        Array(std::size_t inSize, T initial) noexcept : mPtr(new T[inSize]), mSize(inSize) {
            for(std::size_t i = 0; i < mSize; i++) mPtr[i] = initial;
        }
        virtual ~Array() noexcept {
            delete[] mPtr;
            mSize = 0;
        }
        std::size_t size() const noexcept {
            return mSize;
        }
        T& operator[](std::size_t index) noexcept {
            return mPtr[index];
        }
        Array<T>& operator=(Array<T>&& rhs) noexcept {
            if(this != &rhs) {
                mPtr = rhs.mPtr;
                mSize = rhs.mSize;
                rhs.mPtr = 0;
                rhs.mSize = 0;
            }
            return *this;
        }
        Array<T>& operator=(Array<T> const& rhs) noexcept {
            if(this != &rhs) {
                delete[] mPtr;
                mPtr = new T[rhs.mSize];
                for(std::size_t i = 0; i < rhs.mSize; i++) mPtr[i] = rhs.mPtr[i];
                mSize = rhs.mSize;
            }
            return *this;
        }
        Array<T>& operator=(std::initializer_list<T> list) noexcept {
            delete[] mPtr;
            mSize = list.size();
            mPtr = new T[mSize];
            int i = 0;
            for(T const& x : list) mPtr[i++] = x;
            return *this; 
        }
        T* data() const noexcept {
            return mPtr;
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

    export template <class T>
    class Slice : public Array<T> {
        public:
        Slice() noexcept {}
        Slice(Array<T> const& array) noexcept : Array<T>(array.data(), array.size()) {}
        Slice(Array<T> const& array, std::size_t start) noexcept : Array<T>(array.data() + start, array.size() - start) {
            if(start >= array.size()) {
                this->mSize = 0;
                this->mPtr = nullptr;
            }
        }
        Slice(Array<T> const& array, std::size_t start, std::size_t count) noexcept : Array<T>(array.data() + start, count) {
            if(start + count >= array.size() or start >= array.size()) {
                this->mSize = 0;
                this->mPtr = nullptr;
            }
        }
        Slice(Array<T>&& array) noexcept = delete;
        Slice(Slice<T> const& other) noexcept : Array<T>(other.mPtr, other.mSize) {}
        Slice(Slice<T>&& other) noexcept : Array<T>(other.mPtr, other.mSize) {
            other.mPtr = nullptr;
            other.mSize = 0;
        }
        virtual ~Slice() noexcept override {
            this->mPtr = nullptr;
            this->mSize = 0;
        }
        //Override behavior of copy assignment operator so that slices do not construct new arrays
        Slice<T>& operator=(Array<T> const& rhs) noexcept {
            if(this != &rhs) {
                this->mPtr = rhs.data();
                this->mSize = rhs.size();
            }
            return *this;
        }
        //A slice can never be assigned to a temporary array
        Slice<T>& operator=(Array<T>&& rhs) = delete;
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
    };
}