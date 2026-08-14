#include <assetmanager/mars_texture.h>

#include <format>

#include <renderer/mars_renderer.h>

namespace mars {
    Texture::Texture(GPUImage&& image) noexcept : mImage(std::move(image)) {
        // This will always succeed
        auto r = slice(mImage.width, mImage.height);
    }
    Texture::~Texture() noexcept {
        vkDeviceWaitIdle(Renderer::device());
        mImage.destroy(Renderer::device());
    }
    Sprite::~Sprite() noexcept {
        uvBuffer.destroy(Renderer::device());
    }

    Error<Slice<const Sprite>> Texture::slice(u32 spriteWidth, u32 spriteHeight) noexcept {
        //Assume zero spacing
        if (mImage.width % spriteWidth != 0 or mImage.height % spriteHeight != 0) {
            FATAL(std::format("Provided sprite dimensions ({},{}) cannot divide texture with dimensions ({},{})", spriteWidth, spriteHeight, mImage.width, mImage.height));
        }
        const u64 columns = mImage.width / spriteWidth;
        const u64 rows = mImage.height / spriteHeight;
        mSprites.resize(rows * columns);
        for (u64 i = 0; i < rows; i++) {
            for (u64 j = 0; j < columns; j++) {
                Sprite& sprite = mSprites[i * rows + j];
                sprite.texture = this;
                sprite.height = spriteHeight;
                sprite.width = spriteWidth;
                // Get normalized UV coordinates
                sprite.uMin = (j * spriteWidth + 0.5) / static_cast<float>(mImage.width); 
                sprite.uMax = ((j + 1) * spriteWidth - 0.5) / static_cast<float>(mImage.width); 
                sprite.vMin = (i * spriteHeight + 0.5) / static_cast<float>(mImage.height);
                sprite.vMax = ((i + 1) * spriteHeight - 0.5) / static_cast<float>(mImage.height);
                TRY_ASSIGN(sprite.uvBuffer, UniformBuffer<glm::vec2>::make(
                    Renderer::device(), Renderer::physicalDevice(), 
                    4 * sizeof(glm::vec2), VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT));
                sprite.uvBuffer.mappedMemory[0] = {sprite.uMin, sprite.vMin};
                sprite.uvBuffer.mappedMemory[1] = {sprite.uMax, sprite.vMin};
                sprite.uvBuffer.mappedMemory[2] = {sprite.uMax, sprite.vMax};
                sprite.uvBuffer.mappedMemory[3] = {sprite.uMin, sprite.vMax};
            }
        }
        return Slice<const Sprite>(mSprites);
    }
    const GPUImage& Texture::image() const noexcept {
        return mImage;
    }

    u32 Texture::width() const noexcept {
        return mImage.width;
    }

    u32 Texture::height() const noexcept {
        return mImage.height;
    }

    Slice<const Sprite> Texture::sprites() const noexcept {
        return mSprites;
    }
}
