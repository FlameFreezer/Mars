module;

#include <string>
#include <stdexcept>

#include <SDL3/SDL.h>

module mars;
namespace mars {
    void init() {
	if(!SDL_Init(SDL_INIT_VIDEO)) {
	   throw std::runtime_error(SDL_GetError());
	}
    }
    void quit() {
	SDL_Quit();
    }
    Game::Game(const std::string& inWindowName) : mRenderer(inWindowName) {
    }
    Game::~Game() {
    }
    void Game::Run() {
    }
}
