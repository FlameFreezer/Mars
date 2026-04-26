module;

#include <cstring>
#include <string>
#include <fstream>
#include <format>
#include <unordered_map>
#include <limits>
#include <sstream>

#include <SDL3/SDL.h>

#include "mars_macros.h"

module input;
import heap_array;

std::unordered_map<std::string, SDL_Scancode> initScancodeMap() noexcept {
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
std::unordered_map<std::string, SDL_GamepadButton> initGamepadButtonMap() noexcept {
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

std::unordered_map<std::string, SDL_GamepadAxis> initAxisMap() noexcept {
    std::unordered_map<std::string, SDL_GamepadAxis> map;
    map["leftx"] = SDL_GAMEPAD_AXIS_LEFTX;
    map["lefty"] = SDL_GAMEPAD_AXIS_LEFTY;
    return map;
}

namespace mars {
    Input::Input() noexcept {
        int numGamepads;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
        if(numGamepads != 0) {
            mGamepad = SDL_OpenGamepad(gamepads[0]);
        }
        mKeyState = SDL_GetKeyboardState(&mNumKeys);
        mPrevKeyState = new bool[mNumKeys];
        for(int i = 0; i < mNumKeys; i++) mPrevKeyState[i] = false;
    }

    Input::~Input() noexcept {
        if(mGamepad != nullptr and SDL_GamepadConnected(mGamepad)) {
            SDL_CloseGamepad(mGamepad);
        }
        if(mPrevKeyState) delete[] mPrevKeyState;
    }

    Input& Input::get() noexcept {
        static Input instance;
        return instance;
    }

    struct InputMapping {
        std::string tag;
        Mapping mapping;
    };

    Error<noreturn> Input::loadMappings(const std::string& path) noexcept {
        static std::unordered_map<std::string, SDL_Scancode> strToScancode = initScancodeMap();
        static std::unordered_map<std::string, SDL_GamepadButton> strToGamepadButton = initGamepadButtonMap();
        static std::unordered_map<std::string, SDL_GamepadAxis> strToAxis = initAxisMap();
        std::ifstream input(path, std::ios::ate);
        if(!input.is_open()) {
            return fatal(std::format("Couldn't find an input mappings file at \"{}\"", path));
        }
        const size_t bufflen = input.tellg();
        char* buff = new char[bufflen];
        input.seekg(0, std::ios::beg);
        input.read(buff, bufflen);
        input.close();
        JSON::Value mappings = JSON::parse(std::string(buff, bufflen));
        if(mappings.getTag() != JSON::ValueTag::jarray) {
            return fatal("Input mappings file should start with an array");
        }
        for(JSON::Value* jmapping : mappings.getData().array) {
            Mapping resultMapping;
            JSON::Object& mapping = jmapping->getData().object;
            if(mapping.contains("scancodes")) {
                for(JSON::Value* scancodeName : mapping["scancodes"]->getData().array) {
                    resultMapping.scancodes[resultMapping.numScancodes++] = strToScancode[scancodeName->getData().str];
                }
            }
            if(mapping.contains("buttons")) {
                for(JSON::Value* buttonName : mapping["buttons"]->getData().array) {
                    resultMapping.gamepadButtons[resultMapping.numGamepadButtons++] = strToGamepadButton[buttonName->getData().str];
                }
            }
            if(mapping.contains("sticks")) {
                for(auto [stickName, stickValue] : mapping["sticks"]->getData().object) {
                    resultMapping.axes[resultMapping.numAxes] = strToAxis[stickName];
                    if(stickValue->getTag() == JSON::ValueTag::jint)  {
                        resultMapping.axisValues[resultMapping.numAxes] = stickValue->getData().integer;
                    }
                    else if(stickValue->getTag() == JSON::ValueTag::jfloat) {
                        resultMapping.axisValues[resultMapping.numAxes] = stickValue->getData().floating;
                    }
                    resultMapping.numAxes++;
                }
            }
            mMappings[mapping["tag"]->getData().str] = resultMapping;
        }
        return success();
    }

    void Input::update() noexcept {
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

    bool Input::isKeyDown(SDL_Scancode scancode) const noexcept {
        return mKeyState[scancode];
    }
    bool Input::isKeyJustPressed(SDL_Scancode scancode) const noexcept {
        return mKeyState[scancode] and !mPrevKeyState[scancode];
    }
    bool Input::isKeyJustReleased(SDL_Scancode scancode) const noexcept {
        return !mKeyState[scancode] and mPrevKeyState[scancode];
    }

    bool Input::isButtonDown(SDL_GamepadButton button) const noexcept {
            return mGamepadButtonState[button];
    }
    bool Input::isButtonJustPressed(SDL_GamepadButton button) const noexcept {
        return mGamepadButtonState[button] and !mPrevGamepadButtonState[button];
    }
    bool Input::isButtonJustReleased(SDL_GamepadButton button) const noexcept {
        return !mGamepadButtonState[button] and mPrevGamepadButtonState[button];
    }

    bool Input::isActionDown(const std::string& action) const noexcept {
        if(!mMappings.contains(action)) return false;
        const Mapping& mapping = mMappings.at(action);
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
    bool Input::isActionJustPressed(const std::string& action) const noexcept {
        if(!mMappings.contains(action)) return false;
        const Mapping& mapping = mMappings.at(action);
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
    bool Input::isActionJustReleased(const std::string& action) const noexcept {
        if(!mMappings.contains(action)) return false;
        const Mapping& mapping = mMappings.at(action);
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
}
