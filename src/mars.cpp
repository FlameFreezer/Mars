module;

#include <string>

#include <SDL3/SDL.h>

module mars;
namespace mars {
    Game::Game() : mRenderer("My Mars Game") {
	mRenderer.init();
    }
    Game::Game(const std::string& inWindowName) : mRenderer(inWindowName) {
	mRenderer.init();
    }
    Game::~Game() {
	mRenderer.destroy();
	SDL_Quit();
    }
    void Game::Run() {
    }
}
