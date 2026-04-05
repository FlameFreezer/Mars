module;

#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

export module mars;
export import types;
export import camera;
export import renderer;
export import flag_bits;
export import error;
export import heap_array;
export import ecs;
export import physics_manager;
export import room;
export import time;
export import timer;
export import input;

namespace mars {
    export Error<noreturn> init() noexcept;
    export void quit() noexcept;

};
