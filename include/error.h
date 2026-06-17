#pragma once

#include <string>
#include <utility>
#include <print>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <list>
#include <source_location>

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

struct ErrorMessage {
    std::string message;
    ErrorMessage* prev{};
    ErrorMessage* next{};
};

class MessageList {
    ErrorMessage* mHead{};
    ErrorMessage* mTail{};
    void freeMemory() noexcept;
    void copyList(const MessageList& other) noexcept;
public:
    MessageList() noexcept = default;
    ~MessageList() noexcept;
    void clear() noexcept;
    MessageList(const MessageList& other) noexcept;
    MessageList(MessageList&& other) noexcept;
    MessageList& operator=(const MessageList& other) noexcept;
    MessageList& operator=(MessageList&& other) noexcept;
    void pushBack(const std::string& message) noexcept;
    void pushBack(std::string&& message) noexcept;
    void pushFront(const std::string& message) noexcept;
    void pushFront(std::string&& message) noexcept;
    ErrorMessage* front() noexcept;
    const ErrorMessage* front() const noexcept;
    ErrorMessage* back() noexcept;
    const ErrorMessage* back() const noexcept;
    bool empty() const noexcept;
};

template <class T>
class [[nodiscard("Potentially unhandled error value")]] Error {
    union {
        T mValue {};
        MessageList mMessage;
    };
    ErrorTag mTag {ErrorTag::allOkay};
public:
	friend class Error<T&>;
	friend class Error<const T&>;
    Error() noexcept = default;
    Error(const T& inValue) noexcept : mTag(ErrorTag::allOkay), mValue(inValue) {}
    Error(T&& inValue) noexcept : mTag(ErrorTag::allOkay), mValue(std::move(inValue)) {}
    Error(ErrorTag inTag, const std::string& inMessage) noexcept : mTag(inTag), mMessage() {
        mMessage.pushBack(inMessage);
    }
    Error(ErrorTag inTag, std::string&& inMessage) noexcept : mTag(inTag), mMessage() {
        mMessage.pushBack(std::move(inMessage));
    }
    Error(ErrorTag inTag, const MessageList& list) noexcept : mTag(inTag), mMessage(list) {}
    Error(ErrorTag inTag, MessageList&& list) noexcept : mTag(inTag), mMessage(std::move(list)) {}
    Error(const Error<T>& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = other.mValue;
        }
        else {
            mValue.~T();
            new (&mMessage) MessageList{other.mMessage};
        }
    }
    Error(Error<T>&& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = std::move(other.mValue);
        }
        else {
            mValue.~T();
            new (&mMessage) MessageList{std::move(other.mMessage)};
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
                new (&mMessage) MessageList{std::move(rhs.mMessage)};
            }
        }
        return *this;
    }
    ~Error() noexcept {
        if(okay()) {
            mValue.~T();
        }
        else {
            mMessage.~MessageList();
        }
    }
    //Returns `true` if `this->tag` is of a value not indicating an error during execution.
    bool okay() const noexcept {
        return mTag == ErrorTag::allOkay;
    }
    ErrorTag tag() const noexcept {
        return mTag;
    }
    const T& value() const noexcept {
        assert(okay());
        return mValue;
    }
    T& value() noexcept {
        assert(okay());
        return mValue;
    }
    //Creates an rvalue reference to `value`. `value` is invalid after calling this, 
    // though it is still considered the active union field.
    T&& moveValue() noexcept {
        assert(okay());
        return std::move(mValue);
    }
    const MessageList& message() const noexcept {
        assert(!okay());
        return mMessage;
    }
    MessageList& message() noexcept {
        assert(!okay());
        return mMessage;
    }
    MessageList&& moveMessage() noexcept {
        assert(!okay());
        return std::move(mMessage);
    }
    //Creates an Error union of the templated type, moving the tag and message from the calling 
    // Error union to it. The callng Error union is left `okay`, with value in a default-initialized 
    // state. Calling this function on an Error union that is `okay` throws an exception.
    template<class U = noreturn>
    Error<U> moveError() noexcept {
        assert(!okay());
        Error<U> result{mTag, std::move(mMessage)};
        mTag = ErrorTag::allOkay;
        new (&mValue) T{};
        return result;
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report() const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        while (cursor) {
            std::println("{}", cursor->message);
            cursor = cursor->next;
        }
        return false;
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report(std::ostream& ostrm) const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        while (cursor) {
            std::println(ostrm, "{}", cursor->message);
            cursor = cursor->next;
        }
        return false;
    }
};

