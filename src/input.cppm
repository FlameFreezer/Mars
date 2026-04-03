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

    struct Mapping {
        SDL_Scancode scancodes[maxScancodes];
        u8 numScancodes = 0;
        SDL_GamepadButton gamepadButtons[maxGamepadButtons];
        u8 numGamepadButtons = 0;
    };

    export class Input {
        bool* mPrevKeyState = nullptr;
        const bool* mKeyState = nullptr;
        int mNumKeys = 0;
        std::unordered_map<std::string, Mapping> mMappings;
        static std::unordered_map<std::string, SDL_Scancode> strToScancode;
        static std::unordered_map<std::string, SDL_GamepadButton> strToGamepadButton;
        public:
        SDL_Gamepad* gamepad = nullptr;
        Input() noexcept;
        ~Input() noexcept;
        Error<noreturn> loadMappings(const std::string& path) noexcept;
        /// Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame.
        /// Returns: void    Nothing
        void update() noexcept;
        bool isKeyDown(SDL_Scancode scancode) const noexcept;
        bool isKeyJustPressed(SDL_Scancode scancode) const noexcept;
        bool isKeyJustReleased(SDL_Scancode scancode) const noexcept;
        bool isActionDown(const std::string& action) const noexcept;
        bool isActionJustPressed(const std::string& action) const noexcept;
        bool isActionJustReleased(const std::string& action) const noexcept;
    };
}
