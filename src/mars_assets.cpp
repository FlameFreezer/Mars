#include <mars_assets.h>

#include <mutex>

#include <mars_renderer.h>

std::mutex mars::Assets::mutex{};

namespace mars {
    Assets& Assets::get() noexcept {
        std::unique_lock<std::mutex> lock{Assets::mutex};
        static Assets instance;
        return instance;
    }

    Error<std::shared_ptr<Texture>> Assets::getTexture(std::string_view path) noexcept {
        Assets& instance = Assets::get();
        if (!instance.mTextures.contains(std::string{path}) or instance.mTextures.at(std::string{path}).expired()) {
            return instance.loadTexture(std::move(path));
        }
        return instance.mTextures.at(std::string{path}).lock();
    }

    Error<std::shared_ptr<Texture>> Assets::loadTexture(std::string_view path) noexcept {
        TRY_INIT(GPUImage, image, Renderer::get().loadTexture(path));
        std::shared_ptr<Texture> texture = std::make_shared<Texture>(std::move(image));
        mTextures[std::string{path}] = std::weak_ptr{texture};
        return texture;
    }
}