template <class T>
class [[nodiscard("Potentially unhandled error value")]] Error<T&> {
    union {
        T* mValue;
        MessageList mMessage;
    };
    ErrorTag mTag {ErrorTag::allOkay};
public:
	friend class Error<T&>;
	friend class Error<const T&>;
    Error() noexcept = delete;
    Error(T& inValue) noexcept : mTag(ErrorTag::allOkay), mValue(&inValue) {}
    Error(ErrorTag inTag, const std::string& inMessage) noexcept : mTag(inTag), mMessage() {
        mMessage.pushBack(inMessage);
    }
    Error(ErrorTag inTag, std::string&& inMessage) noexcept : mTag(inTag), mMessage() {
        mMessage.pushBack(std::move(inMessage));
    }
    Error(const Error<T&>& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = other.mValue;
        }
        else {
            new (&mMessage) MessageList{other.mMessage};
        }
    }
    ~Error() noexcept {
        if (!okay()) {
            mMessage.~MessageList();
        }
    }
    //Cannot move Error Unions which hold references
    Error(Error<T&>&&) noexcept = delete;
    Error<T&>& operator=(Error<T&>&&) noexcept = delete;
    //Returns `true` if `this->tag` is of a value not indicating an error during execution.
    bool okay() const noexcept {
        return mTag == ErrorTag::allOkay;
    }
    ErrorTag tag() const noexcept {
        return mTag;
    }
    const T& value() const noexcept {
        assert(okay());
        return *mValue;
    }
    T& value() noexcept {
        assert(okay());
        return *mValue;
    }
    const T& moveValue() const noexcept {
        assert(okay());
        return value();
    }
    T& moveValue() noexcept {
        assert(okay());
        return value();
    }
    const MessageList& message() const noexcept {
        assert(!okay());
        return mMessage;
    }
    MessageList& message() noexcept {
        assert(!okay());
        return mMessage;
    }
    MessageList&& moveMessage() noexcept {
        assert(!okay());
        return std::move(mMessage);
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report() const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        while (cursor) {
            std::println("{}", cursor->message);
            cursor = cursor->next;
        }
        return false;
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report(std::ostream& ostrm) const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        while (cursor) {
            std::println(ostrm, "{}", cursor->message);
            cursor = cursor->next;
        }
        return false;
    }
};
template <class T>
class [[nodiscard("Potentially unhandled error value")]] Error<const T&> {
    union {
        const T* mValue;
        MessageList mMessage;
    };
    ErrorTag mTag {ErrorTag::allOkay};
public:
	friend class Error<T&>;
	friend class Error<const T&>;
    Error() noexcept = delete;
    Error(const T& inValue) noexcept : mTag(ErrorTag::allOkay), mValue(&inValue) {}
    Error(ErrorTag inTag, const std::string& inMessage) noexcept : mTag(inTag), mMessage() {
        mMessage.pushBack(inMessage);
    }
    Error(ErrorTag inTag, std::string&& inMessage) noexcept : mTag(inTag), mMessage() {
        mMessage.pushBack(std::move(inMessage));
    }
    Error(const Error<const T&>& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = other.mValue;
        }
        else {
            new (&mMessage) MessageList{other.mMessage};
        }
    }
    Error(const Error<T&>& other) noexcept {
        mTag = other.mTag;
        if(other.okay()) {
            mValue = other.mValue;
        }
        else {
            new (&mMessage) MessageList{ other.mMessage };
        }
    }

    ~Error() noexcept {
        if (!okay()) {
            mMessage.~MessageList();
        }
    }
    //Cannot move Error Unions which hold references
    Error(Error<const T&>&&) noexcept = delete;
    Error<const T&>& operator=(Error<const T&>&&) noexcept = delete;
    //Returns `true` if `this->tag` is of a value not indicating an error during execution.
    bool okay() const noexcept {
        return mTag == ErrorTag::allOkay;
    }
    ErrorTag tag() const noexcept {
        return mTag;
    }
    const T& value() const noexcept {
        assert(okay());
        return *mValue;
    }
    const MessageList& message() const noexcept {
        assert(!okay());
        return mMessage;
    }
    MessageList& message() noexcept {
        assert(!okay());
        return mMessage;
    }
    MessageList&& moveMessage() noexcept {
        assert(!okay());
        return std::move(mMessage);
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report() const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        while (cursor) {
            std::println("{}", cursor->message);
            cursor = cursor->next;
        }
        return false;
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report(std::ostream& ostrm) const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        while (cursor) {
            std::println(ostrm, "{}", cursor->message);
            cursor = cursor->next;
        }
        return false;
    }
};

template<>
const noreturn& Error<noreturn>::value() const noexcept = delete;
template<>
noreturn& Error<noreturn>::value() noexcept = delete;
template<>
noreturn&& Error<noreturn>::moveValue() noexcept = delete;

#define MOVE_ERROR(err) {err.tag(), err.moveMessage()}

#define FATAL(msg) do{\
    std::source_location source{std::source_location::current()};\
    std::string fullMessage {std::format("In file: {}:{}\n\tIn function: {}\n\n{}: {}", source.file_name(), source.line(), source.function_name(), typeToString(ErrorTag::fatalError), msg)};\
return {ErrorTag::fatalError, fullMessage};\
} while(false)

#define SUCCESS Error<noreturn>{}

#define APPEND_SOURCE_INFO(errorUnion) do{\
	std::source_location source{std::source_location::current()};\
	std::string nextMsg {std::format("In file: {}:{}\n\tIn function: {}\n", source.file_name(), source.line(), source.function_name())};\
	errorUnion.message().pushFront(std::move(nextMsg));\
} while(false)

#define PROPAGATE_ERROR(errorUnion)\
APPEND_SOURCE_INFO(errorUnion);\
return MOVE_ERROR(errorUnion)

#define TRY(proc) \
if(auto procResult = proc; !procResult.okay()) do {\
    PROPAGATE_ERROR(procResult);\
} while(false)

#define TRY_ASSIGN(name, proc) \
if(auto procResult = proc; !procResult.okay()) {\
    PROPAGATE_ERROR(procResult);\
}\
else name = procResult.moveValue()

#define TRY_INIT(type, name, proc) \
type name{};\
if(auto procResult = proc; !procResult.okay()) {\
    PROPAGATE_ERROR(procResult);\
}\
else name = procResult.moveValue()

#define TRY_RETURN(proc) \
if(auto procResult = proc; !procResult.okay()) {\
    APPEND_SOURCE_INFO(procResult);\
    return procResult;\
}\
else return procResult
