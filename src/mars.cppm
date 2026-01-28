module;

#include <string>
#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

export module mars;
export import :camera;
import :renderer;
export import flag_bits;
export import error;
export import heap_array;
export import object;

namespace mars {
    export class Game {
        Renderer* renderer;
        std::string windowName;
        std::string appName;
        std::chrono::steady_clock::time_point time;
        std::chrono::nanoseconds deltaTime;
        GameFlags flags;
        public:
        Objects objects;
        Camera camera;
        bool const* keyState;
        SDL_Gamepad* gamepad;
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ~Game() noexcept;
        Error<noreturn> init() noexcept;
        Error<noreturn> draw() noexcept;
        void setFlags(RendererFlags flag) noexcept;
        void setFlags(GameFlags flag) noexcept;
        bool hasFlags(RendererFlags flag) noexcept;
        bool hasFlags(GameFlags flag) noexcept;
        std::chrono::steady_clock::time_point::rep getFrameTime() const noexcept;
        std::chrono::nanoseconds::rep getDeltaTime() const noexcept;
        std::chrono::duration<float, std::chrono::seconds::period>::rep getDeltaTimeSeconds() const noexcept;
        void updateTime() noexcept;
        void updateKeyState() noexcept;
        Error<std::size_t> loadMesh(std::string const& path) noexcept;
        Error<std::size_t> loadTexture(std::string const& path) noexcept;
        Error<std::size_t> createObject(std::size_t mesh, std::size_t texture, glm::vec3 const& pos) noexcept;
    };

};
