#pragma once

#include <array>
#include <mutex>
#include <string>
#include <string_view>
#include <queue>
#include <cstddef>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "mars_renderer_flags.h"
#include "mars_gpubuffer.h"
#include "mars_gpuimage.h"
#include "error.h"
#include "mars_heaparray.h"
#include "mars_types.h"
#include <camera/mars_camera.h>
#include <camera/mars_camera2D.h>

inline constexpr u32 maxConcurrentFrames = 2;

namespace mars {
    struct SurfaceInfo {
        VkSurfaceCapabilitiesKHR capabilities;
        VkPresentModeKHR presentMode;
        VkSurfaceFormatKHR format;
    };

    struct Vertex {
        alignas(4) glm::vec3 pos;
        alignas(4) glm::vec2 texCoord;

        constexpr Vertex(glm::vec3 inPos, glm::vec2 inTexCoord) noexcept : pos(inPos), texCoord(inTexCoord) {}

        static constexpr VkVertexInputBindingDescription getInputBindingDescription() noexcept {
            return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
        }

        static constexpr std::array<VkVertexInputBindingDescription, 2> getInputBindingDescriptions() noexcept {
            std::array<VkVertexInputBindingDescription, 2> bindings{};
            bindings[0] = { 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX };
            bindings[1] = { 1, sizeof(glm::vec2), VK_VERTEX_INPUT_RATE_VERTEX };
            return bindings;
        }
        static constexpr std::array<VkVertexInputAttributeDescription, 2> getInputAttributeDescriptions2() noexcept {
            std::array<VkVertexInputAttributeDescription, 2> descs{};
            // pos : vec3
            descs[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            };
            // texCoord : vec2
            descs[1] = {
                .location = 1,
                .binding = 1,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = 0
            };
            return descs;
        }

        static constexpr std::array<VkVertexInputAttributeDescription, 2> getInputAttributeDescriptions() noexcept {
            std::array<VkVertexInputAttributeDescription, 2> descs{};
            // POS
            descs[0] = {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(Vertex, pos)
            };
            // TEXCOORD
            descs[1] = {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(Vertex, texCoord)
            };
            return descs;
        }
    };
    constexpr glm::vec3 topleft{0.0f, 0.0f, 0.0f};
    constexpr glm::vec3 topright{1.0f, 0.0f, 0.0f};
    constexpr glm::vec3 bottomright{1.0f, 1.0f, 0.0f};
    constexpr glm::vec3 bottomleft{0.0f, 1.0f, 0.0f};
    constexpr glm::vec3 toprightback{1.0f, 0.0f, 1.0f};
    constexpr glm::vec3 topleftback{0.0f, 0.0f, 1.0f};
    constexpr glm::vec3 bottomleftback{0.0f, 1.0f, 1.0f};
    constexpr glm::vec3 bottomrightback{1.0f, 1.0f, 1.0f};
    struct Cube {
        GPUBuffer buffer;
        u32 dim;
        glm::mat4 matrix;
        float fov;
        float aspect;
        static constexpr std::array<Vertex, 24> vertices = {
            //FRONT FACE
            Vertex{topleft, glm::vec2(0.0f, 0.0f)},
            Vertex{topright, glm::vec2(1.0f, 0.0f)},
            Vertex{bottomright, glm::vec2(1.0f, 1.0f)},
            Vertex{bottomleft, glm::vec2(0.0f, 1.0f)},
            //RIGHT FACE
            Vertex{topright, glm::vec2(0.0f, 0.0f)},
            Vertex{toprightback, glm::vec2(1.0f, 0.0f)},
            Vertex{bottomrightback, glm::vec2(1.0f, 1.0f)},
            Vertex{bottomright, glm::vec2(0.0f, 1.0f)},
            //BACK FACE
            Vertex{toprightback, glm::vec2(0.0f, 0.0f)},
            Vertex{topleftback, glm::vec2(1.0f, 0.0f)},
            Vertex{bottomleftback, glm::vec2(1.0f, 1.0f)},
            Vertex{bottomrightback, glm::vec2(0.0f, 1.0f)},
            //LEFT FACE
            Vertex{topleftback, glm::vec2(0.0f, 0.0f)},
            Vertex{topleft, glm::vec2(1.0f, 0.0f)},
            Vertex{bottomleft, glm::vec2(1.0f, 1.0f)},
            Vertex{bottomleftback, glm::vec2(0.0f, 1.0f)},
            //TOP FACE
            Vertex{topleftback, glm::vec2(0.0f, 0.0f)},
            Vertex{toprightback, glm::vec2(1.0f, 0.0f)},
            Vertex{topright, glm::vec2(1.0f, 1.0f)},
            Vertex{topleft, glm::vec2(0.0f, 1.0f)},
            //BOTTOM FACE
            Vertex{bottomleft, glm::vec2(0.0f, 0.0f)},
            Vertex{bottomright, glm::vec2(1.0f, 0.0f)},
            Vertex{bottomrightback, glm::vec2(1.0f, 1.0f)},
            Vertex{bottomleftback, glm::vec2(0.0f, 1.0f)}
        };
        static constexpr std::array<u32, 36> indices = {
            0, 1, 2, 0, 2, 3, //FRONT FACE
            4, 5, 6, 4, 6, 7, //RIGHT FACE
            8, 9, 10, 8, 10, 11, //BACK FACE
            12, 13, 14, 12, 14, 15, //LEFT FACE
            16, 17, 18, 16, 18, 19, //TOP FACE
            20, 21, 22, 20, 22, 23 //BOTTOM FACE
        };
    };

