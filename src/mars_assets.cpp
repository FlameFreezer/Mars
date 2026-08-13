#include <mars_assets.h>
#include <mutex>

std::mutex mars::Assets::mutex{};

namespace mars {
    Assets& Assets::get() noexcept {
        std::unique_lock<std::mutex> lock{Assets::mutex};
        static Assets instance;
        return instance;
    }

    Error<std::shared_ptr<Texture>> Assets::loadTexture(std::string_view path, std::string &&keyName) noexcept {
        FATAL("Unimplemented!");
        //TRY_INIT(GPUImage, image, Renderer::loadTexture(path)));
        //Texture texture{std::move(image)};
        //std::shared_ptr<Texture> texture = std::make_shared(std::move(image));
        //mAssets.emplace(std::move(keyName), std::make_weak(texture));
        //return texture;
    }
}