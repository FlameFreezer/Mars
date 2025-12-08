module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

module mars;

namespace mars {
    Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::INIT_SDL_FAIL, SDL_GetError()};
        }
        return success();
    }
    void quit() noexcept {
        SDL_Quit();
    }
    Error<noreturn> run() noexcept {
        Game g;
        if(!g.getProcResult().okay()) return g.getProcResult();
        bool shouldKeepRunning = true;
        while(shouldKeepRunning) {
            SDL_Event e;
            while(SDL_PollEvent(&e)) {
                switch(e.type) {
                case SDL_EVENT_QUIT:
                    shouldKeepRunning = false;
                    break;
                default:
                    break;
                }
            }
        }
        return success();
    }

    void Game::init(const std::string& appName) {
        procResult = renderer.init(appName);
	if(!procResult.okay()) return;
    }
    Game::Game() noexcept : windowName("My Mars Game"), appName("My Mars Game") {
        this->init(appName);
    }
    Game::Game(const std::string& name) noexcept : windowName(name), appName(name) {
        this->init(appName);
    }
    Game::~Game() noexcept {}

    Error<noreturn> const& Game::getProcResult() const noexcept {
	return procResult;
    }
}
