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

    Error<InputMapping> readInputMapping(std::istringstream& block, char* buff, u16 buffSize) noexcept {
        static const std::unordered_map<std::string, SDL_Scancode> strToScancode = initScancodeMap();
        static const std::unordered_map<std::string, SDL_GamepadButton> strToGamepadButton = initGamepadButtonMap();
        static const std::unordered_map<std::string, SDL_GamepadAxis> strToAxis = initAxisMap();
        InputMapping inputMapping;
        while(!block.eof()) {
            //Ignore up to the first parenthesis of the property
            block.ignore(std::numeric_limits<std::streamsize>::max(), '\"');  
            if(block.eof()) break;
            //Read in the whole property name into the buffer
            block.getline(buff, buffSize, '\"');
            if(strcmp(buff, "tag") == 0) {
                //Ignore until the name of the tag
                block.ignore(std::numeric_limits<std::streamsize>::max(), '\"');
                //Read in the tag name
                block.getline(buff, buffSize, '\"');
                inputMapping.tag = std::string(buff, block.gcount() - 1);
            }
            else if(strcmp(buff, "scancodes") == 0) {
                //Ignore until the start of the array
                block.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                //Read in the whole array
                block.getline(buff, buffSize, ']');
                //Put the array into a string stream
                std::istringstream array(std::string(buff, block.gcount() - 1));
                while(true) {
                    //Ignore until the start of the next scancode name
                    array.ignore(std::numeric_limits<std::streamsize>::max(), '\"');
                    //If there isn't another scancode name, this safely quits
                    if(array.eof()) break;
                    //Read in the name, discarding the close quote
                    array.getline(buff, buffSize, '\"');
                    const std::string codename(buff, array.gcount() - 1);
                    if(!strToScancode.contains(codename)) {
                        return fatal<InputMapping>(std::format("Invalid scancode name \"{}\"", codename));
                    }
                    SDL_Scancode scancode = strToScancode.at(codename);
                    inputMapping.mapping.scancodes[inputMapping.mapping.numScancodes++] = scancode;
                    if(inputMapping.mapping.numScancodes == maxScancodes) break;
                }
            }
            else if(strcmp(buff, "buttons") == 0) {
                block.ignore(std::numeric_limits<std::streamsize>::max(), '[');
                //Read in the whole array
                block.getline(buff, buffSize, ']');
                std::istringstream array(std::string(buff, block.gcount() - 1));
                while(true) {
                    //Ignore until the next button name
                    array.ignore(std::numeric_limits<std::streamsize>::max(), '\"');
                    //If there isn't another button name, this safely quits
                    if(array.eof()) break;
                    //Read in the name, discarding the close quote
                    array.getline(buff, buffSize, '\"');
                    const std::string buttonname(buff, array.gcount() - 1);
                    if(!strToGamepadButton.contains(buttonname)) {
                        return fatal<InputMapping>(std::format("Invalid gamepad button name \"{}\"", buttonname));
                    }
                    SDL_GamepadButton button = strToGamepadButton.at(buttonname);
                    inputMapping.mapping.gamepadButtons[inputMapping.mapping.numGamepadButtons++] = button;
                    if(inputMapping.mapping.numGamepadButtons == maxGamepadButtons) break;
                }
            }
            else if(strcmp(buff, "sticks") == 0) {
                block.ignore(std::numeric_limits<std::streamsize>::max(), '{');
                block.getline(buff, buffSize, '}');
                std::istringstream sticks(std::string(buff, block.gcount() - 1));
                while(true) {
                    sticks.ignore(std::numeric_limits<std::streamsize>::max(), '\"');
                    if(sticks.eof()) break;
                    sticks.getline(buff, buffSize, '\"');
                    const std::string axisname(buff, sticks.gcount() - 1);
                    if(!strToAxis.contains(axisname)) {
                        return fatal<InputMapping>(std::format("Invalid axis name \"{}\"", axisname));
                    }
                    SDL_GamepadAxis axis = strToAxis.at(axisname);
                    inputMapping.mapping.axes[inputMapping.mapping.numAxes] = axis;
                    sticks.ignore(3);
                    float value;
                    sticks >> value;
                    inputMapping.mapping.axisValues[inputMapping.mapping.numAxes++] = value;
                    if(inputMapping.mapping.numAxes == maxAxes) break;
                }
            }
        }
        return inputMapping;
    }

    Error<noreturn> Input::loadMappings(const std::string& path) noexcept {
        std::ifstream input(path);
        if(!input.is_open()) {
            return fatal(std::format("Couldn't find an input mappings file at \"{}\"", path));
        }
        const u16 buffSize = 1024;
        char buff[buffSize];
        char c;
        input >> c;
        if(c != '[') {
            return fatal(std::format("Expected \'[\' at start of input mappings file, got \'{}\'", c));
        }
        bool end = false;
        while(!end) {
            input >> c;
            if(input.eof()) break;
            switch(c) {
            case ']':
                end = true;
                break;
            case '{':
                //Read in whole block
                input.getline(buff, buffSize, '}');
                std::istringstream block(std::string(buff, input.gcount() - 1));
                TRY_INIT(InputMapping, inputMapping, readInputMapping(block, buff, buffSize), noreturn); 
                mMappings[inputMapping.tag] = inputMapping.mapping;
                break;
            }
        }
        input.close();
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
            static constexpr i16 angleToAxisValue = SDL_JOYSTICK_AXIS_MAX / 90.0f;
            const i16 val = mapping.axisValues[i] * angleToAxisValue;
            if(val <= 0) {
                if(SDL_GetGamepadAxis(mGamepad, mapping.axes[i]) <= val) return true;
            }
            else {
                if(SDL_GetGamepadAxis(mGamepad, mapping.axes[i]) >= val) return true;
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
        return false;
    }
}
