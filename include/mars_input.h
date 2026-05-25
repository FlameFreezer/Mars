#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <utility>
#include <vector>
#include <cstring>

#include <SDL3/SDL.h>

#include "error.h"
#include "mars_types.h"
#include "mars_heaparray.h"
#include "jsonparser.h"
#include "mars_macros.h"

static std::unordered_map<std::string, SDL_Scancode> initScancodeMap() noexcept {
    std::unordered_map<std::string, SDL_Scancode> map;
    map["space"] = SDL_SCANCODE_SPACE;
    map["a"] = SDL_SCANCODE_A;
    map["d"] = SDL_SCANCODE_D;
    map["l"] = SDL_SCANCODE_L;
    map["p"] = SDL_SCANCODE_P;
    map["w"] = SDL_SCANCODE_W;
    map["s"] = SDL_SCANCODE_S;
    return map;
}
static std::unordered_map<std::string, SDL_GamepadButton> initGamepadButtonMap() noexcept {
    std::unordered_map<std::string, SDL_GamepadButton> map;
    map["south"] = SDL_GAMEPAD_BUTTON_SOUTH;
    map["north"] = SDL_GAMEPAD_BUTTON_NORTH;
    map["east"] = SDL_GAMEPAD_BUTTON_EAST;
    map["west"] = SDL_GAMEPAD_BUTTON_WEST;
    map["dpad right"] = SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    map["dpad left"] = SDL_GAMEPAD_BUTTON_DPAD_LEFT;
    map["dpad up"] = SDL_GAMEPAD_BUTTON_DPAD_UP;
    map["dpad down"] = SDL_GAMEPAD_BUTTON_DPAD_DOWN;
    map["left shoulder"] = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
    return map;
}

static std::unordered_map<std::string, SDL_GamepadAxis> initAxisMap() noexcept {
    std::unordered_map<std::string, SDL_GamepadAxis> map;
    map["leftx"] = SDL_GAMEPAD_AXIS_LEFTX;
    map["lefty"] = SDL_GAMEPAD_AXIS_LEFTY;
    return map;
}

namespace mars {
    constexpr u8 maxScancodes = 5;
    constexpr u8 maxGamepadButtons = 5;
    constexpr u8 maxAxes = SDL_GAMEPAD_AXIS_COUNT;
    constexpr i16 angleToAxisValue = SDL_JOYSTICK_AXIS_MAX / 90.0f;

    struct Mapping {
        SDL_Scancode scancodes[maxScancodes];
        u8 numScancodes = 0;
        SDL_GamepadButton gamepadButtons[maxGamepadButtons];
        u8 numGamepadButtons = 0;
        SDL_GamepadAxis axes[maxAxes];
        float axisValues[maxAxes];
        u8 numAxes = 0;
        bool isValid = false;
    };

