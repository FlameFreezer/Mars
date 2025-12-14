module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

export module mars;
export import :renderer;
export import error;
export import array;

namespace mars {
    export Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::INIT_SDL_FAIL, SDL_GetError()};
        }
        return success();
    }
    export void quit() noexcept {
        SDL_Quit();
    }

    export class Game {
        public:
        Game() noexcept : windowName("My Mars Game"), appName("My Mars Game") {
            this->init(appName);
        }
        Game(const std::string& name) noexcept : windowName(name), appName(name) {
            this->init(appName);
        }
        virtual ~Game() noexcept {

        }
        Error<noreturn> const& getProcResult() const noexcept {
            return procResult;
        }
        void init(const std::string& appName){
            procResult = renderer.init(appName);
            if(!procResult.okay()) return;
        }
        Error<noreturn> draw() noexcept {
            return success();
        }
        private:
        std::string windowName;
        std::string appName;
        Renderer renderer;
        Error<noreturn> procResult;
    };
};
