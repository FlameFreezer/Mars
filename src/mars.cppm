module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

export module mars;
export import :error;

namespace mars {
    static int const MAX_CONCURRENT_FRAMES = 2;

    export Error<noreturn> init() noexcept;
    export int quit() noexcept;
    export Error<noreturn> run() noexcept;

    export class Renderer {
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

    export class Game {
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