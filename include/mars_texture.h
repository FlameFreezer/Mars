#pragma once

#include <mars_renderer_gpuimage.h>
#include "glm/ext/vector_float2.hpp"
#include "mars_renderer_gpubuffer.h"
#include "mars_types.h"
#include "error.h"
#include "mars_heaparray.h"

namespace mars {
    struct Sprite {
        const class Texture* texture;
        float uMin = 0.0f;
        float uMax = 1.0f;
        float vMin = 0.0f;
        float vMax = 1.0f;
        u32 width;
        u32 height;
        UniformBuffer<glm::vec2> uvBuffer;
        ~Sprite() noexcept;
    };
    class Texture {
    public:
        Texture(GPUImage&& image) noexcept;
        ~Texture() noexcept;
        Error<Slice<const Sprite>> slice(u32 spriteWidth, u32 spriteHeight) noexcept;
        const GPUImage& image() const noexcept;
        u32 width() const noexcept;
        u32 height() const noexcept;
        Slice<const Sprite> sprites() const noexcept;
    private:
        GPUImage mImage;
        HeapArray<Sprite> mSprites;
    };
}
