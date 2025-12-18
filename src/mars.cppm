module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>
#include <cstdint>

export module mars;
import :renderer;
export import flag_bits;
export import error;
export import heap_array;

namespace mars {

    export Error<noreturn> init() noexcept;
    export void quit() noexcept; 
    export class Game {
        std::string windowName;
        std::string appName;
        Renderer renderer;
        public:
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ~Game() noexcept;
        Error<noreturn> init() noexcept;
        Error<noreturn> draw() noexcept;
        void setFlag(std::uint64_t flag) noexcept;
    };

};
