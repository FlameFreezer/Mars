#pragma once

#include <string>
#include <utility>
#include <print>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <list>

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

template<class T>
void destruct(T& t) {
    t.~T();
}

struct ErrorMessage {
    std::string message;
    ErrorMessage* prev{};
    ErrorMessage* next{};
};

class MessageList {
    ErrorMessage* mHead{};
    ErrorMessage* mTail{};
    void freeMemory() noexcept {
        ErrorMessage* current = mHead;
        while (current) {
            ErrorMessage* toDelete = current;
            current = current->next;
            delete toDelete;
        }
    }
    void copyList(const MessageList& other) noexcept {
        ErrorMessage* current = other.mHead;
        ErrorMessage** cursor = &mHead;
        ErrorMessage* prev = nullptr;

        while (current) {
            *cursor = new ErrorMessage{ .message = current->message, .prev = prev };
            mTail = *cursor;
            if (prev) {
                prev->next = *cursor;
            }
            prev = *cursor;
            current = current->next;
        }
    }
public:
    MessageList() noexcept = default;
    ~MessageList() noexcept {
        freeMemory();
	}
    void clear() noexcept {
        freeMemory();
        mHead = nullptr;
        mTail = nullptr;
    }
    MessageList(const MessageList& other) noexcept {
        copyList(other);
    }
    MessageList(MessageList&& other) noexcept : mHead(other.mHead), mTail(other.mTail) {
        other.mHead = nullptr;
        other.mTail = nullptr;
    }
    MessageList& operator=(const MessageList& other) noexcept {
        clear();
        copyList(other);
        return *this;
    }
    MessageList& operator=(MessageList&& other) noexcept {
        clear();
        mHead = other.mHead;
        mTail = other.mTail;
        other.mHead = nullptr;
        other.mTail = nullptr;
    }
    void pushBack(const std::string& message) noexcept {
        ErrorMessage* back = new ErrorMessage{ .message = message, .prev = mTail };
        if (!mHead) {
            mHead = back;
            mTail = back;
        }
        else {
			mTail->next = back;
			mTail = back;
        }
    }
    void pushBack(std::string&& message) noexcept {
        ErrorMessage* back = new ErrorMessage{ .message = std::move(message), .prev = mTail };
        if (!mHead) {
            mHead = back;
            mTail = back;
        }
        else {
			mTail->next = back;
			mTail = back;
        }

    }
    ErrorMessage* front() noexcept {
        return mHead;
    }
    const ErrorMessage* front() const noexcept {
        return mHead;
    }
    ErrorMessage* back() noexcept {
        return mTail;
    }
    const ErrorMessage* back() const noexcept {
        return mTail;
    }
    bool empty() const noexcept {
        return mHead == nullptr;
    }
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
            destruct(mMessage);
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
        std::println("{}: {}", typeToString(mTag), cursor->message);
        cursor = cursor->next;
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
        std::println(ostrm, "{}: {}", typeToString(mTag), cursor->message);
        cursor = cursor->next;
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
            destruct(mMessage);
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
    MessageList&& moveMessage() noexcept {
        assert(!okay());
        return std::move(mMessage);
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report() const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        std::println("{}: {}", typeToString(mTag), cursor->message);
        cursor = cursor->next;
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
        std::println(ostrm, "{}: {}", typeToString(mTag), cursor->message);
        cursor = cursor->next;
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
            destruct(mMessage);
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
    MessageList&& moveMessage() noexcept {
        assert(!okay());
        return std::move(mMessage);
    }
    //Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    bool report() const noexcept {
        if(okay()) return true;
        const ErrorMessage* cursor = mMessage.front();
        std::println("{}: {}", typeToString(mTag), cursor->message);
        cursor = cursor->next;
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
        std::println(ostrm, "{}: {}", typeToString(mTag), cursor->message);
        cursor = cursor->next;
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
#define FATAL(msg) {ErrorTag::fatalError, msg}
#define SUCCESS Error<noreturn>{}

#define TRY(proc) \
if(auto procResult = proc; !procResult.okay()) return procResult

#define TRY_ASSIGN(name, proc) \
if(auto procResult = proc; !procResult.okay()) return MOVE_ERROR(procResult);\
else name = procResult.moveValue()

#define TRY_INIT(type, name, proc) \
type name{};\
if(auto procResult = proc; !procResult.okay()) return MOVE_ERROR(procResult);\
else name = procResult.moveValue()
