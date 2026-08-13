#pragma once

#include <mutex>
#include <string_view>
#include <unordered_map>
#include <string>
#include <memory>

#include "mars_texture.h"

namespace mars {
    class Assets {
    public:
        static Assets& get() noexcept;
        static Error<std::shared_ptr<Texture>> loadTexture(std::string_view path, std::string&& keyName) noexcept;
    private:
        Assets() = default;
        static std::mutex mutex;

        std::unordered_map<std::string, std::weak_ptr<int>> mAssets; 
    };
}