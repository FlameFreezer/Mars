#pragma once

#include <string>
#include <utility>
#include <print>
#include <iostream>
#include <stdexcept>

#include "mars_types.h"

enum class ErrorTag : u8 {
    allOkay = 0,
    searchFail,
    fatalError,
};

static constexpr std::string typeToString(ErrorTag tag) noexcept {
    switch(tag) {
    case ErrorTag::allOkay:
        return "All Okay";
    case ErrorTag::searchFail:
        return "Search Fail";
    case ErrorTag::fatalError:
        return "Fatal Error";
    }
    std::unreachable();
}

template <class T>
class [[nodiscard("Potentially unhandled error value")]] Error {
    union {
        T mValue {};
        std::string mMessage;
    };
    ErrorTag mTag {ErrorTag::allOkay};
    public:
    Error() noexcept = default;
    Error(const T& inValue) noexcept : mTag(ErrorTag::allOkay), mValue(inValue) {}
    Error(T&& inValue) noexcept : mTag(ErrorTag::allOkay), mValue(std::move(inValue)) {}
    Error(ErrorTag inTag, const std::string& inMessage) noexcept : mTag(inTag), mMessage(inMessage) {}
    Error(ErrorTag inTag, std::string&& inMessage) noexcept : mTag(inTag), mMessage(std::move(inMessage)) {}
    Error(const Error<T>& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = other.mValue;
        }
        else {
            mValue.~T();
            new (&mMessage) std::string{other.mMessage};
        }
    }
    Error(Error<T>&& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = std::move(other.mValue);
        }
        else {
            mValue.~T();
            new (&mMessage) std::string{std::move(other.mMessage)};
        }
    }
    Error<T>& operator=(Error<T>&& rhs) noexcept {
        if(this != &rhs) {
            this->~Error<T>();
            mTag = rhs.mTag;
            if(rhs.okay()) {
                new (&mValue) T{std::move(rhs.mValue)};
            }
            else {
                new (&mMessage) std::string{std::move(rhs.mMessage)};
            }
        }
        return *this;
    }
    ~Error() noexcept {
        if(okay()) {
            mValue.~T();
        }
        else {
            mMessage.~basic_string();
        }
    }
    //Returns `true` if `this->tag` is of a value not indicating an error during execution.
    bool okay() const noexcept {
        return mTag == ErrorTag::allOkay;
    }
    operator bool() const noexcept {
        return okay();
    }
    ErrorTag tag() const noexcept {
        return mTag;
    }
    const T& value() const {
        if(!okay()) [[unlikely]] throw std::runtime_error("Called \"value\" on an error union which does not have a value");
        return mValue;
    }
    T& value() {
        if(!okay()) [[unlikely]] throw std::runtime_error("Called \"value\" on an error union which does not have a value");
        return mValue;
    }
    //Creates an rvalue reference to `value`. `value` is invalid after calling this, 
    // though it is still considered the active union field.
    T&& moveValue() {
        if(!okay()) [[unlikely]] throw std::runtime_error("Called \"moveData\" on an error union which does not have a value");
        return std::move(mValue);
    }
    const std::string& message() const {
        if(okay()) [[unlikely]] throw std::runtime_error("Called \"message\" on an error union which does not have an error");
        return mMessage;
    }
    std::string&& moveMessage() {
        if(okay()) [[unlikely]] throw std::runtime_error("Called \"moveMessage\" on an error union which does not have an error");
        return std::move(mMessage);
    }
    //Creates an Error union of the templated type, moving the tag and message from the calling 
    // Error union to it. The callng Error union is left `okay`, with value in a default-initialized 
    // state. Calling this function on an Error union that is `okay` throws an exception.
    template<class U = noreturn>
    Error<U> moveError() {
        if(okay()) [[unlikely]] throw std::runtime_error("Called \"moveError\" on an error union which does not have an error");
        Error<U> result{mTag, std::move(mMessage)};
        mTag = ErrorTag::allOkay;
        new (&mValue) T{};
        return result;
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report() const noexcept {
        if(okay()) return true;
        std::println("{}: {}", typeToString(mTag), mMessage);
        return false;
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report(std::ostream& ostrm) const noexcept {
        if(okay()) return true;
        std::println(ostrm, "{}: {}", typeToString(mTag), mMessage);
        return false;
    }
};

template<>
const noreturn& Error<noreturn>::value() const = delete;
template<>
noreturn& Error<noreturn>::value() = delete;

//Returns an `Error<noreturn>` with `key == allOkay`. Used mainly for the final return value of a function with return type `Error<noreturn>`.
Error<noreturn> success() noexcept;

//Returns an `Error<T>` with `key == fatalError`.
template<typename T = noreturn>
Error<T> fatal(std::string&& message) noexcept {
    return Error<T>(ErrorTag::fatalError, std::move(message));
}
template<typename T = noreturn>
Error<T> fatal(const std::string& message) noexcept {
    return Error<T>(ErrorTag::fatalError, message);
}

#define MOVE_ERROR(err) {err.tag(), err.moveMessage()}

#define TRY(proc) \
if(auto procResult = proc; !procResult) return procResult

#define TRY_ASSIGN(name, proc) \
if(auto procResult = proc; !procResult) return MOVE_ERROR(procResult);\
else name = procResult.moveValue()

#define TRY_INIT(type, name, proc) \
type name{};\
if(auto procResult = proc; !procResult) return MOVE_ERROR(procResult);\
else name = procResult.moveValue()
