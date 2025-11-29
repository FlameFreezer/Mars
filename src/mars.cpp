#ifndef MARS_H
#include <mars.hpp>
#endif

#include "renderer/renderer.cpp"

namespace mars {
    Error<noreturn> success()  {
        return Error<noreturn>();
    }

    Error<noreturn> init() {
        if(!SDL_Init(SDL_INIT_VIDEO)) {
            return {ErrorTag::INIT_SDL_FAIL, SDL_GetError()};
        }
        return success();
    }
    int quit() noexcept {
        SDL_Quit();
        return 0;
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