    template<class ActionIndex>
    class Input {
        std::vector<Mapping> mMappings;
        bool mPrevGamepadButtonState[SDL_GAMEPAD_BUTTON_COUNT] = {false};
        bool mGamepadButtonState[SDL_GAMEPAD_BUTTON_COUNT] = {false};
        i16 mPrevAxisState[SDL_GAMEPAD_AXIS_COUNT] = {0};
        i16 mAxisState[SDL_GAMEPAD_AXIS_COUNT] = {0};
        SDL_Gamepad* mGamepad = nullptr;
        bool* mPrevKeyState = nullptr;
        const bool* mKeyState = nullptr;
        int mNumKeys = 0;
        Input() noexcept {
            int numGamepads;
            SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
            if(numGamepads != 0) {
                mGamepad = SDL_OpenGamepad(gamepads[0]);
            }
            mKeyState = SDL_GetKeyboardState(&mNumKeys);
            mPrevKeyState = new bool[mNumKeys];
            for(int i = 0; i < mNumKeys; i++) mPrevKeyState[i] = false;
        }
        ~Input() noexcept {
            if(mGamepad != nullptr and SDL_GamepadConnected(mGamepad)) {
                SDL_CloseGamepad(mGamepad);
            }
            if(mPrevKeyState) delete[] mPrevKeyState;
        }
        Input(const Input& other) = delete;
        Input(Input&& other) = delete;
        bool isMappingValid(ActionIndex action) const noexcept {
            return mMappings.size() > std::to_underlying(action) and mMappings[std::to_underlying(action)].isValid;
        }
        public:
        static Input& get() noexcept {
            static Input instance;
            return instance;
        }
        Error<noreturn> loadMappings(const std::string& path, const std::unordered_map<std::string, ActionIndex>& strToIndex) noexcept {
            static const std::unordered_map<std::string, SDL_Scancode> strToScancode = initScancodeMap();
            static const std::unordered_map<std::string, SDL_GamepadButton> strToGamepadButton = initGamepadButtonMap();
            static const std::unordered_map<std::string, SDL_GamepadAxis> strToAxis = initAxisMap();
            std::ifstream input(path, std::ios::ate);
            if(!input.is_open()) {
                return fatal(std::format("Couldn't find an input mappings file at \"{}\"", path));
            }
            HeapArray<char> buff(input.tellg());
            input.seekg(0, std::ios::beg);
            input.read(buff.data(), buff.size());
            input.close();
            TRY_INIT(JSON::Value, mappings, JSON::parse(std::string(buff.data(), buff.size())), noreturn);
            if(mappings.getTag() != JSON::ValueTag::jarray) {
                return fatal("Input mappings file should start with an array");
            }
            for(const JSON::Value& jmapping : mappings.getArray()) {
                Mapping resultMapping;
                resultMapping.isValid = true;
                const JSON::Object& mapping = jmapping.getObject();
                if(mapping.contains("scancodes")) {
                    for(const JSON::Value& scancodeName : mapping.at("scancodes").getArray()) {
                        resultMapping.scancodes[resultMapping.numScancodes++] = strToScancode.at(scancodeName.getString());
                    }
                }
                if(mapping.contains("buttons")) {
                    for(const JSON::Value& buttonName : mapping.at("buttons").getArray()) {
                        resultMapping.gamepadButtons[resultMapping.numGamepadButtons++] = strToGamepadButton.at(buttonName.getString());
                    }
                }
                if(mapping.contains("sticks")) {
                    for(const auto [stickName, stickValue] : mapping.at("sticks").getObject()) {
                        resultMapping.axes[resultMapping.numAxes] = strToAxis.at(stickName);
                        resultMapping.axisValues[resultMapping.numAxes] = stickValue.getNumberAs<float>();
                        resultMapping.numAxes++;
                    }
                }
                //Get the index of the current mapping from its name
                const auto mappingIndex = std::to_underlying(strToIndex.at(mapping.at("tag").getString()));
                //Resize the mappings vector if we need space to fit
                if(mMappings.size() <= mappingIndex) {
                    mMappings.resize(mappingIndex + 1);
                }
                //Write the result mapping
                mMappings[mappingIndex] = resultMapping;
            }
            return success();
        }
        /// Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame.
        /// Returns: void    Nothing
        void update() noexcept {
            std::memcpy(mPrevKeyState, mKeyState, mNumKeys);
            mKeyState = SDL_GetKeyboardState(nullptr);
            int numGamepads = 0;
            SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
            if(numGamepads != 0 and !SDL_GamepadConnected(mGamepad)) {
                mGamepad = SDL_OpenGamepad(gamepads[0]);
            }
            else if(!SDL_GamepadConnected(mGamepad)) {
                SDL_CloseGamepad(mGamepad);
                mGamepad = nullptr;
                std::memset(mPrevGamepadButtonState, SDL_GAMEPAD_BUTTON_COUNT, false);
                std::memset(mGamepadButtonState, SDL_GAMEPAD_BUTTON_COUNT, false);
            }
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
        }
        bool isKeyDown(SDL_Scancode scancode) const noexcept {
            return mKeyState[scancode];
        }
        bool isKeyJustPressed(SDL_Scancode scancode) const noexcept {
            return mKeyState[scancode] and !mPrevKeyState[scancode];
        }
        bool isKeyJustReleased(SDL_Scancode scancode) const noexcept {
            return !mKeyState[scancode] and mPrevKeyState[scancode];
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
        bool isActionDown(ActionIndex action) const noexcept {
            if(!isMappingValid(action)) return false;

            const Mapping& mapping = mMappings[std::to_underlying(action)];
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
        bool isActionJustPressed(ActionIndex action) const noexcept {
            if(!isMappingValid(action)) return false;
            const Mapping& mapping = mMappings[std::to_underlying(action)];
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
        bool isActionJustReleased(ActionIndex action) const noexcept {
            if(!isMappingValid(action)) return false;
            const Mapping& mapping = mMappings[std::to_underlying(action)];
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
    };
}
