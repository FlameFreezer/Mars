module;

#include <string>
#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

export module mars;
export import :camera;
import :renderer;
export import flag_bits;
export import error;
export import heap_array;

namespace mars {
    export class Game {
        Renderer renderer;
        std::string windowName;
        std::string appName;
        std::chrono::steady_clock::time_point time;
        std::chrono::nanoseconds deltaTime;
        GameFlags flags;
        public:
        Camera camera;
        bool const* keyState;
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ~Game() noexcept;
        Error<noreturn> init() noexcept;
        Error<noreturn> draw() noexcept;
        void setRendererFlags(RendererFlags flag) noexcept;
        void setFlags(GameFlags flag) noexcept;
        bool rendererHasFlags(RendererFlags flag) noexcept;
        bool hasFlags(GameFlags flag) noexcept;
        std::chrono::steady_clock::time_point getFrameTime() const noexcept;
        std::chrono::nanoseconds::rep getDeltaTime() const noexcept;
        std::chrono::duration<float, std::chrono::seconds::period>::rep getDeltaTimeSeconds() const noexcept;
        void updateTime() noexcept;
        void updateKeyState() noexcept;
    };

};
