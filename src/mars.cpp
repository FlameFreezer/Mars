module;

#include <string>

#include <SDL3/SDL.h>

module mars;
namespace mars {
    Game::Game(const std::string& windowName) : mWindowName(windowName), mWindow(nullptr) {
	SDL_Init(SDL_INIT_VIDEO);
	mWindow = SDL_CreateWindow(mWindowName.c_str(), 800, 600, SDL_WINDOW_VULKAN);	
    }
    Game::~Game() {
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
    }
    void Game::Run() {
	
    }
}
