module;

#include <cstdint>
#include <string>
#include <utility>
#include <cstring>
#include <print>

export module error;

namespace mars {
    export class noreturn {};

    export enum class ErrorTag : uint32_t {
        ALL_OKAY = 0,
        MISC_ERROR,
        BAD_FUNCTION_CALL,
        SEARCH_FAIL,
        MEMORY_ALLOC_FAIL,
        VULKAN_QUERY_ERROR,
        VULKAN_VALIDATION_LAYER,
        INIT_SDL_FAIL,
        WINDOW_CREATION_FAIL,
        INSTANCE_CREATION_FAIL,
        SURFACE_CREATION_FAIL,
        DEBUG_MESSENGER_CREATION_FAIL,
        FIND_SUITABLE_GPU_FAIL,
        DEVICE_CREATION_FAIL,
        SWAPCHAIN_CREATION_FAIL,
	SDL_QUERY_FAIL,
	COMMAND_POOL_CREATE_FAIL,
	COMMAND_BUFFER_ALLOC_FAIL,
	SWAPCHAIN_IMAGE_ACQUISITION_FAIL,
	IMAGE_VIEW_CREATE_FAIL,
    };

    export template <class T>
    class [[nodiscard("Potentially unhandled error value")]] Error {
        public:
        Error() noexcept : tag(ErrorTag::ALL_OKAY), data() {}
        Error(const T& inData) noexcept : tag(ErrorTag::ALL_OKAY), data(inData) {}
        Error(T&& inData) noexcept : tag(ErrorTag::ALL_OKAY), data(std::move(inData)) {}
        Error(ErrorTag inTag, const std::string& inMessage) noexcept : tag(inTag), message(inMessage) {}
        Error(ErrorTag inTag, std::string&& inMessage) noexcept : tag(inTag), message(std::move(inMessage)) {}
        Error(const Error<T>& other) noexcept : tag(other.tag) {
            if(other.okay()) {
                data = other.data;
            }
            else {
                message = other.message;
            }
        }
        Error(Error<T>&& other) noexcept : tag(other.tag) {
            if(other.okay()) {
                data = std::move(other.data);
            }
            else {
                message = std::move(other.message);
            }
        }
        Error<T>& operator=(const Error<T>& rhs) noexcept {
            if(this != &rhs) {
                //Call destructor on active data member, then write zeroes to whole object.
                //We have to do this to prevent any invalid pointers from being read once the 
                // memory is reinterpreted
                zeroMemory();
                //Now we can safely assign data members
                tag = rhs.tag;
                if(rhs.okay()) {
                    data = rhs.data;
                }
                else {
                    message = rhs.message;
                }
            }
            return *this;
        }
        Error<T>& operator=(Error<T>&& rhs) noexcept {
            if(this != &rhs) {
                //Call destructor on active data member, then write zeroes to whole object.
                //We have to do this to prevent any invalid pointers from being read once the 
                // memory is reinterpreted
                zeroMemory();
                //Now we can safely assign data members
                tag = rhs.tag;
                if(rhs.okay()) {
                    data = rhs.data;
                }
                else {
                    message = rhs.message;
                }
            }
            return *this;
        }
        ~Error() noexcept {
        }
        //Returns `true` if `this->tag` is of a value not indicating an error during execution.
        bool okay() const noexcept {
            return tag == ErrorTag::ALL_OKAY;
        }
        ErrorTag getTag() const noexcept {
            return tag;
        }
        //Accessor for `data`. Raises a compile error if `message` is the active union field.
        T getData() const noexcept {
            //No data to retrieve if there has been an error - message is the active union field
            if(!okay()) std::unreachable();
            return data;
        }
        //Accessor for `message`. Raises a compile error if `data` is the active union field.
        const std::string& getMessage() const noexcept {
            //No message to retrieve if there is no error - data is the active union field
            if(okay()) std::unreachable();
            return message;
        }
	//Returns `true` if okay. Otherwise, prints `message` and returns `false`.
	bool report() const noexcept {
	    if(okay()) return true;
	    std::println("Error: {}", message);
	    return false;
	}
        private:
        ErrorTag tag;
        union {
            T data;
            std::string message;
        };
        //Calls the destructor of the active data member, then writes zeroes to the entire space taken up by the object
        void zeroMemory() {
            if(okay()) {
                data.~T();
            }
            else {
                message.~basic_string();
            }
            //Once either destructor has been called, write the zeroes
	    std::memset(static_cast<void*>(this), 0x00, sizeof(Error<T>));
        }
    };

    template<>
    noreturn Error<noreturn>::getData() const noexcept = delete;

    //Returns an `Error<noreturn>` with `key == ALL_OKAY`. Used mainly for the final return value of a function with return type `Error<noreturn>`.
    export Error<noreturn> success() {
	return Error<noreturn>();
    }
}
