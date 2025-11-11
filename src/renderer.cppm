module;
#include <string>
#include <stdexcept>
#include <cstdint>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

export module mars:renderer;
import vulkan_hpp;

namespace mars {
    int const WIDTH = 800;
    int const HEIGHT = 600;

    export class Renderer {
    public:
	Renderer() = delete;
	Renderer(const std::string& inWindowName) : mWindowName(inWindowName), mWindow(nullptr) {
	    if(!SDL_Init(SDL_INIT_VIDEO)) {
	       throw std::runtime_error(SDL_GetError());
	    }
	    init();
	}
	void init() {
	    mWindow = SDL_CreateWindow(mWindowName.c_str(), WIDTH, HEIGHT, SDL_WINDOW_VULKAN);	
	    if(mWindow == nullptr) {
		throw std::runtime_error(SDL_GetError());
	    }
	    createVkInstance(mInstance); 
	}
	void destroy() {
	    mInstance.destroy();
	    SDL_DestroyWindow(mWindow);
	}
    private:
	std::string mWindowName;
	SDL_Window* mWindow;
	vk::Instance mInstance;

	static void createVkInstance(vk::Instance& inst) {
	    uint32_t sdlExtensionCount = 0;
	    char const * const * sdlExtensionNames = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
	    if(sdlExtensionNames == nullptr) {
		throw std::runtime_error("Failed to Find Required SDL Instance Extensions");
	    }

	    uint32_t const extensionCount = sdlExtensionCount + 1;
	    char const** extensionNames = new char const*[extensionCount];
	    extensionNames[0] = vk::EXTDebugUtilsExtensionName;
	    memcpy(&extensionNames[1], sdlExtensionNames, sdlExtensionCount * sizeof(char const*));

	    vk::ApplicationInfo const appInfo{
		.pNext = nullptr,
		.pApplicationName = "My Mars App",
		.applicationVersion = vk::makeApiVersion(1, 0, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = vk::makeApiVersion(1, 0, 0, 0),
		.apiVersion = vk::makeApiVersion(1, 4, 313, 0),
	    };

	    vk::InstanceCreateInfo const instanceInfo{
		.pNext = nullptr,
		.flags = {},
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = extensionNames,
	    };

	    inst = vk::createInstance(instanceInfo);

	    delete[] extensionNames;
	}
    };
}
