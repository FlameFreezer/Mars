module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>
#include <chrono>

export module mars;
export import :camera;
import :renderer;
export import flag_bits;
export import error;
export import heap_array;

namespace mars {

    export class Game {
        std::string windowName;
        std::string appName;
        Renderer renderer;
        std::chrono::steady_clock::time_point prevTime;
        GameFlags flags;
        Camera camera;
        public:
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ~Game() noexcept;
        Error<noreturn> init() noexcept;
        Error<noreturn> draw() noexcept;
        void setRendererFlags(RendererFlags flag) noexcept;
        void setFlags(GameFlags flag) noexcept;
        bool rendererHasFlags(RendererFlags flag) noexcept;
        bool hasFlags(GameFlags flag) noexcept;
    };

};
