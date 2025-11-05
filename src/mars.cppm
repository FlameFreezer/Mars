module;

#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

export module mars;
import vulkan_hpp;
export namespace mars {
    class Game {
    public:
	Game() = delete;
	Game(const std::string& inWindowName);
	~Game();
	void Run();
    private:
	std::string mWindowName;
	SDL_Window* mWindow;
    };
}
