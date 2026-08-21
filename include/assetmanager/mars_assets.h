#pragma once

#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>

#include "mars_texture.h"
#include "renderer/mars_renderer.h"

namespace mars {
    class Assets {
    public:
        Assets() = default;
        Error<std::shared_ptr<Texture>> getTexture(std::string_view path) noexcept;
        void setRenderer(Renderer* renderer) noexcept;
        Assets& operator=(const Assets&) = delete;
        Assets& operator=(Assets&&) = delete;
        Assets(const Assets&) = delete;
        Assets(Assets&&) = delete;
    private:
        Error<std::shared_ptr<Texture>> loadTexture(std::string_view path) noexcept;

        std::unordered_map<std::string, std::weak_ptr<Texture>> mTextures; 
        Renderer* mRenderer = nullptr;
    };
}