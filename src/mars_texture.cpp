#include <format>
#include <mars_texture.h>

namespace mars {
    Texture::Texture(GPUImage&& image, u64 width, u64 height) noexcept : mImage(std::move(image)), mWidth(width), mHeight(height) {

    }
    Texture::~Texture() noexcept {
        //mImage.destroy(Renderer::get().device);
    }

    Error<Slice<const Sprite>> Texture::slice(u64 spriteWidth, u64 spriteHeight) noexcept {
        //Assume zero spacing
        if (mWidth % spriteWidth != 0 or mHeight % spriteHeight != 0) {
            FATAL(std::format("Provided sprite dimensions ({},{}) cannot divide texture with dimensions ({},{})", spriteWidth, spriteHeight, mWidth, mHeight));
        }
        const u64 columns = mWidth / spriteWidth;
        const u64 rows = mHeight / spriteHeight;
        mSprites.resize(rows * columns);
        for (u64 i = 0; i < rows; i++) {
            for (u64 j = 0; j < columns; j++) {
                Sprite& sprite = mSprites[i * rows + j];
                sprite.texture = this;
                sprite.height = spriteHeight;
                sprite.width = spriteWidth;
                // Get normalized UV coordinates
                sprite.uMin = j * spriteWidth / static_cast<float>(mWidth);
                sprite.uMax = (j + 1) * spriteWidth / static_cast<float>(mWidth);
                sprite.vMin = i * spriteHeight / static_cast<float>(mHeight);
                sprite.vMax = (i + 1) * spriteHeight / static_cast<float>(mHeight);
            }
        }
        return Slice<const Sprite>(mSprites);
    }
    const GPUImage& Texture::image() const noexcept {
        return mImage;
    }

    u64 Texture::width() const noexcept {
        return mWidth;
    }

    u64 Texture::height() const noexcept {
        return mHeight;
    }

    Slice<const Sprite> Texture::sprites() const noexcept {
        return mSprites;
    }
}
