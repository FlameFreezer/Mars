module;

#include <string>
#include <chrono>
#include <vector>

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
export import multimap;

namespace mars {
    export struct Rect2D {
        std::uint64_t w;
        std::uint64_t h;
    };

    export class Game {
        Renderer* renderer;
        std::string windowName;
        std::string appName;
        std::chrono::steady_clock::time_point time;
        std::chrono::nanoseconds deltaTime;
        GameFlags flags;
        std::vector<ID> objectsToUpdate; 
        public:
        struct {
            ID const square = 0;
        } shapes;
        Objects objects;
        Camera camera;
        bool const* keyState;
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
        std::chrono::steady_clock::time_point::rep getFrameTime() const noexcept;
        // Gets the time between the last frame and the current frame. Make sure to call `updateTime` before any calls to this within the current frame.
        // Returns:     The time between the last frame and the current frame, in nanoseconds
        std::chrono::nanoseconds::rep getDeltaTime() const noexcept;
        // Gets the time between the last frame and the current frame. Make sure to call `updateTime` before any calls to this within the current frame.
        // Returns: float   The time between the last frame and the current frame, in seconds
        std::chrono::duration<float, std::chrono::seconds::period>::rep getDeltaTimeSeconds() const noexcept;
        // Updates the `time` and `deltaTime` private class members to reflect the current frame. Should be called at the start of the current frame.
        // Returns: void    Nothing
        void updateTime() noexcept;
        // Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame.
        // Returns: void    Nothing
        void updateKeyState() noexcept;
        // Resizes the window to the dimensions provided by the arguments. CURRENTLY UNTESTED
        // std::unit32_t width  The new width of the window
        // std::uint32_t height The new height of the window
        // Returns: void    Nothing
        void resizeWindow(std::uint32_t width, std::uint32_t height) noexcept;
        // Loads the mesh located at `path`.
        // std::string const& path  The file path of the model to load.
        // Returns: Error<ID>   The ID of the mesh, or an error indicating that loading failed.
        Error<ID> loadMesh(std::string const& path) noexcept;
        // Loads the texture located at `path`.
        // std::string const& path  The file path of the texture to load.
        // Returns: Error<ID>   The ID of the texture, or an error indicating that loading failed.
        Error<ID> loadTexture(std::string const& path) noexcept;
        // Creates an object with the provided mesh, texture, position, and size.
        // ID mesh  The ID of the mesh to use.
        // ID texture   The ID of the texture to use.
        // glm::vec3 const& pos The position of the object.
        // glm::vec3 const& scale   The size of the object.
        Error<ID> createObject(ID mesh, ID texture, glm::vec3 const& pos, glm::vec3 const& scale) noexcept;
        // Returns the dimensions of the window.
        // Returns: Rect2D  The dimensions of the window.
        Rect2D getWindowDimensions() const noexcept;
        Error<noreturn> setPosition(ID object, glm::vec3 const& pos) noexcept;
        Error<noreturn> addPosition(ID object, glm::vec3 const& pos) noexcept;
        Error<glm::vec3> getPosition(ID object) const noexcept;
        Error<noreturn> setScale(ID object, glm::vec3 const& pos) noexcept;
        Error<glm::vec3> getScale(ID object) const noexcept;
        bool checkCollision(ID o1, ID o2) const noexcept;
        Error<noreturn> setTexture(ID object, ID texture) noexcept;
    };
};
