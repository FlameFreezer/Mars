#define MARS_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

namespace mars {
    enum class ErrorKey : uint32_t {
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

    using noreturn = int32_t;

    template <class T>
    union ErrorUnion {
        T data;
        std::string message;
        ErrorUnion() noexcept : data(T{0}) {}
        ErrorUnion(const T& inData) noexcept : data(inData) {}
        ErrorUnion(const std::string& inMessage) noexcept : message(inMessage) {}
        ErrorUnion(const ErrorUnion<T>& errU) noexcept : message(errU.message) {}
        ~ErrorUnion() noexcept {}
    };

    template <class T>
    class [[nodiscard("Potentially unhandled error value")]] Error {
        public:
        Error() noexcept : key(ErrorKey::ALL_OKAY), content() {}
        Error(const T& data) noexcept : key(ErrorKey::ALL_OKAY), content(data) {}
        Error(ErrorKey inKey, const std::string& message) noexcept : key(inKey), content(message) {}
        Error(const Error<T>& err) noexcept : key(err.key), content(err.content) {}
        Error(const Error<T>&& err) noexcept : key(err.key), content(std::move(err.content)) {}
        Error<T>& operator=(const Error<T>& rhs) {
            key = rhs.key;
            if(key == ErrorKey::ALL_OKAY) {
                content.data = rhs.content.data;
            }
            else {
                content.message = rhs.content.message;
            }
            return *this;
        }
        ~Error() {}
        ErrorKey getKey()  {
            return key;
        }
        bool okay()  {
            return key == ErrorKey::ALL_OKAY;
        }
        T getData()  {
            return content.data;
        }
        std::string getMessage()  {
            return content.message;
        }
        private:
        ErrorKey key;
        ErrorUnion<T> content;
    };

    Error<noreturn> success();

    //Make sure to declare an Error<T> named procResult inside your function before using this macro
    #define MARS_TRY(proc) procResult = proc;\
    if(!procResult.okay()) return procResult

    Error<noreturn> init();
    int quit() noexcept;

    class Renderer {
        public:
        Renderer() noexcept;
        virtual ~Renderer() noexcept;
        Error<noreturn> init(const std::string& name);
        private:
        VkInstance instance;
        SDL_Window* window;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkSwapchainKHR swapchain;
    };

    class Game {
        public:
        Game() = delete;
        Game(Error<noreturn>& result) noexcept;
        Game(Error<noreturn>& result, const std::string& name) noexcept;
        virtual ~Game() noexcept;
        Error<noreturn> init(const std::string& appName);
        private:
        std::string windowName;
        std::string appName;
        Renderer renderer;
    };
};