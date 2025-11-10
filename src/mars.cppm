module;

#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

export module mars;
export import :renderer;
export namespace mars {
    void init();
    void quit();
    class Game {
    public:
	Game() = delete;
	Game(const std::string& inWindowName);
	~Game();
	void Run();
    private:
	Renderer mRenderer;	
    };
}
