#pragma once

#include <cstdint>

using u64 = uint64_t;

namespace mars {
    #ifdef MARS_ECS_MAX_ENTITIES
    inline constexpr u64 maxEntities = MARS_ECS_MAX_ENTITIES;
    #else
    inline constexpr u64 maxEntities = 512;
    #endif

    #ifdef MARS_RENDERER_MAX_MESHES
    inline constexpr u64 maxMeshes = MARS_RENDERER_MAX_MESHES;
    #else
    inline constexpr u64 maxMeshes = 128;
    #endif

    #ifdef MARS_RENDERER_MAX_TEXTURES
    inline constexpr u64 maxTextures = MARS_RENDERER_MAX_TEXTURES;
    #else
    inline constexpr u64 maxTextures = 256;
    #endif
}
