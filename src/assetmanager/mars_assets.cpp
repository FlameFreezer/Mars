#include <assetmanager/mars_assets.h>

#include <mutex>

std::mutex mars::Assets::mutex{};

namespace mars {
    Assets& Assets::get() noexcept {
        std::unique_lock<std::mutex> lock{Assets::mutex};
        static Assets instance;
        return instance;
    }

    void Assets::setRenderer(Renderer* renderer) noexcept {
        mRenderer = renderer;
    }

    Error<std::shared_ptr<Texture>> Assets::getTexture(std::string_view path) noexcept {
        if (!mTextures.contains(std::string{path}) or mTextures.at(std::string{path}).expired()) {
            return loadTexture(std::move(path));
        }
        return mTextures.at(std::string{path}).lock();
    }

    Error<std::shared_ptr<Texture>> Assets::loadTexture(std::string_view path) noexcept {
        if (mRenderer == nullptr) {
            FATAL("Asset manager not given a reference to a renderer");
        }
        TRY_INIT(GPUImage, image, mRenderer->loadTexture(path));
        std::shared_ptr<Texture> texture = std::make_shared<Texture>(std::move(image));
        mTextures[std::string{path}] = std::weak_ptr{texture};
        return texture;
    }
}