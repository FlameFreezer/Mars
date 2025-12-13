module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <print>
#include <array>
#include <vector>
#include <string>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cstdint>

#include "mars_macros.h"

export module mars:renderer;
import error;

namespace mars {
    constexpr int width = 800;
    constexpr int height = 600;
    constexpr std::uint32_t maxConcurrentFrames = 2;
    constexpr std::array<char const*, 2> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME
    };
    constexpr std::array<char const*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    #ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
    #else
    constexpr bool enableValidationLayers = true;
    #endif

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;

    bool initFail = false;

    VkBool32 debugCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	    VkDebugUtilsMessengerCallbackDataEXT const*	pCallbackData,
	    void* pUserData
    ) {
        std::println("Validation Layer: {}", pCallbackData->pMessage);
        return VK_FALSE;
    }

    struct SurfaceInfo {
        VkSurfaceCapabilitiesKHR capabilities;
        VkPresentModeKHR presentMode;
    	VkSurfaceFormatKHR format;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static constexpr VkVertexInputBindingDescription getInputBindingDescription() noexcept {
            return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
        }

        static constexpr std::array<VkVertexInputAttributeDescription, 3> getInputAttributeDescriptions() noexcept {
            std::array<VkVertexInputAttributeDescription, 3> descs;
            // POS
            descs[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            };
            // COLOR
            descs[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, color)
            };
            // TEXCOORD
            descs[2] = {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texCoord)
            };
            return descs;
        }
    };	

    constexpr std::array<Vertex, 3> vertices = {
        Vertex{glm::vec3(-0.5f, -0.5f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(0.0f, 0.5f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(0.5f, -0.5f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)}
    };

    struct GPUBuffer {
        VkBuffer handle;
        VkDeviceMemory memory;

        GPUBuffer() noexcept : handle(nullptr), memory(nullptr) {}
        GPUBuffer(Error<noreturn>& procResult, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, std::uint32_t memoryType) noexcept {
            VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .size = size,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr
            };
            if(vkCreateBuffer(device, &bufferInfo, nullptr, &handle) != VK_SUCCESS) {
                procResult = {ErrorTag::BUFFER_CREATION_FAIL, "Failed to create VkBuffer while initializing GPUBuffer"};
                return;
            }
            VkMemoryAllocateInfo allocInfo = {
        		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        		.pNext = nullptr,
        		.allocationSize = size,
        		.memoryTypeIndex = memoryType
            };
            if(vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
                procResult = {ErrorTag::MEMORY_ALLOC_FAIL, "Failed to allocate device memory while initializing GPUBuffer"};
                return;
            }
            if(vkBindBufferMemory(device, handle, memory, 0) != VK_SUCCESS) {
                procResult = {ErrorTag::BUFFER_CREATION_FAIL, "Failed to bind buffer memory while initializing GPUBuffer!"};
                return;
            }
            procResult = success();
        }

        GPUBuffer& operator=(GPUBuffer&& other) {
            if(this != &other) {
                handle = other.handle;
                memory = other.memory;
                other.handle = nullptr;
                other.memory = nullptr;
            }
            return *this;
        }

        void destroy(VkDevice device) noexcept {
            vkDestroyBuffer(device, handle, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
    };

    export class Renderer {
    private:
    	std::vector<VkImage> swapchainImages;
    	std::vector<VkImageView> swapchainImageViews;	
        std::vector<VkQueue> queues;
    	std::array<VkCommandBuffer, maxConcurrentFrames> commandBuffers;
    	GPUBuffer vertexBuffer;
        Error<noreturn> procResult;
        VkInstance instance;
        SDL_Window* window;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkSwapchainKHR swapchain;
    	VkCommandPool commandPool;
    	VkPipeline graphicsPipeline;

        Error<noreturn> createVertexBuffer() noexcept {
            vertexBuffer = GPUBuffer(
                procResult, 
                device, 
                vertices.max_size() * sizeof(Vertex), 
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            if(!procResult.okay()) return procResult;
            return success();
        }

        Error<VkShaderModule> createShaderModule() noexcept {
            std::ifstream shaderCode("slang.spv", std::ios::binary | std::ios::ate);
            if(!shaderCode.is_open()) {
                return {ErrorTag::FILE_OPEN_ERROR, "Failed to find shader code!"};
            }
            VkShaderModuleCreateInfo shaderModuleInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,                                                     
                .codeSize = static_cast<std::uint32_t>(shaderCode.tellg()),                                  
                .pCode = nullptr
            };

            std::vector<char> code(shaderModuleInfo.codeSize);
            shaderCode.seekg(0);
            shaderCode.read(code.data(), shaderModuleInfo.codeSize);
            shaderCode.close();
            shaderModuleInfo.pCode = reinterpret_cast<std::uint32_t*>(code.data());

            VkShaderModule result;
            if(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &result) != VK_SUCCESS) {
                return {ErrorTag::SHADER_MODULE_CREATE_FAIL, "Failed to create shader module!"};
            }
            return result;
        }

        Error<noreturn> createGraphicsPipeline() noexcept {
            std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos;
            Error<VkShaderModule> shaderModule = createShaderModule();
            if(!shaderModule.okay()) return {shaderModule.getTag(), shaderModule.getMessage()};

            shaderStageInfos[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr, 
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shaderModule.getData(),
                .pName = "vertMain",
                .pSpecializationInfo = nullptr
            };
            shaderStageInfos[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr, 
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = shaderModule.getData(),
                .pName = "fragMain",
                .pSpecializationInfo = nullptr
            };

            VkVertexInputBindingDescription const desc = Vertex::getInputBindingDescription();
            auto const attributeDescs = Vertex::getInputAttributeDescriptions();
            VkPipelineVertexInputStateCreateInfo const vertexInputStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &desc,
                .vertexAttributeDescriptionCount = attributeDescs.max_size(),
                .pVertexAttributeDescriptions = attributeDescs.data()
            };

            VkPipelineInputAssemblyStateCreateInfo const inputAssemblyInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
            };

            VkPipelineRasterizationStateCreateInfo const rasterizationStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                .depthBiasConstantFactor = 0.0f,
                .depthBiasClamp = 0.0f,
                .depthBiasSlopeFactor = 0.0f,
                .lineWidth = 1.0f
            };

            VkPipelineMultisampleStateCreateInfo const multisampleStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable = VK_FALSE,
                .minSampleShading = 0.0f,
                .pSampleMask = nullptr,
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable = VK_FALSE
            };

            VkPipelineDepthStencilStateCreateInfo const depthStencilStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .depthTestEnable = VK_FALSE,
                .depthWriteEnable = VK_FALSE,
                .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE,
                .front = {},
                .back = {},
                .minDepthBounds = 0.0f,
                .maxDepthBounds = 1.0f
            };

            VkPipelineColorBlendAttachmentState const colorBlendAttachmentState = {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            };

            VkPipelineColorBlendStateCreateInfo const colorBlendStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_NO_OP,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachmentState,
                .blendConstants = {1.0f, 1.0f, 1.0f, 1.0f}
            };

            std::array<VkFormat, 1> const colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB};

            VkPipelineRenderingCreateInfo const pipelineRenderingInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext = nullptr,
                .viewMask = 0,
                .colorAttachmentCount = colorAttachmentFormats.max_size(),
                .pColorAttachmentFormats = colorAttachmentFormats.data(),
                .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
            };

            std::array<VkDynamicState, 2> const dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT, 
                VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT, 
            };
            VkPipelineDynamicStateCreateInfo const dynamicStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .dynamicStateCount = dynamicStates.max_size(),
                .pDynamicStates = dynamicStates.data()
            };
            
            VkPipelineLayoutCreateInfo const pipelineLayoutInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = 0,
                .pSetLayouts = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges = nullptr
            };
            
            VkGraphicsPipelineCreateInfo pipelineInfo = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &pipelineRenderingInfo,
                .flags = 0,
                .stageCount = shaderStageInfos.max_size(),
                .pStages = shaderStageInfos.data(),
                .pVertexInputState = &vertexInputStateInfo,
                .pInputAssemblyState = &inputAssemblyInfo,
                .pTessellationState = nullptr,
                .pViewportState = nullptr, //dynamic
                .pRasterizationState = &rasterizationStateInfo,
                .pMultisampleState = &multisampleStateInfo,
                .pDepthStencilState = &depthStencilStateInfo,
                .pColorBlendState = &colorBlendStateInfo,
                .pDynamicState = &dynamicStateInfo,
                .layout = nullptr,
                .renderPass = nullptr,
                .subpass = 0,
                .basePipelineHandle = nullptr,
                .basePipelineIndex = 0
            };

            if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineInfo.layout) != VK_SUCCESS) {
                return {ErrorTag::GRAPHICS_PIPELINE_CREATION_FAIL, "Failed to create graphics pipeline layout!"};
            }

            if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                return {ErrorTag::GRAPHICS_PIPELINE_CREATION_FAIL, "Failed to create graphics pipeline!"};
            }

            vkDestroyPipelineLayout(device, pipelineInfo.layout, nullptr);
            vkDestroyShaderModule(device, shaderModule.getData(), nullptr);
            return success();
        }
        Error<noreturn> getSwapchainImages(SurfaceInfo const& surfaceInfo) noexcept {
            std::uint32_t imageCount;
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::SWAPCHAIN_IMAGE_ACQUISITION_FAIL, "Failed to get swapchain images!"};
            }
            swapchainImages.resize(imageCount);
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS) {
                return {ErrorTag::SWAPCHAIN_IMAGE_ACQUISITION_FAIL, "Failed to get swapchain images!"};
            }
            swapchainImageViews.resize(imageCount);

            VkImageViewCreateInfo imageViewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = nullptr, //This will be updated for each image in a loop later
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = surfaceInfo.format.format,
                .components = {
                    VK_COMPONENT_SWIZZLE_IDENTITY, 
                    VK_COMPONENT_SWIZZLE_IDENTITY, 
                    VK_COMPONENT_SWIZZLE_IDENTITY, 
                    VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            for(std::uint32_t i = 0; i < imageCount; i++) {
                imageViewInfo.image = swapchainImages[i];
                if(vkCreateImageView(device, &imageViewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                    return {ErrorTag::IMAGE_VIEW_CREATE_FAIL, std::format("Failed to create swapchain image view {}!", i)};
                }
            }
            return success();
        }
        Error<noreturn> createCommandBuffers(std::uint32_t queueFamilyIndex) noexcept {
            VkCommandPoolCreateInfo const poolInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = queueFamilyIndex
            };
            if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                return {ErrorTag::COMMAND_POOL_CREATE_FAIL, "Failed to create VkCommandPool!"};
            }
            VkCommandBufferAllocateInfo const allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = maxConcurrentFrames
            };
            if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                return {ErrorTag::COMMAND_BUFFER_ALLOC_FAIL, "Failed to allocate command buffers!"};
            }
            return success();
        }
        Error<VkExtent2D> chooseImageExtent(VkSurfaceCapabilitiesKHR const& capabilities) noexcept {
            if(capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
                return capabilities.currentExtent;
            }
            int width = 0; 
            int height = 0;
            if(!SDL_GetWindowSize(window, &width, &height)) {
                return {ErrorTag::SDL_QUERY_FAIL, SDL_GetError()};
            }
            return VkExtent2D{
                .width = std::clamp<std::uint32_t>(width, 
                    capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                .height = std::clamp<std::uint32_t>(height, 
                    capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };
        }

        Error<noreturn> createVkSwapchainKHR(SurfaceInfo const& surfaceInfo) noexcept {
            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceInfo.capabilities);
            if(!imageExtent.okay()){
                return imageExtent.move<noreturn>();
            }
            VkSwapchainCreateInfoKHR const swapchainInfo = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .surface = surface,
                .minImageCount = surfaceInfo.capabilities.minImageCount + 1,
                .imageFormat = surfaceInfo.format.format,
                .imageColorSpace = surfaceInfo.format.colorSpace,
                .imageExtent = imageExtent.getData(),
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


        Error<noreturn> pickQueueFamilyIndex(
            std::uint32_t& queueFamilyIndex, 
            std::uint32_t& queueCount, 
            VkPhysicalDevice physicalDevice 
        )  {
            //Get queue family properties for the current physical device
            std::uint32_t queueFamilyPropertyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, nullptr);
            std::vector<VkQueueFamilyProperties2> queueFamilyProperties(
                queueFamilyPropertyCount, 
                VkQueueFamilyProperties2{
                    .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2, 
                    .pNext = nullptr, 
                    .queueFamilyProperties = {}
                }
            );
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

        Error<VkPresentModeKHR> choosePresentMode(VkPhysicalDevice physicalDevice) {
            VkPresentModeKHR presentMode{};
            //Get present modes for the surface supported by the physical device
            std::uint32_t presentModeCount = 0;
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

        Error<VkSurfaceFormatKHR> checkDeviceSurfaceFormats(VkPhysicalDevice physicalDevice) {
            //Get physical device's surface formats
            std::uint32_t surfaceFormatCount = 0;
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
            std::uint32_t deviceExtensionPropertyCount = 0;
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate physical device extension properties!"};
            }
            std::vector<VkExtensionProperties> extensionProperties(deviceExtensionPropertyCount);
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, extensionProperties.data()) != VK_SUCCESS) {
                return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate physical device extension properties!"};
            }
            //Construct set of extension names
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

        Error<noreturn> createVkDevice(SurfaceInfo& surfaceInfo, std::uint32_t& queueFamilyIndex) noexcept {
            std::uint32_t queueCount = 0;

            //Get all the physical devices installed on the system
            std::uint32_t physicalDeviceCount = 0;
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
                if(procResult.getTag() == ErrorTag::SEARCH_FAIL) continue;
                else if(!procResult.okay()) return procResult;
                //Get physical device's capabilities with the surface
                if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                    physicalDevices[i], 
                    surface, 
                    &surfaceInfo.capabilities
                ) != VK_SUCCESS) {
                    return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to get physical device surface capabilities!"};
                }

                //Get a surface format to use
                Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(physicalDevices[i]);
                if(surfaceFormat.okay()) {
                    surfaceInfo.format = surfaceFormat.getData();
                }
                else {
                    return surfaceFormat.move<noreturn>();
                }
                //Choose a present mode to use
                Error<VkPresentModeKHR> presentMode = choosePresentMode(physicalDevices[i]);
                if(presentMode.getTag() == ErrorTag::SEARCH_FAIL) continue;
                else if(presentMode.okay()) {
                    surfaceInfo.presentMode = presentMode.getData();
                }
                else {
                    return presentMode.move<noreturn>();
                }
                //Pick the desired queue family index
                procResult = pickQueueFamilyIndex(queueFamilyIndex, queueCount, physicalDevices[i]);
                if(procResult.getTag() == ErrorTag::SEARCH_FAIL) continue;
                else if(!procResult.okay()) return procResult;
                //At this point if all has succeeded, we are ready to use the current physical device and
                // create the logical device
                physicalDevice = physicalDevices[i];
                goto Device_Creation;
            } 
            //If we got here, none of the physical devices supported the features we needed
            return {ErrorTag::FIND_SUITABLE_GPU_FAIL, "Failed to find a suitable GPU!"};

        Device_Creation:
            std::vector<float> const queuePriorities(queueCount, 0.0f);

            VkDeviceQueueCreateInfo const queueInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = queueCount,
                .pQueuePriorities = queuePriorities.data()
            };

            VkPhysicalDeviceVulkan13Features vulkan13Features{};
            vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            vulkan13Features.synchronization2 = VK_TRUE;
            vulkan13Features.dynamicRendering = VK_TRUE;
            vulkan13Features.maintenance4 = VK_TRUE;

            VkDeviceCreateInfo const deviceInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &vulkan13Features,
                .flags = 0,
                .queueCreateInfoCount = 1,
                .pQueueCreateInfos = &queueInfo,
                .enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size()),
                .ppEnabledExtensionNames = deviceExtensions.data(),
                .pEnabledFeatures = nullptr
            };

            if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
                return {ErrorTag::DEVICE_CREATION_FAIL, "Failed to create VkDevice!"};
            }

            //Acquire handles to all the GPU queues we just created
            queues.resize(queueCount, nullptr);
            for(std::uint32_t i = 0; i < queueCount; i++) {
                vkGetDeviceQueue(device, queueFamilyIndex, i, &queues.at(i));
            }

            return success();
        }

        Error<noreturn> createVkDebugUtilsMessenger() noexcept {
            VkDebugUtilsMessengerCreateInfoEXT const debugMessengerInfo = {
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
            vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if(vkCreateDebugUtilsMessengerEXT == nullptr) {
                return {ErrorTag::DEBUG_MESSENGER_CREATION_FAIL, "Failed to find function vkCreateDebugUtilsMessengerEXT!"};
            }
            if(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                return {ErrorTag::DEBUG_MESSENGER_CREATION_FAIL, "Failed to create vkDebugUtilsMessengerEXT!"};
            }
            vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                instance, "vkDestroyDebugUtilsMessengerEXT"
            );
            if(vkDestroyDebugUtilsMessengerEXT == nullptr) {
                return {ErrorTag::DEBUG_MESSENGER_CREATION_FAIL, "Failed to find function vkDestroyDebugUtilsMessengerEXT!"};
            }
            return success();
        }

        Error<noreturn> createVkInstance(const std::string& appName) noexcept {
            if constexpr(enableValidationLayers) {
                std::uint32_t layerPropertyCount = 0;
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr) != VK_SUCCESS) {
                    return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate instance layer properties!"};
                }
                std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data()) != VK_SUCCESS) {
                    return {ErrorTag::VULKAN_QUERY_ERROR, "Failed to enumerate instance layer properties!"};
                }
                std::println("Available Layers:");
                for(char const* layer : validationLayers) {
                    for(VkLayerProperties const& property : layerProperties) {
                        std::println("\t{}", property.layerName);
                        if(std::strcmp(layer, property.layerName) == 0) {
                            goto Next_Layer;
                        }
                    }
                    return {ErrorTag::INSTANCE_CREATION_FAIL, "Needed instance layers not found!"};
                    Next_Layer:
                }
            }
            std::uint32_t extCount = 0;
            char const*const* sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
            std::vector<char const*> extNames;
            extNames.reserve(extCount);
            extNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            for(int i = 0; i < extCount; i++) {
                extNames.push_back(sdlExtNames[i]);
            }

            if constexpr(enableValidationLayers) {
                std::println("Enabled Instance Extensions:");
                for(char const* extName : extNames) {
                    std::println("\t{}", extName);
                }
            }

            VkApplicationInfo const appInfo = {
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
                .enabledExtensionCount = static_cast<std::uint32_t>(extNames.size()),
                .ppEnabledExtensionNames = extNames.data()
            };
            if constexpr(enableValidationLayers) {
                instanceInfo.enabledLayerCount = validationLayers.size();
                instanceInfo.ppEnabledLayerNames = validationLayers.data();
            }

            if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
                return {ErrorTag::INSTANCE_CREATION_FAIL, "Failed to create VkInstance!"};
            }
            return success();
        }
        public:
        Error<noreturn> init(std::string const& name) noexcept {
            INIT_TRY(createVkInstance(name));
            if constexpr(enableValidationLayers) {
                INIT_TRY(createVkDebugUtilsMessenger());
            }
            window = SDL_CreateWindow(name.c_str(), width, height, SDL_WINDOW_VULKAN);
            if(window == nullptr) {
                initFail = true;
                return {ErrorTag::WINDOW_CREATION_FAIL, SDL_GetError()};
            }
            if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
                initFail = true;
                return {ErrorTag::SURFACE_CREATION_FAIL, SDL_GetError()};
            }
            SurfaceInfo surfaceInfo{};
            std::uint32_t queueFamilyIndex;
            INIT_TRY(createVkDevice(surfaceInfo, queueFamilyIndex));
            INIT_TRY(createVkSwapchainKHR(surfaceInfo));
            INIT_TRY(getSwapchainImages(surfaceInfo));
            INIT_TRY(createCommandBuffers(queueFamilyIndex));
            INIT_TRY(createGraphicsPipeline());
            INIT_TRY(createVertexBuffer());

            return success();
        }
        Renderer() noexcept : 
            instance(nullptr), 
            window(nullptr), 
            debugMessenger(nullptr), 
            surface(nullptr), 
            device(nullptr), 
            physicalDevice(nullptr), 
            swapchain(nullptr),
            commandPool(nullptr),
            graphicsPipeline(nullptr)
        {}
        virtual ~Renderer() noexcept {
            //If something went wrong during initialization, we can't destory vulkan objects, so 
            // we'll quickly end program execution
            if(initFail) return;
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            for(VkImageView view : swapchainImageViews) {
                vkDestroyImageView(device, view, nullptr);
            }
            vertexBuffer.destroy(device);
            vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
            vkDestroyCommandPool(device, commandPool, nullptr);
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyDevice(device, nullptr);
            SDL_Vulkan_DestroySurface(instance, surface, nullptr);
            SDL_DestroyWindow(window);
            if constexpr(enableValidationLayers) {
                vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }
            vkDestroyInstance(instance, nullptr);
        }
        Error<noreturn> const& getProcResult() const noexcept {
            return procResult;
        }
    };
}
