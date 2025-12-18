module;

#include <cstdint>

export module flag_bits;

namespace mars {
    export using RendererFlags = std::uint8_t;
    export namespace flagBits {
        constexpr RendererFlags failedInitialization = 1U;
        constexpr RendererFlags recreateSwapchain = 1U << 1;
    }
}
