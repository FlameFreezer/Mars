module;

#include <string>
#include <unordered_map>

#include <SDL3/SDL.h>

export module input;
import heap_array;
import error;
import types;

namespace mars {

    export constexpr u8 maxScancodes = 5;
    export constexpr u8 maxGamepadButtons = 5;
    export constexpr u8 maxAxes = SDL_GAMEPAD_AXIS_COUNT;
    export constexpr i16 angleToAxisValue = SDL_JOYSTICK_AXIS_MAX / 90.0f;

    struct Mapping {
        SDL_Scancode scancodes[maxScancodes];
        u8 numScancodes = 0;
        SDL_GamepadButton gamepadButtons[maxGamepadButtons];
        u8 numGamepadButtons = 0;
        SDL_GamepadAxis axes[maxAxes];
        float axisValues[maxAxes];
        u8 numAxes = 0;
    };

    export class Input {
        std::unordered_map<std::string, Mapping> mMappings;
        bool mPrevGamepadButtonState[SDL_GAMEPAD_BUTTON_COUNT] = {false};
        bool mGamepadButtonState[SDL_GAMEPAD_BUTTON_COUNT] = {false};
        i16 mPrevAxisState[SDL_GAMEPAD_AXIS_COUNT] = {0};
        i16 mAxisState[SDL_GAMEPAD_AXIS_COUNT] = {0};
        SDL_Gamepad* mGamepad = nullptr;
        bool* mPrevKeyState = nullptr;
        const bool* mKeyState = nullptr;
        int mNumKeys = 0;

        Input() noexcept;
        ~Input() noexcept;
        Input(const Input& other) = delete;
        Input(Input&& other) = delete;

        public:
        static Input& get() noexcept;
        Error<noreturn> loadMappings(const std::string& path) noexcept;
        /// Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame.
        /// Returns: void    Nothing
        void update() noexcept;
        bool isKeyDown(SDL_Scancode scancode) const noexcept;
        bool isKeyJustPressed(SDL_Scancode scancode) const noexcept;
        bool isKeyJustReleased(SDL_Scancode scancode) const noexcept;
        bool isButtonDown(SDL_GamepadButton button) const noexcept;
        bool isButtonJustPressed(SDL_GamepadButton button) const noexcept;
        bool isButtonJustReleased(SDL_GamepadButton button) const noexcept;
        bool isActionDown(const std::string& action) const noexcept;
        bool isActionJustPressed(const std::string& action) const noexcept;
        bool isActionJustReleased(const std::string& action) const noexcept;
    };
}
