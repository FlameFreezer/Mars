module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

export module mars;
import :renderer;
export import error;
export import array;

namespace mars {
    export Error<noreturn> init() noexcept;
    export void quit() noexcept; 
    export class Game {
        public:
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ~Game() noexcept;
        Error<noreturn> init() noexcept;
        Error<noreturn> draw() noexcept;
        private:
        std::string windowName;
        std::string appName;
        Renderer renderer;
    };
};
