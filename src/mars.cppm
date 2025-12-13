module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

export module mars;
export import error;
export import :renderer;

namespace mars {
    export ErrorNoreturn init() noexcept;
    export void quit() noexcept;

    export class Game {
        public:
        Game() noexcept;
        Game(const std::string& name) noexcept;
        ErrorNoreturn const& getProcResult() const noexcept;
        virtual ~Game() noexcept;
        void init(const std::string& appName);
        private:
        std::string windowName;
        std::string appName;
        Renderer renderer;
        ErrorNoreturn procResult;
    };
};
