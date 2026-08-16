#pragma once

#include "SDL3/SDL_gamepad.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cstring>
#include <mutex>

#include <SDL3/SDL.h>

#include <error.h>
#include <mars_types.h>
#include <mars_heaparray.h>
#include <jsonparser.h>
#include <timer/mars_timer.h>

static const std::unordered_map<std::string, SDL_Scancode> strToScancode = {
    {"space", SDL_SCANCODE_SPACE},
    {"a", SDL_SCANCODE_A},
    {"d", SDL_SCANCODE_D},
    {"l", SDL_SCANCODE_L},
    {"p", SDL_SCANCODE_P},
    {"w", SDL_SCANCODE_W},
    {"s", SDL_SCANCODE_S},
};

static const std::unordered_map<std::string, SDL_GamepadButton> strToGamepadButton = {
    {"south", SDL_GAMEPAD_BUTTON_SOUTH},
    {"north", SDL_GAMEPAD_BUTTON_NORTH},
    {"east", SDL_GAMEPAD_BUTTON_EAST},
    {"west", SDL_GAMEPAD_BUTTON_WEST},
    {"dpad right", SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
    {"dpad left", SDL_GAMEPAD_BUTTON_DPAD_LEFT},
    {"dpad up", SDL_GAMEPAD_BUTTON_DPAD_UP},
    {"dpad down", SDL_GAMEPAD_BUTTON_DPAD_DOWN},
    {"left shoulder", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
};

static const std::unordered_map<std::string, SDL_GamepadAxis> strToGamepadAxis = {
    {"leftx", SDL_GAMEPAD_AXIS_LEFTX},
    {"lefty", SDL_GAMEPAD_AXIS_LEFTY},
};

namespace mars {
    constexpr u8 maxScancodes = 5;
    constexpr u8 maxGamepadButtons = 5;
    constexpr u8 maxAxes = SDL_GAMEPAD_AXIS_COUNT;
    // Put axis values on a scale [-90,90] degrees
    constexpr i16 angleToAxisValue = SDL_JOYSTICK_AXIS_MAX / 90.0f;

    struct Mapping {
        SDL_Scancode scancodes[maxScancodes];
        u8 numScancodes = 0;
        SDL_GamepadButton gamepadButtons[maxGamepadButtons];
        u8 numGamepadButtons = 0;
        SDL_GamepadAxis axes[maxAxes];
        float axisValues[maxAxes] = { 0.0f };
        u8 numAxes = 0;
        bool isValid = false;
        bool isBuffered = false;
        u32 bufferIndex{};
    };

    struct ActionBuffer {
        bool isActive{ false };
        mars::Timer timer{};
        u16 action{};
    };

    class Input {
        public:
        static Input& get() noexcept;

        Error<noreturn> loadMappings(const std::string& path, const std::unordered_map<std::string, u16>& strToIndex) noexcept; 

        template<class ActionIndex>
        const Mapping& getMapping(ActionIndex action) const noexcept {
            return mMappings[std::to_underlying(action)];
        }
        template<>
        const Mapping& getMapping(u16 action) const noexcept {
            return mMappings[action];
        }
        /// Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame AFTER all events have been processed.
        /// Returns: void    Nothing
        template<class DeltaType>
        void update(DeltaType deltaTime) noexcept {
            std::memcpy(mPrevKeyState, mCurrentKeyState, mNumKeys);
            std::memcpy(mCurrentKeyState, mKeyState, mNumKeys);

            int numGamepads = 0;
            SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
            //If a gamepad is detected but we don't have any connected, open the first gamepad
            if(numGamepads != 0 and !SDL_GamepadConnected(mGamepad)) {
                mGamepad = SDL_OpenGamepad(gamepads[0]);
            }
            //If no gamepads are connected, close the first gamepad
            else if(!SDL_GamepadConnected(mGamepad)) {
                SDL_CloseGamepad(mGamepad);
                mGamepad = nullptr;
                std::memset(mPrevGamepadButtonState, SDL_GAMEPAD_BUTTON_COUNT, false);
                std::memset(mGamepadButtonState, SDL_GAMEPAD_BUTTON_COUNT, false);
            }
            //Otherwise, update the gamepad states
            else {
                for(u64 i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; i++) {
                    mPrevGamepadButtonState[i] = mGamepadButtonState[i];
                    mGamepadButtonState[i] = SDL_GetGamepadButton(mGamepad, static_cast<SDL_GamepadButton>(i));
                }
                for(u64 i = 0; i < SDL_GAMEPAD_AXIS_COUNT; i++) {
                    mPrevAxisState[i] = mAxisState[i];
                    mAxisState[i] = SDL_GetGamepadAxis(mGamepad, static_cast<SDL_GamepadAxis>(i));
                }
            }
            SDL_free(gamepads);
            SDL_GetMouseState(&mMouseX, &mMouseY);
            SDL_GetRelativeMouseState(&mMouseDx, &mMouseDy);

            //Update action buffers
            for (ActionBuffer& buffer : mActionBuffers) {
                if (isActionJustPressed(buffer.action)) {
                    buffer.isActive = true;
                    // If no waitTime was specified (or it was specified to zero), the action's
                    //    buffer is held until it is next consumed
                    if (buffer.timer.waitTime() != 0.0f) {
						buffer.timer.stop();
						buffer.timer.start();
                    }
                }
                else if (buffer.timer.status() != TimerStatus::stopped) {
                    buffer.timer.update(deltaTime);
                    if (buffer.timer.status() == TimerStatus::stopped) {
                        buffer.isActive = false;
                    }
                }
            }
        }
        Rect getMousePosition() const noexcept {
            return { mMouseX, mMouseY };
        }
        Rect getMouseDelta() const noexcept {
            return { mMouseDx, mMouseDy };
        }
        bool isKeyDown(SDL_Scancode scancode) const noexcept {
            return mCurrentKeyState[scancode];
        }
        bool isKeyJustPressed(SDL_Scancode scancode) const noexcept {
            return mCurrentKeyState[scancode] and !mPrevKeyState[scancode];
        }
        bool isKeyJustReleased(SDL_Scancode scancode) const noexcept {
            return !mCurrentKeyState[scancode] and mPrevKeyState[scancode];
        }
        bool isButtonDown(SDL_GamepadButton button) const noexcept {
            return mGamepadButtonState[button];
        }
        bool isButtonJustPressed(SDL_GamepadButton button) const noexcept {
            return mGamepadButtonState[button] and !mPrevGamepadButtonState[button];
        }
        bool isButtonJustReleased(SDL_GamepadButton button) const noexcept {
            return !mGamepadButtonState[button] and mPrevGamepadButtonState[button];
        }

        template<class ActionIndex>
        bool isActionDown(ActionIndex action) const noexcept {
            if(!isMappingValid(action)) return false;

            const Mapping& mapping = getMapping(action);
            for(u8 i = 0; i < mapping.numScancodes; i++) {
                if(isKeyDown(mapping.scancodes[i])) return true;
            }
            for(u8 i = 0; i < mapping.numGamepadButtons; i++) {
               if(isButtonDown(mapping.gamepadButtons[i])) return true;
            }
            for(u8 i = 0; i < mapping.numAxes; i++) {
                const i16 val = mapping.axisValues[i] * angleToAxisValue;
                if(val <= 0) {
                    if(mAxisState[mapping.axes[i]] <= val) return true;
                }
                else {
                    if(mAxisState[mapping.axes[i]] >= val) return true;
                }
            }
            return false;
        }
        template<class ActionIndex>
        bool isActionJustPressed(ActionIndex action) const noexcept {
            if(!isMappingValid(action)) return false;
            const Mapping& mapping = getMapping(action);
            for(u8 i = 0; i < mapping.numScancodes; i++) {
                if(isKeyJustPressed(mapping.scancodes[i])) return true;
            }
            for(u8 i = 0; i < mapping.numGamepadButtons; i++) {
                if(isButtonJustPressed(mapping.gamepadButtons[i])) return true;
            }
            for(u8 i = 0; i < mapping.numAxes; i++) {
                const i16 val = mapping.axisValues[i] * angleToAxisValue;
                if(val <= 0) {
                    if(mAxisState[mapping.axes[i]] <= val and mPrevAxisState[mapping.axes[i]] > val) return true;
                }
                else {
                    if(mAxisState[mapping.axes[i]] >= val and mPrevAxisState[mapping.axes[i]] < val) return true;
                }
            }
            return false;
        }
        template<class ActionIndex>
        bool isActionJustReleased(ActionIndex action) const noexcept {
            if(!isMappingValid(action)) return false;
            const Mapping& mapping = getMapping(action);
            for(u8 i = 0; i < mapping.numScancodes; i++) {
                if(isKeyJustReleased(mapping.scancodes[i])) return true;
            }
            for(u8 i = 0; i < mapping.numGamepadButtons; i++) {
                if(isButtonJustReleased(mapping.gamepadButtons[i])) return true;
            }
            for(u8 i = 0; i < mapping.numAxes; i++) {
                const i16 val = mapping.axisValues[i] * angleToAxisValue;
                if(val <= 0) {
                    if(mAxisState[mapping.axes[i]] > val and mPrevAxisState[mapping.axes[i]] <= val) return true;
                }
                else {
                    if(mAxisState[mapping.axes[i]] < val and mPrevAxisState[mapping.axes[i]] >= val) return true;
                }
            }

            return false;
        }
        // Checks if an action was buffered. Returns false if the action's mapping is invalid OR if 
        //   the action does not have a buffer OR if the buffer is inactive.
        // Arguments:   action    The action to check
        // Returns: bool
        template<class ActionIndex>
	    bool isActionBuffered(ActionIndex action) const noexcept {
            if (!isMappingValid(action)) return false;
            const Mapping& mapping = getMapping(action);
            if (!mapping.isBuffered) return false;
            return mActionBuffers[mapping.bufferIndex].isActive;
        }

        // Checks if an action was buffered, then unbuffers the action if it was. Returns false if 
        //   the action's mapping is invalid OR if the action does not have a buffer OR if the 
        //   buffer is inactive.
        // Postconditions:  isActionBuffered(action) == false for this action
        //                  consumeActionBuffer(action) == false for this action
        // Arguments:   action  The action to check and unbuffer
        // Returns: bool
        template<class ActionIndex>
        bool consumeActionBuffer(ActionIndex action) noexcept {
            if (!isMappingValid(action)) return false;
            const Mapping& mapping = getMapping(action);
            if (!mapping.isBuffered) return false;
            const bool result = mActionBuffers[mapping.bufferIndex].isActive;
			mActionBuffers[mapping.bufferIndex].isActive = false;
            return result;
        }
        Input(const Input&) = delete;
        Input(Input&&) = delete;
        Input& operator=(const Input&) = delete;
        Input& operator=(Input&&) = delete;
    private:
        static std::mutex mutex;
        std::vector<Mapping> mMappings{};
        bool mPrevGamepadButtonState[SDL_GAMEPAD_BUTTON_COUNT] = { false };
        bool mGamepadButtonState[SDL_GAMEPAD_BUTTON_COUNT] = { false };
        i16 mPrevAxisState[SDL_GAMEPAD_AXIS_COUNT] = { 0 };
        i16 mAxisState[SDL_GAMEPAD_AXIS_COUNT] = { 0 };
        float mMouseX{};
        float mMouseY{};
        float mMouseDx{};
        float mMouseDy{};
        SDL_Gamepad* mGamepad = nullptr;
        bool* mPrevKeyState = nullptr;
        bool* mCurrentKeyState = nullptr;
        const bool* mKeyState = nullptr;
        int mNumKeys = 0;
        std::vector<ActionBuffer> mActionBuffers{};

        Input() noexcept;

        ~Input() noexcept;

        template<class ActionIndex>
        bool isMappingValid(ActionIndex action) const noexcept {
            return mMappings.size() > std::to_underlying(action) and mMappings[std::to_underlying(action)].isValid;
        }
        template<>
        bool isMappingValid(u16 action) const noexcept {
            return mMappings.size() > action and mMappings[action].isValid;
        }
    };
}
