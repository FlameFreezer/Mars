#pragma once

#include <format>
#include <utility>

#include "error.h"

namespace mars {
    template<class T>
    class Swaparray {
    public:
        Swaparray() noexcept : mSize(0), mCapacity(mInitialCapacity), mData(new T[mInitialCapacity]) {}
        Swaparray(const Swaparray& other) noexcept : mSize(other.mSize), mCapacity(other.mCapacity), mData(nullptr) {
            if (other.mData) {
                mData = new T[mCapacity];
            }
            for (size_t i = 0; i < mSize; i++) {
                mData[i] = other.mData[i];
            }
        }
        Swaparray(Swaparray&& other) noexcept : mSize(other.mSize), mCapacity(other.mCapacity), mData(other.mData) {
            other.mData = nullptr;
            other.mSize = 0;
            other.mCapacity = 0;
        }
        ~Swaparray() noexcept {
            if (mData) {
                delete[] mData;
            }
        }
        Swaparray& operator=(const Swaparray& other) noexcept {
            if (mData) {
                delete[] mData;
            }
            mSize = other.mSize;
            mCapacity = other.mCapacity;
            mData = new T[mCapacity];
            for (size_t i = 0; i < mSize; i++) {
                mData[i] = other.mData[i];
            }
            return *this; 
        }
        Swaparray& operator=(Swaparray&& other) noexcept {
            if (mData) {
                delete[] mData;
            }
            mSize = other.mSize;
            mCapacity = other.mCapacity;
            mData = other.mData;

            other.mData = nullptr;
            other.mSize = 0;
            other.mCapacity = 0;
            return *this;
        }
        size_t size() const noexcept {
            return mSize;
        }
        size_t capacity() const noexcept {
            return mCapacity;
        }
        void pushBack(const T& element) noexcept {
            if (mCapacity == 0) {
                mData = new T[mInitialCapacity];
                mCapacity = mInitialCapacity;
            }
            else if (mSize == mCapacity) {
                mCapacity *= mGrowthRatio;
                T* data = new T[mCapacity];
                for (size_t i = 0; i < mSize; i++) {
                    data[i] = mData[i];
                }
                delete[] mData;
                mData = data;
            }
            mData[mSize++] = element;
        }
        void erase(size_t index) noexcept {
            if (index + 1 == mSize) {
                mData[index] = T{};
                --mSize;
            }
            else if (index < mSize) {
                mData[index] = std::move(mData[--mSize]);
            }
        }
        T& operator[](size_t index) noexcept {
            return mData[index];
        }
        const T& operator[](size_t index) const noexcept {
            return mData[index];
        }
        Error<T&> at(size_t index) noexcept {
            if (index >= mSize) {
                FATAL(std::format("Tried to access index {} in a Swaparray with size {}", index, mSize));
            }
            return mData[index];
        }
        Error<const T&> at(size_t index) const noexcept {
            if (index >= mSize) {
                FATAL(std::format("Tried to access index {} in a Swaparray with size {}", index, mSize));
            }
            return mData[index];
        }
    private:
        T* mData;
        size_t mSize;
        size_t mCapacity;
        static constexpr size_t mInitialCapacity = 5;
        static constexpr float mGrowthRatio = 1.5f;
    };
}