module;

#include <cstdint>
#include <string>
#include <utility>
#include <cstring>
#include <print>
#include <iostream>
#include <stdexcept>

export module error;

namespace mars {
    export class noreturn {};

    export enum class ErrorTag : uint32_t {
        ALL_OKAY = 0,
        SEARCH_FAIL,
        FATAL_ERROR,
    };

    export constexpr std::string tagToString(ErrorTag tag) noexcept {
        switch(tag) {
        case ErrorTag::ALL_OKAY:
            return "All Okay";
        case ErrorTag::SEARCH_FAIL:
            return "Search Fail";
        case ErrorTag::FATAL_ERROR:
            return "Fatal Error";
        }
        std::unreachable();
    }

    export template <class T>
    class [[nodiscard("Potentially unhandled error value")]] Error {
        private:
        union {
            T mData;
            std::string mMessage;
        };
        ErrorTag mTag;
        //Writes zeroes to the entire memory space taken up by the Error union, then sets the tag to 
        // `ALL_OKAY`. Does NOT call destructors to the member `data` or `memory`.
        void reset() {
            std::memset(static_cast<void*>(this), 0x00, sizeof(Error<T>));
            mTag = ErrorTag::ALL_OKAY;
        }
        //Calls the destructor of the active data member, then resets the union.
        void clear() {
            if(okay()) {
                mData.~T();
            }
            else {
                mMessage.~basic_string();
            }
            reset();
        }
        public:
        Error() noexcept : mTag(ErrorTag::ALL_OKAY), mData() {}
        Error(const T& inData) noexcept : mTag(ErrorTag::ALL_OKAY), mData(inData) {}
        Error(T&& inData) noexcept : mTag(ErrorTag::ALL_OKAY), mData(std::move(inData)) {}
        Error(ErrorTag inTag, const std::string& inMessage) noexcept : mTag(inTag), mMessage(inMessage) {}
        Error(ErrorTag inTag, std::string&& inMessage) noexcept : mTag(inTag), mMessage(std::move(inMessage)) {}
        Error(const Error<T>& other) noexcept {
            //Zero memory, so that any pointers stored within data members are null before any attempted initialization
            std::memset(static_cast<void*>(this), 0x00, sizeof(Error<T>));
            mTag = other.mTag;
            if(other.okay()) {
                mData = other.mData;
            }
            else {
                mMessage = other.mMessage;
            }
        }
        Error(Error<T>&& other) noexcept {
            //Zero memory, so that any pointers stored within data members are null before any attempted initialization
            std::memset(static_cast<void*>(this), 0x00, sizeof(Error<T>));
            mTag = other.mTag;
            if(other.okay()) {
                mData = std::move(other.mData);
            }
            else {
                mMessage = std::move(other.mMessage);
            }
            other.reset();
        }
        Error<T>& operator=(Error<T>&& rhs) noexcept {
            if(this != &rhs) {
                //Call destructor on active data member, then write zeroes to whole object.
                //We have to do this to prevent any invalid pointers from being read once the 
                // memory is reinterpreted
                clear();
                //Now we can safely assign data members
                mTag = rhs.mTag;
                if(rhs.okay()) {
                    mData = std::move(rhs.mData);
                }
                else {
                    mMessage = std::move(rhs.mMessage);
                }
                rhs.reset();
            }
            return *this;
        }
        ~Error() noexcept {
            if(okay()) {
                mData.~T();
            }
            else {
                mMessage.~basic_string();
            }
        }
        //Returns `true` if `this->tag` is of a value not indicating an error during execution.
        bool okay() const noexcept {
            return mTag == ErrorTag::ALL_OKAY;
        }
        operator bool() const noexcept {
            return okay();
        }
        ErrorTag tag() const noexcept {
            return mTag;
        }
        const T& data() const {
            if(!okay()) [[unlikely]] throw std::runtime_error("Called \"data\" on an error union which does not have a value");
            return mData;
        }
        T& data() {
            if(!okay()) [[unlikely]] throw std::runtime_error("Called \"data\" on an error union which does not have a value");
            return mData;
        }
        operator const T&() const {
            if(!okay()) [[unlikely]] throw std::runtime_error("Called \"operator T& const\" on an error union which does not have a value");
            return mData;
        }
        //Creates an rvalue reference to `data`. `data` is invalid after calling this, 
        // though it is still considered the active union field.
        T&& moveData() {
            if(!okay()) [[unlikely]] throw std::runtime_error("Called \"moveData\" on an error union which does not have a value");
            return std::move(mData);
        }
        const std::string& message() const {
            if(okay()) [[unlikely]] throw std::runtime_error("Called \"message\"  on an error union which does not have a message");
            return mMessage;
        }
        //Creates an Error union of the templated type, moving the tag and message from the calling 
        // Error union to it. The callng Error union is left `okay`, with data set to zeroes. Calling this function on an 
        // Error union that is `okay` throws an exception.
        template<class U = noreturn>
        Error<U> moveError() {
            if(okay()) [[unlikely]] throw std::runtime_error("Called \"moveError\"  on an error union which does not have a message");
            Error<U> result(mTag, std::move(mMessage));
            reset();
            return result;
        }
    	//Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    	bool report() const noexcept {
    	    if(okay()) return true;
            std::println("{}: {}", tagToString(mTag), mMessage);
    	    return false;
    	}
    	//Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    	bool report(std::ostream& ostrm) const noexcept {
    	    if(okay()) return true;
            std::println(ostrm, "{}: {}", tagToString(mTag), mMessage);
    	    return false;
    	}
    };

    template<>
    const noreturn& Error<noreturn>::data() const = delete;
    template<>
    noreturn& Error<noreturn>::data() = delete;

    //Returns an `Error<noreturn>` with `key == ALL_OKAY`. Used mainly for the final return value of a function with return type `Error<noreturn>`.
    export Error<noreturn> success() noexcept {
        return Error<noreturn>();
    }
    //Returns an `Error<T>` with `key == FATAL_ERROR`.
    export template<typename T = noreturn>
    Error<T> fatal(std::string&& message) noexcept {
        return Error<T>(ErrorTag::FATAL_ERROR, std::move(message));
    }
    export template<typename T = noreturn>
    Error<T> fatal(const std::string& message) noexcept {
        return Error<T>(ErrorTag::FATAL_ERROR, message);
    }
}
