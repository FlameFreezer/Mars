module;

#include <string>
#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

export module mars;
export import types;
export import :camera;
import :renderer;
export import flag_bits;
export import error;
export import heap_array;
export import ecs;

namespace mars {
    export struct Rect2D {
        u64 w;
        u64 h;
    };

    export class Game {
        Renderer* mRenderer;
        std::string mWindowName;
        std::string mAppName;
        std::chrono::steady_clock::time_point mTime;
        std::chrono::nanoseconds mDeltaTime;
        GameFlags mFlags;
        public:
        EntityManager entityManager;
        struct {
            ID rectangle;
        } shapes;
        Camera camera;
        const bool* keyState;
        SDL_Gamepad* gamepad;
        //Constructors - don't do any actual initialization. Call `init` to initialize the game and necessary libraries
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ~Game() noexcept;
        // Initializes the game and used libraries.
        // Returns: Error<noreturn> Nothing, or an error indicating initialization fail
        Error<noreturn> init() noexcept;
        // Delivers the necessary data and calls to the GPU to render and present the scene
        // Returns: Error<noreturn>     Nothing, or an error indicating that drawing failed
        Error<noreturn> draw() noexcept;
        // Sets the flags indicated by the argument
        // Game/RendererFlags flag    Bitmask of flags for the application to set
        // Returns: void    Nothing
        void setFlags(RendererFlags flag) noexcept;
        void setFlags(GameFlags flag) noexcept;
        // Checks if the flags indicated by the argument are set
        // Game/RendererFlags flag  Bitmask of flags for the application to set
        bool hasFlags(RendererFlags flag) noexcept;
        bool hasFlags(GameFlags flag) noexcept;
        // Gets the epoch time for the start of the frame. Make sure to call `updateTime` before any calls to this within the current frame.
        // Returns:     The epoch time of the start of the current frame, in nanoseconds
        std::chrono::steady_clock::time_point::rep frameTime() const noexcept;
        // Gets the time between the last frame and the current frame. Make sure to call `updateTime` before any calls to this within the current frame.
        // Returns:     The time between the last frame and the current frame, in nanoseconds
        std::chrono::nanoseconds::rep deltaTimeNanoseconds() const noexcept;
        // Gets the time between the last frame and the current frame. Make sure to call `updateTime` before any calls to this within the current frame.
        // Returns: float   The time between the last frame and the current frame, in seconds
        std::chrono::duration<float, std::chrono::seconds::period>::rep deltaTime() const noexcept;
        // Updates the `time` and `deltaTime` private class members to reflect the current frame. Should be called at the start of the current frame.
        // Returns: void    Nothing
        void updateTime() noexcept;
        // Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame.
        // Returns: void    Nothing
        void updateKeyState() noexcept;
        // Loads the mesh located at `path`.
        // std::string const& path  The file path of the model to load.
        // Returns: Error<ID>   The ID of the mesh, or an error indicating that loading failed.
        Error<ID> loadMesh(const std::string& path) noexcept;
        // Loads the texture located at `path`.
        // const std::string& path  The file path of the texture to load.
        // Returns: Error<ID>   The ID of the texture, or an error indicating that loading failed.
        Error<ID> loadTexture(const std::string& path) noexcept;
        bool checkCollision(const Transform& t1, const Transform& t2) const noexcept;
        Error<Transform*> transform(Entity e) noexcept;
        Error<Physics*> physics(Entity e) noexcept;
        void setMesh(Entity e, ID id) noexcept;
        void setTexture(Entity e, ID id) noexcept;
    };
};
