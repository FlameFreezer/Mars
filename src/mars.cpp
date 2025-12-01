module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <string>

module mars;

#define MARS_TRY(proc) procResult = proc;\
if(!procResult.okay()) return procResult

namespace mars {
    Error<noreturn> success()  {
        return Error<noreturn>();
    }

    Error<noreturn> init() noexcept {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::INIT_SDL_FAIL, SDL_GetError()};
        }
        return success();
    }
    int quit() noexcept {
        SDL_Quit();
        return 0;
    }
    Error<noreturn> run() noexcept {
        Error<noreturn> procResult;
        Game g(procResult);
        if(!procResult.okay()) return procResult;
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

    Error<noreturn> Game::init(const std::string& appName) {
        Error<noreturn> procResult;
        MARS_TRY(renderer.init(appName));
        return success();
    }
    Game::Game(Error<noreturn>& result) noexcept : windowName("My Mars Game"), appName("My Mars Game") {
        result = this->init(appName);
    }
    Game::Game(Error<noreturn>& result, const std::string& name) noexcept : windowName(name), appName(name) {
        result = this->init(appName);
    }
    Game::~Game() noexcept {}
}
