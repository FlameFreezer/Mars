#pragma once

#include "mars_types.h"
#include "camera/mars_camera.h"
#include "camera/mars_camera2D.h"
#include "renderer/mars_renderer.h"
#include "mars_heaparray.h"
#include "ecs/mars_ecs.h"
#include "mars_room.h"
#include "time/mars_time.h"
#include "timer/mars_timer.h"
#include "input/mars_input.h"
#include "mars_global.h"
#include "jsonparser.h"
#include "mars_event.h"
#include "assetmanager/mars_assets.h"
#include "assetmanager/mars_texture.h"
#include "animation/mars_animation_player.h"
#include "error.h"

namespace mars {
    Error<noreturn> init() noexcept;
    void quit() noexcept;
};