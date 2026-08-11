#pragma once

#include "mars_types.h"
#include "mars_camera.h"
#include "mars_camera2D.h"
#include "mars_renderer.h"
#include "mars_heaparray.h"
#include "mars_ecs.h"
#include "mars_room.h"
#include "mars_time.h"
#include "mars_timer.h"
#include "mars_input.h"
#include "mars_global.h"
#include "jsonparser.h"
#include "mars_event.h"
#include "error.h"

namespace mars {
    Error<noreturn> init() noexcept;
    void quit() noexcept;
};