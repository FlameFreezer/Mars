module;

#include <string>

#include <SDL3/SDL.h>

export module mars;
export namespace mars {
	void runApp(const std::string& windowName) {
		SDL_Init(SDL_INIT_VIDEO);
		SDL_Window* window = SDL_CreateWindow(windowName.c_str(), 800, 600, SDL_WINDOW_VULKAN);
		SDL_DestroyWindow(window);	
		SDL_Quit();
	}
}
