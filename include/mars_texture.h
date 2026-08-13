#pragma once

#include <mars_renderer_gpuimage.h>
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
        u64 width;
        u64 height;
    };
    class Texture {
    public:
        Texture(GPUImage&& image, u64 width, u64 height) noexcept;
        ~Texture() noexcept;
        Error<Slice<const Sprite>> slice(u64 spriteWidth, u64 spriteHeight) noexcept;
        const GPUImage& image() const noexcept;
        u64 width() const noexcept;
        u64 height() const noexcept;
        Slice<const Sprite> sprites() const noexcept;
    private:
        GPUImage mImage;
        u64 mWidth;
        u64 mHeight;
        HeapArray<Sprite> mSprites;
    };
}
