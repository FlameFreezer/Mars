module;

export module mars;
export import types;
export import camera;
export import renderer;
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
