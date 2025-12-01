module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <set>

module mars;

#define MARS_TRY(proc) procResult = proc;\
if(!procResult.okay()) return procResult

static int const WIDTH = 800;
static int const HEIGHT = 600;
static std::array<char const*, 1> const deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static std::array<char const*, 1> const validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
static bool const enableValidationLayers = false;
#else
static bool const enableValidationLayers = true;
#endif

static PFN_vkCreateDebugUtilsMessengerEXT createVkDebugUtilsMessengerEXT = nullptr;
static PFN_vkDestroyDebugUtilsMessengerEXT destroyVkDebugUtilsMessengerEXT = nullptr;

static VkBool32 debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT	messageTypes,
	VkDebugUtilsMessengerCallbackDataEXT const*	pCallbackData,
	void* pUserData
) {
    std::cout << "Validation Layer: " << pCallbackData->pMessage << "\n";
    return VK_FALSE;
}

namespace mars {
    struct SurfaceInfo {
        VkSurfaceCapabilitiesKHR capabilities;
        VkPresentModeKHR presentMode;
    	VkSurfaceFormatKHR format;
        VkSurfaceKHR surface;
    };

    Error<noreturn> pickQueueFamilyIndex(
        uint32_t& queueFamilyIndex, 
        uint32_t& queueCount, 
        VkPhysicalDevice physicalDevice, 
        VkSurfaceKHR surface
    )  {
        //Get queue family properties for the current physical device
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> queueFamilyProperties(queueFamilyPropertyCount);
        for(VkQueueFamilyProperties2& prop : queueFamilyProperties) {
            prop.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
        //Check each queue family index for needed support
        for(int i = 0; i < queueFamilyPropertyCount; i++) {
            if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
                VkBool32 surfaceSupport;
                if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport) != VK_SUCCESS) {
                    return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface support!"};
                }
                if(surfaceSupport == VK_TRUE) {
                    //At this point, we have found our desired queue family index
                    queueFamilyIndex = i;
                    queueCount = queueFamilyProperties[i].queueFamilyProperties.queueCount;
                    return success();
                }
            }
        }
        return {ErrorTag::SEARCH_FAIL, "Physical Device did not have queue family with needed properties!"};
    }

    Error<VkPresentModeKHR> choosePresentMode(
        VkPhysicalDevice physicalDevice, 
        VkSurfaceKHR surface
    ) {
        VkPresentModeKHR presentMode{};
        //Get present modes for the surface supported by the physical device
        uint32_t presentModeCount = 0;
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, 
            surface, 
            &presentModeCount, 
            nullptr
        ) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface present modes!"};
        }
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice, 
            surface, 
            &presentModeCount, 
            presentModes.data()
        ) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface present modes!"};
        }
        //Select correct present mode
        for(int i = 0; i < presentModeCount; i++) {
            //This is our preferred presentation mode
            if(presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
                return VK_PRESENT_MODE_MAILBOX_KHR;
            }
            //We are willing to use this present mode if we have to, but we'll continue the search
            else if(presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
                presentMode = VK_PRESENT_MODE_FIFO_KHR;
            }
        }
        //If we got here but didn't write to surfaceInfo.presentMode, then the current device did not
        // support any of the desired present modes
        if(presentMode == 0) {
            return {ErrorTag::SEARCH_FAIL, "Physical device did not support needed present modes!"};
        }
        return presentMode;
    }

    Error<VkSurfaceFormatKHR> checkDeviceSurfaceFormats(
        VkPhysicalDevice physicalDevice, 
        VkSurfaceKHR surface
    ) {
        //Get physical device's surface formats
        uint32_t surfaceFormatCount = 0;
        if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface formats!"};
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface formats!"};
        }
        for(int j = 0; j < surfaceFormatCount; j++) {
            if(surfaceFormats[j].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[j].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return surfaceFormats[j];
            }
        }
        return surfaceFormats[0];
    }

    Error<noreturn> checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)  {
        //Check that the device supports the extensions needed by the application
        uint32_t deviceExtensionPropertyCount = 0;
        if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, nullptr) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate physical device extension properties!"};
        }
        std::vector<VkExtensionProperties> extensionProperties(deviceExtensionPropertyCount);
        if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, extensionProperties.data()) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate physical device extension properties!"};
        }
        //Construct linked list of extension names
        std::set<std::string> physicalDeviceExtensions;
        for(int i = 0; i < deviceExtensions.size(); i++) {
            physicalDeviceExtensions.emplace(deviceExtensions[i]);
        }
        //Check if we have every needed extension
        for(int i = 0; i < deviceExtensionPropertyCount; i++) {
            //Remove the current extension name from the set, if we have it
            physicalDeviceExtensions.erase(extensionProperties[i].extensionName);
            //Once we've "ticked off" each name from the set, we know the device supports all the needed
            //	extensions
            if(physicalDeviceExtensions.empty()) {
                return success();
            }
        }
        return {ErrorTag::SEARCH_FAIL, "Physical Device did not support needed extensions!"};
    }

    Error<noreturn> createVkDeviceAndSwapchain(
        VkDevice& device, 
        VkPhysicalDevice& physicalDevice, 
        VkSwapchainKHR& swapchain, 
        VkInstance instance, 
        VkSurfaceKHR surface
    )  {
        uint32_t queueFamilyIndex = 0;
        uint32_t queueCount = 0;
        SurfaceInfo surfaceInfo = {};
        surfaceInfo.surface = surface;
        Error<noreturn> procResult;

        //Get all the physical devices installed on the system
        uint32_t physicalDeviceCount = 0;
        if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate physical devices!"};
        }
        std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS) {
            return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate physical devices!"};
        }

        //Iterate through each of these devices
        for(int i = 0; i < physicalDeviceCount; i++) {
            //Check device extension support for the current physical device
            procResult = checkDeviceExtensionSupport(physicalDevices[i]);
            if(procResult.getTag() == ErrorTag::SEARCH_FAIL) {
                continue;
            }
            else if(!procResult.okay()) {
                return procResult;
            }
            //Get physical device's capabilities with the surface
            if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevices[i], 
                surface, 
                &surfaceInfo.capabilities
            ) != VK_SUCCESS) {
                return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface capabilities!"};
            }

            //Get a surface format to use
            Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(physicalDevices[i], surface);
            if(surfaceFormat.okay()) {
                surfaceInfo.format = surfaceFormat.getData();
            }
            else {
                return {surfaceFormat.getTag(), surfaceFormat.getMessage()};
            }
            //Choose a present mode to use
            Error<VkPresentModeKHR> presentMode = choosePresentMode(physicalDevices[i], surface);
            if(presentMode.getTag() == ErrorTag::SEARCH_FAIL) {
                continue;
            }
            else if(presentMode.okay()) {
                surfaceInfo.presentMode = presentMode.getData();
            }
            else {
                return {presentMode.getTag(), presentMode.getMessage()};
            }
            //Pick the desired queue family index
            procResult = pickQueueFamilyIndex(queueFamilyIndex, queueCount, physicalDevices[i], surface);
            //At this point if all has succeeded, we are ready to use the current physical device and
            // create the logical device
            if(procResult.okay()) {
                physicalDevice = physicalDevices[i];
                goto Device_Creation;
            }
            //Otherwise, if something went wrong, return the error
            else if(procResult.getTag() != ErrorTag::SEARCH_FAIL) {
                return procResult;
            }
        } 
        //If we got here, none of the physical devices supported the features we needed
        return {ErrorTag::FIND_SUITABLE_GPU_FAIL, "Failed to find a suitable GPU!"};

    Device_Creation:
        std::vector<float> queuePriorities(queueCount, 0.0f);

        VkDeviceQueueCreateInfo queueInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = queueCount,
            .pQueuePriorities = queuePriorities.data()
        };

        VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueInfo,
            .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
            .ppEnabledExtensionNames = deviceExtensions.data(),
            .pEnabledFeatures = nullptr
        };

        if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
            return {ErrorTag::DEVICE_CREATION_FAIL, "Failed to create VkDevice!"};
        }

        VkSwapchainCreateInfoKHR swapchainInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surfaceInfo.surface,
            .minImageCount = surfaceInfo.capabilities.minImageCount + 1,
            .imageFormat = surfaceInfo.format.format,
            .imageColorSpace = surfaceInfo.format.colorSpace,
            .imageExtent = {.width = WIDTH, .height = HEIGHT,},
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = nullptr,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = surfaceInfo.presentMode,
            .clipped = VK_FALSE,
            .oldSwapchain = nullptr
        };

        if(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
            return {ErrorTag::SWAPCHAIN_CREATION_FAIL, "Failed to create VkSwapchainKHR!"};
        }
        return success();
    }

    Error<noreturn> createVkDebugUtilsMessenger(VkDebugUtilsMessengerEXT& debugMessenger, VkInstance instance) {
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |*/
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
            .pUserData = nullptr,
        };
        createVkDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if(createVkDebugUtilsMessengerEXT == nullptr) {
            return {ErrorTag::DEBUG_MESSENGER_CREATION_FAIL, "Failed to find function vkCreateDebugUtilsMessengerEXT!"};
        }
        if(createVkDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            return {ErrorTag::DEBUG_MESSENGER_CREATION_FAIL, "Failed to create vkDebugUtilsMessengerEXT!"};
        }
        destroyVkDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if(destroyVkDebugUtilsMessengerEXT == nullptr) {
            return {ErrorTag::DEBUG_MESSENGER_CREATION_FAIL, "Failed to find function vkDestroyDebugUtilsMessengerEXT!"};
        }
        return success();
    }

    Error<noreturn> createVkInstance(VkInstance& instance, const std::string& appName)  {
        if(enableValidationLayers) {
            uint32_t layerPropertyCount = 0;
            if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate instance layer properties!"};
            }
            std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
            if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data()) != VK_SUCCESS) {
                return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate instance layer properties!"};
            }
            std::cout << "Available Layers:\n";
            for(char const* layer : validationLayers) {
                for(VkLayerProperties const& property : layerProperties) {
                    std::cout << '\t' << property.layerName << '\n';
                    if(strcmp(layer, property.layerName) == 0) {
                        goto Next_Layer;
                    }
                }
                return {ErrorTag::INSTANCE_CREATION_FAIL, "Needed instance layers not found!"};
                Next_Layer:
            }
        }
        uint32_t extCount = 0;
        char const*const* sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
        std::vector<char const*> extNames;
        extNames.reserve(extCount);
        extNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        for(int i = 0; i < extCount; i++) {
            extNames.push_back(sdlExtNames[i]);
        }

        if(enableValidationLayers) {
            std::cout << "Enabled Instance Extensions:\n";
        	for(char const* extName : extNames) {
                std::cout << '\t' << extName << '\n';
        	}
        }

        VkApplicationInfo appInfo = {
        	.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        	.pNext = nullptr,
        	.pApplicationName = appName.c_str(),
        	.applicationVersion = 1,
        	.pEngineName = nullptr,
        	.engineVersion = 0,
        	.apiVersion = VK_MAKE_API_VERSION(1, 4, 313, 0)
        };
        VkInstanceCreateInfo instanceInfo = {
        	.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        	.pNext = nullptr,
        	.flags = 0,
        	.pApplicationInfo = &appInfo,
        	.enabledLayerCount = 0,
        	.ppEnabledLayerNames = nullptr,
        	.enabledExtensionCount = static_cast<uint32_t>(extNames.size()),
        	.ppEnabledExtensionNames = extNames.data()
        };
        if(enableValidationLayers) {
            instanceInfo.enabledLayerCount = validationLayers.size();
            instanceInfo.ppEnabledLayerNames = validationLayers.data();
        }

        if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
            return {ErrorTag::INSTANCE_CREATION_FAIL, "Failed to create VkInstance!"};
    	}
        return success();
    }

    Error<noreturn> Renderer::init(const std::string& name)  {
        Error<noreturn> procResult;
        MARS_TRY(createVkInstance(instance, name));
        if(enableValidationLayers) {
            MARS_TRY(createVkDebugUtilsMessenger(debugMessenger, instance));
        }
        window = SDL_CreateWindow(name.c_str(), WIDTH, HEIGHT, SDL_WINDOW_VULKAN);
        if(window == nullptr) {
            return {ErrorTag::WINDOW_CREATION_FAIL, SDL_GetError()};
        }
        if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
            return {ErrorTag::SURFACE_CREATION_FAIL, SDL_GetError()};
        }
        MARS_TRY(createVkDeviceAndSwapchain(
            device, 
            physicalDevice, 
            swapchain, 
            instance, 
            surface
        ));
        return success();
    }

    Renderer::Renderer() noexcept : 
        instance(nullptr), 
        window(nullptr), 
        debugMessenger(nullptr), 
        surface(nullptr), 
        device(nullptr), 
        physicalDevice(nullptr), 
        swapchain(nullptr) 
    {}

    Renderer::~Renderer() noexcept {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        vkDestroyDevice(device, nullptr);
        SDL_Vulkan_DestroySurface(instance, surface, nullptr);
        SDL_DestroyWindow(window);
        if(enableValidationLayers) {
            destroyVkDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }
        vkDestroyInstance(instance, nullptr);
    }
}
