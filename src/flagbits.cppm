module;

#include <cstdint>

#define RENDERER_NEXT_BIT(name) export constexpr RendererFlags name = 1U << (__LINE__ - rendererStart)

export module flag_bits;

namespace mars {
    export using RendererFlags = std::uint8_t;
    namespace flagBits {
        constexpr RendererFlags rendererStart = __LINE__ + 1U;
        RENDERER_NEXT_BIT(failedInitialization);
        RENDERER_NEXT_BIT(recreateSwapchain);
    }
}