    class Renderer {
        public:
        static Renderer& get() noexcept;
        static VkDevice device() noexcept;
        static VkPhysicalDevice physicalDevice() noexcept;

        rendererFlags::FlagT flags = 0;

        static Error<Renderer&> init(const std::string& name) noexcept; 
        //Destructor
        ~Renderer() noexcept; 

        void destroy() noexcept;

        Error<noreturn> draw(const Camera& camera) noexcept; 

        Error<GPUImage> loadTexture(std::string_view texturePath) noexcept;

        Error<noreturn> loadTilemap(std::string_view tilemapPath) noexcept;

        const mars::Camera2D* getMainCamera2D() const noexcept;

        void setMainCamera2D(const mars::Camera2D& camera2D) noexcept;
    private:
        Renderer() = default;
        static std::mutex mutex;

        Cube cube;

        UniformBuffer<glm::mat4> mCamera2D;
        UniformBuffer<glm::mat4> mCamera3D;

    	HeapArray<VkImage> swapchainImages;
    	HeapArray<VkImageView> swapchainImageViews;	
        HeapArray<GPUImage> renderTargets2D;
        HeapArray<GPUImage> textures2DScene;
        HeapArray<GPUImage> renderTargets3D;
        HeapArray<VkQueue> queues;
        std::queue<GPUBuffer> transferBuffers;
        Slice<VkQueue> graphicsQueues;
        Slice<VkQueue> presentQueues;
        HeapArray<VkSemaphore> semaphores;
        GPUImage depthImage2D;
        GPUImage depthImage3D;
        //Each frame needs a command buffer for the 2D and 3D scenes, and one reserved for transfer
    	std::array<VkCommandBuffer, (2 * maxConcurrentFrames) + maxConcurrentFrames> commandBuffers;
        std::array<VkCommandBuffer, maxConcurrentFrames> transferCommandBuffers;
        std::array<VkFence, maxConcurrentFrames> fences;
        std::array<VkPipeline, 2> graphicsPipelines;
        VkInstance instance = nullptr;
        VkDebugUtilsMessengerEXT debugMessenger = nullptr;
        VkSurfaceKHR surface = nullptr;
        VkDevice mDevice = nullptr;
        VkPhysicalDevice mPhysicalDevice = nullptr;
        VkSwapchainKHR swapchain = nullptr;
        VkExtent2D swapchainImageExtent;
    	VkCommandPool commandPool = nullptr;
        VkPipelineLayout pipelineLayout2D = nullptr;
        VkPipelineLayout pipelineLayout3D = nullptr;
        VkSampler sampler = nullptr;
        u32 currentFrame = 0;
        u32 graphicsQueueFamilyIndex = 0;
        u32 presentQueueFamilyIndex = 0;
        VkSampleCountFlagBits msaaSampleCount;

        VkDescriptorSetLayout mGlobalLayout2D;
        VkDescriptorSetLayout mGlobalLayout3D;
        VkDescriptorSetLayout mPushLayout2D;
        VkDescriptorSetLayout mPushLayout3D;
        VkDescriptorPool mDescriptorPool;
        std::array<VkDescriptorSet, 2 * maxConcurrentFrames> mDescriptorSets;

        SDL_Window* window = nullptr;
        const mars::Camera2D* mainCamera2D = nullptr;

        SurfaceInfo mSurfaceInfo{};

        std::array<VkImageMemoryBarrier2, 3> setup3DMemoryBarriers(u32 imageIndex) noexcept;

        std::array<VkImageMemoryBarrier2, 3> setup2DMemoryBarriers() const noexcept;

        void updateCamera2D() noexcept;

        Error<noreturn> createCamera() noexcept;

        Error<noreturn> drawFrame(float fov, float aspect) noexcept;

        Error<noreturn> beginTransferOps() noexcept;

        Error<noreturn> doTransferOps() noexcept;

        Error<noreturn> createCube() noexcept; 

        Error<noreturn> createRenderTargets() noexcept; 

        Error<noreturn> createDepthImages() noexcept; 

        Error<noreturn> createDescriptorSetLayouts() noexcept; 

        Error<noreturn> createDescriptorPool() noexcept;

        Error<noreturn> allocateDescriptorSets() noexcept;

        Error<noreturn> createSampler() noexcept;

        Error<noreturn> recreateSwapchain() noexcept; 

        void renderPass3D(u32 imageIndex, VkCommandBuffer commandBuffer) noexcept; 

        void renderPass2D(u32 d, VkCommandBuffer commandBuffer) noexcept;        

        Error<noreturn> createSyncObjects() noexcept; 

        Error<VkShaderModule> createShaderModule(std::string_view filename) const noexcept; 

        Error<noreturn> createGraphicsPipeline() noexcept; 

        Error<noreturn> getSwapchainImages() noexcept; 

        Error<noreturn> createCommandBuffers() noexcept; 

        Error<VkExtent2D> chooseImageExtent(VkSurfaceCapabilitiesKHR const& capabilities) noexcept; 

        Error<noreturn> createSwapchain() noexcept; 

        Error<noreturn> createDevice() noexcept; 

        Error<noreturn> createDebugUtilsMessenger() noexcept; 

        Error<noreturn> createVkInstance(std::string_view appName) noexcept; 

        Error<noreturn> createSurface(std::string_view name) noexcept; 

    };
}


