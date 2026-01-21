module;

#include <cstdint>
#include <concepts>

#define RENDERER_NEXT_BIT(name) export constexpr RendererFlags name{1U << (__LINE__ - rendererStart)}
#define GAME_NEXT_BIT(name) export constexpr GameFlags name{1U << (__LINE__ - gameStart)}

export module flag_bits;

namespace mars {
    template<class T> requires std::integral<T>
    class MarsFlags {
        T bits;
        public:
        using Width = T;
        constexpr MarsFlags() noexcept : bits(0) {}
        constexpr MarsFlags(Width inBits) noexcept : bits(inBits) {}
        constexpr MarsFlags(MarsFlags const& other) noexcept : bits(other.bits) {}
        MarsFlags operator|(MarsFlags rhs) const noexcept {
            return bits | rhs.bits;
        }
        MarsFlags operator&(MarsFlags rhs) const noexcept {
            return bits & rhs.bits;
        }
        MarsFlags operator^(MarsFlags rhs) const noexcept {
            return bits ^rhs.bits;
        }
        MarsFlags& operator&=(MarsFlags rhs) noexcept {
            bits &= rhs.bits;
            return *this;
        }
        MarsFlags& operator|=(MarsFlags rhs) noexcept {
            bits |= rhs.bits;
            return *this;
        }
        MarsFlags& operator^=(MarsFlags rhs) noexcept {
            bits ^= rhs.bits;
            return *this;
        }
        MarsFlags operator~() const noexcept {
            return ~bits;
        }
        operator bool() const noexcept {
            return bits != 0;
        }
    };
    export class RendererFlags : public MarsFlags<std::uint8_t> {};
    export class GameFlags : public MarsFlags<std::uint8_t> {};
    namespace flagBits {
        constexpr std::uint32_t rendererStart = __LINE__ + 1U;
        RENDERER_NEXT_BIT(recreateSwapchain);
        RENDERER_NEXT_BIT(deviceInvalid);
        RENDERER_NEXT_BIT(instanceInvalid);
        RENDERER_NEXT_BIT(beganTransferOps);

        constexpr std::uint32_t gameStart = __LINE__ + 1U;
        GAME_NEXT_BIT(stopExecution);
    }
}
