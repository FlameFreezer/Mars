#pragma once

#include <mutex>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

#include "mars_texture.h"

namespace mars {
    class Assets {
    public:
        static Assets& get() noexcept;
        static Error<std::shared_ptr<Texture>> getTexture(std::string_view path) noexcept;
    private:
        Assets() = default;
        static std::mutex mutex;
        Error<std::shared_ptr<Texture>> loadTexture(std::string_view path) noexcept;

        std::unordered_map<std::string, std::weak_ptr<Texture>> mTextures; 
    };
}