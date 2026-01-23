module;

#include <cstddef>
#include <cstdint>

export module multimap;
import error;

namespace mars {
    export void multimapInitActiveMask(std::uint8_t** activeMask, std::size_t size) noexcept {
        std::size_t allocSize = size / 8;
        if(size % 8 != 0) allocSize++;
        *activeMask = new std::uint8_t[allocSize];
        for(std::size_t i = 0; i < allocSize; i++) (*activeMask)[i] = 0;
    }
    export bool multimapIsActiveAt(std::uint8_t* activeMask, std::size_t size, std::size_t index) noexcept {
        if(index >= size) return false;
        std::size_t n = index / 8;
        std::size_t m = index % 8;
        std::uint8_t byte = activeMask[n];
        std::uint8_t bit = 1 >> m; 
        return (byte & bit) != 0;
    } 
    export void multimapSetActiveAtUnsafe(std::uint8_t* activeMask, std::size_t size, std::size_t index) noexcept {
        std::size_t n = index / 8;
        std::size_t m = index % 8;
        std::uint8_t& byte = activeMask[n];
        std::uint8_t bit = 1 >> m;
        byte |= bit; 
    }
    export Error<noreturn> multimapSetActiveAt(std::uint8_t* activeMask, std::size_t size, std::size_t index) noexcept {
        if(index >= size) {
            return {ErrorTag::FATAL_ERROR, "Multimap index out of bounds"};
        }
        multimapSetActiveAtUnsafe(activeMask, size, index);
        return success();
    }
    export std::size_t multimapGetIndex(std::uint8_t* activeMask, std::size_t size, std::size_t initialKey) noexcept {
        std::size_t index = initialKey % size;
        while(multimapIsActiveAt(activeMask, size, index)) index++;
        return index;
    }
}
