module;

#include <cstdint>
#include <string>
#include <utility>
#include <cstring>
#include <print>
#include <iostream>

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
        default:
            return "";
        }
    }

    export template <class T>
    class [[nodiscard("Potentially unhandled error value")]] Error {
        private:
        union {
            T mData;
            std::string mMessage;
        };
        ErrorTag mtag;
        //Writes zeroes to the entire memory space taken up by the Error union, then sets the tag to 
        // `ALL_OKAY`. Does NOT call destructors to the member `data` or `memory`.
        void reset() {
            std::memset(static_cast<void*>(this), 0x00, sizeof(Error<T>));
            mtag = ErrorTag::ALL_OKAY;
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
        Error() noexcept : mtag(ErrorTag::ALL_OKAY), mData() {}
        Error(T const& inData) noexcept : mtag(ErrorTag::ALL_OKAY), mData(inData) {}
        Error(T&& inData) noexcept : mtag(ErrorTag::ALL_OKAY), mData(std::move(inData)) {}
        Error(ErrorTag inTag, std::string const& inMessage) noexcept : mtag(inTag), mMessage(inMessage) {}
        Error(ErrorTag inTag, std::string&& inMessage) noexcept : mtag(inTag), mMessage(std::move(inMessage)) {}
        Error(Error<T> const& other) noexcept {
            //Zero memory, so that any pointers stored within data members are null before any attempted initialization
            std::memset(static_cast<void*>(this), 0x00, sizeof(Error<T>));
            mtag = other.mtag;
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
            mtag = other.mtag;
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
                mtag = rhs.mtag;
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
        //Creates an Error union of the templated type, moving the tag and message from the calling 
        // Error union to it. The callng Error union is left `okay`, with data set to zeroes. Calling this function on an 
        // Error union that is `okay` raises a compile error.
        template<class U>
        Error<U> moveError() noexcept {
            Error<U> result(mtag, std::move(mMessage));
            reset();
            return result;
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
            return mtag == ErrorTag::ALL_OKAY;
        }
        ErrorTag tag() const noexcept {
            return mtag;
        }
        T const& data() const noexcept {
            return mData;
        }
        T& data() noexcept {
            return mData;
        }
        //Creates an rvalue reference to `data`. `data` is invalid after calling this, 
        // though it is still considered the active union field.
        T&& moveData() noexcept {
            return std::move(mData);
        }
        std::string const& message() const noexcept {
            return mMessage;
        }
    	//Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    	bool report() const noexcept {
    	    if(okay()) return true;
            std::println("{}: {}", tagToString(mtag), mMessage);
    	    return false;
    	}
    	//Returns `true` if okay. Otherwise, prints `message` and returns `false`.
    	bool report(std::ostream& ostrm) const noexcept {
    	    if(okay()) return true;
            std::println(ostrm, "{}: {}", tagToString(mtag), mMessage);
    	    return false;
    	}
    };

    template<>
    noreturn const& Error<noreturn>::data() const noexcept = delete;

    //Returns an `Error<noreturn>` with `key == ALL_OKAY`. Used mainly for the final return value of a function with return type `Error<noreturn>`.
    export Error<noreturn> success() {
        return Error<noreturn>();
    }
}
