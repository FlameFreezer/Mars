module;

#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

export module mars;
export import :renderer;
export namespace mars {
    class Game {
    public:
	Game();
	Game(const std::string& inWindowName);
	~Game();
	void Run();
    private:
	Renderer mRenderer;	
    };
}
