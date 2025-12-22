module;

#include <cstdint>

#define RENDERER_NEXT_BIT(name) export constexpr RendererFlags name = 1U << (__LINE__ - rendererStart)
#define GAME_NEXT_BIT(name) export constexpr GameFlags name = 1U << (__LINE__ - gameStart)

export module flag_bits;

namespace mars {
    export using RendererFlags = std::uint8_t;
    export using GameFlags = std::uint8_t;
    namespace flagBits {
        constexpr RendererFlags rendererStart = __LINE__ + 1U;
        RENDERER_NEXT_BIT(failedInitialization);
        RENDERER_NEXT_BIT(recreateSwapchain);

        constexpr GameFlags gameStart = __LINE__ + 1U;
        GAME_NEXT_BIT(stopExecution);
    }
}
