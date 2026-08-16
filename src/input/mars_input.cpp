#include "input/mars_input.h"

namespace mars {
    std::mutex Input::mutex{};

    Input& Input::get() noexcept {
        std::unique_lock<std::mutex> l(mutex);
        static Input instance;
        return instance;
    }

    Error<noreturn> Input::loadMappings(const std::string& path, const std::unordered_map<std::string, u16>& strToIndex) noexcept {
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
                    resultMapping.axisValues[resultMapping.numAxes] = JSON::valueTo<float>(stickValue).value();
                    resultMapping.numAxes++;
                }
            }
            if (mapping.contains("buffered")) {
                if (mapping.at("buffered").getType() != JSON::Type::jtrue and mapping.at("buffered").getType() != JSON::Type::jfalse) {
                    FATAL("Mapping field \"buffered\" should be true or false");
                }
                resultMapping.isBuffered = mapping.at("buffered").getBool().value();
                resultMapping.bufferIndex = mActionBuffers.size();
                //Get the wait time for the buffer
                float waitTimeS = 0.0f;
                if (mapping.contains("bufferTime")) {
                    if (mapping.at("bufferTime").getType() != JSON::Type::jnumber) {
                        FATAL("Mapping field \"bufferTime\" should be a number");
                    }
                    waitTimeS = JSON::valueTo<float>(mapping.at("bufferTime")).value();
                }
                //Construct the action buffer struct at the end of this array
                mActionBuffers.push_back(ActionBuffer{
                    .isActive = false,
                    .timer = mars::Timer{ waitTimeS },
                    .action = strToIndex.at(mapping.at("tag").getString().value())
                });
            }

            //Get the index of the current mapping from its name
            if(mapping.at("tag").getType() != JSON::Type::jstring) {
                FATAL("Mapping field \"tag\" should hold a JSON string");
            }
            const auto mappingIndex = strToIndex.at(mapping.at("tag").getString().value());
            //Resize the mappings vector if we need space to fit
            if(mMappings.size() <= mappingIndex) {
                mMappings.resize(mappingIndex + 1);
            }

            //Write the result mapping
            mMappings[mappingIndex] = resultMapping;
        }
        return SUCCESS;
    }
    Input::Input() noexcept {
        int numGamepads;
        SDL_JoystickID* gamepads = SDL_GetGamepads(&numGamepads);
        if(numGamepads != 0) {
            mGamepad = SDL_OpenGamepad(gamepads[0]);
        }
        mKeyState = SDL_GetKeyboardState(&mNumKeys);
        mCurrentKeyState = new bool[mNumKeys];
        mPrevKeyState = new bool[mNumKeys];
        for(int i = 0; i < mNumKeys; i++) mPrevKeyState[i] = false;
        std::memcpy(mCurrentKeyState, mKeyState, mNumKeys);
    }

    Input::~Input() noexcept {
        if(mGamepad != nullptr and SDL_GamepadConnected(mGamepad)) {
            SDL_CloseGamepad(mGamepad);
        }
        if (mPrevKeyState) delete[] mPrevKeyState;
        if (mCurrentKeyState) delete[] mCurrentKeyState;
    }

}