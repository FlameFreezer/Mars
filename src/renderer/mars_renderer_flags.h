#pragma once

#include "mars_types.h"

namespace mars {
    namespace rendererFlags {
        using FlagT = u16;
        constexpr FlagT recreateSwapchain = 1;
        constexpr FlagT instanceInvalid = 1 << 1;
        constexpr FlagT deviceInvalid = 1 << 2;
        constexpr FlagT beganTransferOps = 1 << 3;
    }
}
