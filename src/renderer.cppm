module;
#include <string>
#include <stdexcept>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

export module mars:renderer;
import vulkan_hpp;

namespace mars {
    void createVkInstance(VkInstance&);

    int const WIDTH = 800;
    int const HEIGHT = 600;

    export class Renderer {
    public:
	Renderer() : mWindowName("My Mars Game"), mWindow(nullptr), mInstance(0) {
	    initRenderer();
	}
	Renderer(const std::string& inWindowName) : mWindowName(inWindowName), mWindow(nullptr), mInstance(0) {
	    initRenderer();
	}
	void initRenderer() {
	    mWindow = SDL_CreateWindow(mWindowName.c_str(), WIDTH, HEIGHT, SDL_WINDOW_VULKAN);	
	    if(mWindow == nullptr) {
		throw std::runtime_error(SDL_GetError());
	    }
	    createVkInstance(mInstance); 
	}
	~Renderer() {
	    SDL_DestroyWindow(mWindow);
	}
    private:
	std::string mWindowName;
	SDL_Window* mWindow;
	VkInstance mInstance;
    };

    void createVkInstance(VkInstance& inst) {
	uint32_t extensionCount = 0;
	char const * const * extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
	if(extensionNames == nullptr) {
	    throw std::runtime_error("Failed to Find Required SDL Instance Extensions");
	}
	std::vector<char const *> extensions(extensionCount);
	for(int i = 0; i < extensionCount; i++) {
	    extensions.push_back(extensionNames[i]);
	}
	extensions.push_back(vk::EXTDebugUtilsExtensionName);

	vk::ApplicationInfo const appInfo{
	    .applicationVersion = 1,
	    .apiVersion = vk::makeApiVersion(1, 4, 313, 0),
	};

	vk::InstanceCreateInfo instanceInfo{
	    .pNext = nullptr,
	    .flags = {},
	    .pApplicationInfo = &appInfo,
	    .enabledLayerCount = 0,
	    .ppEnabledLayerNames = nullptr,
	    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
	    .ppEnabledExtensionNames = extensions.data(),
	};

	inst = vk::createInstance(instanceInfo, nullptr);
    }
}
