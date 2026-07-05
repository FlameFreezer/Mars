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
        float axisValues[maxAxes] = { 0.0f };
        u8 numAxes = 0;
        bool isValid = false;
    };

    template<class ActionIndex>
    class Input {
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
        const bool* mKeyState = nullptr;
        int mNumKeys = 0;

		const std::unordered_map<std::string, SDL_Scancode> strToScancode = initScancodeMap();
		const std::unordered_map<std::string, SDL_GamepadButton> strToGamepadButton = initGamepadButtonMap();
		const std::unordered_map<std::string, SDL_GamepadAxis> strToAxis = initAxisMap();

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
            std::ifstream input(path, std::ios::ate);
            if(!input.is_open()) {
                FATAL(std::format("Couldn't find an input mappings file at \"{}\"", path));
            }
            HeapArray<char> buff(input.tellg());
            input.seekg(0, std::ios::beg);
            input.read(buff.data(), buff.size());
            input.close();
            TRY_INIT(JSON::Value, mappings, JSON::parse(std::string(buff.data(), buff.size())));
            if(mappings.getType() != JSON::Type::jarray) {
                FATAL("Input mappings file should start with an array");
            }
            for(const JSON::Value& jmapping : mappings.getArray().value()) {
                Mapping resultMapping{};
                resultMapping.isValid = true;
                if(jmapping.getType() != JSON::Type::jobject) {
                    FATAL("Entries inside mappings JSON array should be objects");
                }
                const JSON::Object& mapping = jmapping.getObject().value();
                if(mapping.contains("scancodes")) {
                    if(mapping.at("scancodes").getType() != JSON::Type::jarray) {
                        FATAL("Mapping field \"scancodes\" should hold a JSON array");
                    }
                    for(const JSON::Value& scancodeName : mapping.at("scancodes").getArray().value()) {
                        if(scancodeName.getType() != JSON::Type::jstring) {
                            FATAL("Scancode name should be a string!");
                        }
                        resultMapping.scancodes[resultMapping.numScancodes++] = strToScancode.at(scancodeName.getString().value());
                    }
                }
                if(mapping.contains("buttons")) {
                    if(mapping.at("buttons").getType() != JSON::Type::jarray) {
                        FATAL("Mapping field \"buttons\" should hold a JSON array");
                    }
                    for(const JSON::Value& buttonName : mapping.at("buttons").getArray().value()) {
                        if(buttonName.getType() != JSON::Type::jstring) {
                            FATAL("Button name should be a string!");
                        }
                        resultMapping.gamepadButtons[resultMapping.numGamepadButtons++] = strToGamepadButton.at(buttonName.getString().value());
                    }
                }
                if(mapping.contains("sticks")) {
                    if(mapping.at("sticks").getType() != JSON::Type::jobject) {
                        FATAL("Mapping field \"sticks\" should hold a JSON object");
                    }
                    for(const auto [stickName, stickValue] : mapping.at("sticks").getObject().value()) {
                        resultMapping.axes[resultMapping.numAxes] = strToAxis.at(stickName);
                        if(stickValue.getType() != JSON::Type::jnumber) {
                            FATAL("Joystick values should be numbers");
                        }
                        resultMapping.axisValues[resultMapping.numAxes] = stickValue.getNumberAs<float>().value();
                        resultMapping.numAxes++;
                    }
                }
                //Get the index of the current mapping from its name
                if(mapping.at("tag").getType() != JSON::Type::jstring) {
                    FATAL("Mapping field \"tag\" should hold a JSON string");
                }
                const auto mappingIndex = std::to_underlying(strToIndex.at(mapping.at("tag").getString().value()));
                //Resize the mappings vector if we need space to fit
                if(mMappings.size() <= mappingIndex) {
                    mMappings.resize(mappingIndex + 1);
                }
                //Write the result mapping
                mMappings[mappingIndex] = resultMapping;
            }
            return SUCCESS;
        }
        /// Updates the `keyState` public class member to reflect the current state of keyboard inputs. Should be called once at the start of the current frame.
        /// Returns: void    Nothing
        void update() noexcept {
            std::memcpy(mPrevKeyState, mKeyState, mNumKeys);
            mKeyState = SDL_GetKeyboardState(nullptr);

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
            SDL_GetMouseState(&mMouseX, &mMouseY);
            SDL_GetRelativeMouseState(&mMouseDx, &mMouseDy);
        }
        Rect getMousePosition() const noexcept {
            return { mMouseX, mMouseY };
        }
        Rect getMouseDelta() const noexcept {
            return { mMouseDx, mMouseDy };
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
