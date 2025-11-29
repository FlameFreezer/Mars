#include <cstdint>

namespace mars {
    class noreturn {};

    enum class ErrorTag : uint32_t {
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
    };

    template <class T>
    class [[nodiscard("Potentially unhandled error value")]] Error {
        public:
        Error() noexcept : tag(ErrorTag::ALL_OKAY), data() {}
        Error(const T& inData) noexcept : tag(ErrorTag::ALL_OKAY), data(inData) {}
        Error(T&& inData) noexcept : tag(ErrorTag::ALL_OKAY), data(std::move(inData)) {}
        Error(ErrorTag inTag, const std::string& inMessage) noexcept : tag(inTag), message(inMessage) {}
        bool okay() const noexcept {
            return tag == ErrorTag::ALL_OKAY;
        }
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
            tag = rhs.tag;
            if(rhs.okay()) {
                data = rhs.data;
            }
            else {
                message = rhs.message;
            }
            return *this;
        }
        Error<T>& operator=(Error<T>&& rhs) noexcept {
            tag = rhs.tag;
            if(rhs.okay()) {
                data = std::move(rhs.data);
            }
            else {
                message = std::move(rhs.message);
            }
            return *this;
        }
        ~Error() noexcept {
            if(!okay()) {
                message.~basic_string();
            }
            else {
                data.~T();
            }
        }
        ErrorTag getTag() const noexcept {
            return tag;
        }
        T getData() const noexcept {
            //No data to retrieve if there has been an error - message is the active union field
            if(!okay()) std::unreachable();
            return data;
        }
        const std::string& getMessage() const noexcept {
            //No message to retrieve if there is no error - data is the active union field
            if(okay()) std::unreachable();
            return message;
        }
        private:
        ErrorTag tag;
        union {
            T data;
            std::string message;
        };
    };

    template<>
    noreturn Error<noreturn>::getData() const noexcept = delete;

    Error<noreturn> success();

    //Make sure to declare an Error<T> named procResult inside your function before using this macro
    #define MARS_TRY(proc) procResult = proc;\
    if(!procResult.okay()) return procResult

}