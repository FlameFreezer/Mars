module;

#include <print>
#include <array>
#include <vector>
#include <string>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <limits>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "mars_macros.h"

export module renderer;
import gpubuffer;
import gpuimage;
import error;
import heap_array;
import flag_bits;
import vkhelper;
import types;
import component_system;
import components;
import renderer_ecs;
import camera;

namespace mars {
    constexpr std::array<const char*, 3> neededDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        VK_EXT_SHADER_OBJECT_EXTENSION_NAME
    };
    constexpr std::array<const char*, 1> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    
    constexpr u32 maxConcurrentFrames = 2;

    #ifdef NDEBUG
    constexpr bool enableValidationLayers = false;
    #else
    constexpr bool enableValidationLayers = true;
    #endif

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;

    VkBool32 debugCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	    const VkDebugUtilsMessengerCallbackDataEXT*	pCallbackData,
	    void* pUserData
    ) {
        std::println("Validation Layer:\n\tSeverity: {}", vkhelper::messageSeverityToString(messageSeverity));
        std::println("\tMessage: {}\n", pCallbackData->pMessage);
        return VK_FALSE;
    }

    struct Vertex {
        alignas(4) glm::vec3 pos;
        alignas(4) glm::vec2 texCoord;

        constexpr Vertex(glm::vec3 inPos, glm::vec2 inTexCoord) noexcept : pos(inPos), texCoord(inTexCoord) {}

        static constexpr VkVertexInputBindingDescription getInputBindingDescription() noexcept {
            return { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
        }

        static constexpr std::array<VkVertexInputAttributeDescription, 2> getInputAttributeDescriptions() noexcept {
            std::array<VkVertexInputAttributeDescription, 2> descs;
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

    struct Square {
        static constexpr std::array<Vertex, 4> vertices = {
            Vertex{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
            Vertex{glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)},
            Vertex{glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f)},
            Vertex{glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f)}
        };
        static constexpr std::array<u32, 6> indices = {
            0, 1, 2,
            0, 2, 3
        };
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

    export class Renderer {

        RendererEntityManager entityManager;
        Cube cube;
        UniformBuffer<glm::mat4> cameraMatrices;
    	HeapArray<VkImage> swapchainImages;
    	HeapArray<VkImageView> swapchainImageViews;	
        HeapArray<GPUImage> renderTargets2D;
        HeapArray<GPUImage> textures2DScene;
        HeapArray<GPUImage> renderTargets3D;
        HeapArray<VkQueue> queues;
        Slice<VkQueue> graphicsQueues;
        Slice<VkQueue> presentQueues;
        HeapArray<VkSemaphore> semaphores;
        GPUImage depthImage2D;
        GPUImage depthImage3D;
        //Each frame needs a command buffer for the 2D and 3D scenes, and one reserved for transfer
    	std::array<VkCommandBuffer, (2 * maxConcurrentFrames) + 1> commandBuffers;
        std::array<VkFence, maxConcurrentFrames> fences;
        std::array<VkPipeline, 2> graphicsPipelines;
        VkDescriptorSetLayout pushSetLayout;
        VkInstance instance;
        SDL_Window* window;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkSwapchainKHR swapchain;
        VkExtent2D swapchainImageExtent;
    	VkCommandPool commandPool;
        VkPipelineLayout pipelineLayout2D;
        VkPipelineLayout pipelineLayout3D;
        VkSampler sampler;
        u32 currentFrame;
        u32 graphicsQueueFamilyIndex;
        u32 presentQueueFamilyIndex;
        VkSampleCountFlagBits msaaSampleCount;

        void setup3DMemoryBarriers(u32 imageIndex, std::array<VkImageMemoryBarrier2, 3>& imageMemoryBarriers3D) noexcept {
            //Transition image layout for color writing
            imageMemoryBarriers3D[0] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = renderTargets3D[currentFrame].handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            //Transition image layout for depth buffering
            imageMemoryBarriers3D[1] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = depthImage3D.handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            //Transition image layout for the 2D scene
            imageMemoryBarriers3D[2] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = textures2DScene[currentFrame].handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
        }

        void setup2DMemoryBarriers(std::array<VkImageMemoryBarrier2, 3>& imageMemoryBarriers2D) noexcept {
            //Transition image layout for color writing
            imageMemoryBarriers2D[0] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = renderTargets2D[currentFrame].handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            //Transition image layout for depth buffering
            imageMemoryBarriers2D[1] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = depthImage2D.handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            //Transition image layout for 2D scene texture
            imageMemoryBarriers2D[2] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = textures2DScene[currentFrame].handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
        }
        void updateCamera() noexcept {
            glm::mat4 constexpr identity = glm::mat4(1.0f);
            //Calculate size of overscan (region of the cube not visible)
            float overscanSize = 0.0f;
            const float w = static_cast<float>(swapchainImageExtent.width);
            const float h = static_cast<float>(swapchainImageExtent.height);
            if(w > h) {
                overscanSize = (w - h) / 2.0f;
            }
            else {
                overscanSize = (h - w) / 2.0f;
            }
            //Factor to put coordinates in range [0, 2]
            const float scale = 1.0f / (0.5f * static_cast<float>(cube.dim));
            //Pushes vertices at (0,0) downwards so that they're on screen, allowing user to
            // assume (0,0) is the top left corner
            const glm::mat4 overscan = glm::translate(identity, glm::vec3(0.0f, overscanSize, 0.0f));
            //Does the afformentioned scaling
            const glm::mat4 s = glm::scale(identity, glm::vec3(scale, scale, 1.0f));
            //Moves scene into canonical vulkan viewing volume with range [-1,1]
            const glm::mat4 t = glm::translate(identity, glm::vec3(-1.0f, -1.0f, 0.0f));
            cameraMatrices.mappedMemory[0] = t * s * overscan;
        }

        Error<noreturn> createCamera() noexcept {
            auto res = UniformBuffer<glm::mat4>::make(device, physicalDevice, 
                //Each frame gets its own 3D camera, while one 2D camera exists
                sizeof(glm::mat4) * (maxConcurrentFrames + 1)); 
            if(!res) {
                return res.moveError();
            }
            else cameraMatrices = res.moveData(); 
            updateCamera();

            return success();
        }


        struct SurfaceInfo {
            VkSurfaceCapabilitiesKHR capabilities;
            VkPresentModeKHR presentMode;
        	VkSurfaceFormatKHR format;
        };

        struct PickQueueFamilyIndexResult {
            u32 presentIndex = std::numeric_limits<u32>::max();
            u32 presentCount = 0U;
            u32 graphicsIndex = std::numeric_limits<u32>::max();
            u32 graphicsCount = 0U;
        };

        struct PickPhysicalDeviceResult {
            VkPhysicalDevice physicalDevice = nullptr;
            PickQueueFamilyIndexResult queueFamilyInfo;
            SurfaceInfo surfaceInfo;
        };
        Error<noreturn> drawFrame(float fov, float aspect, float pixelsPerMeter, const ComponentSystem<Transform>& sysTransform, const ComponentSystem<Draw>& sysDraw) noexcept {
            //Fix the cube - only if any of the viewport details changed
            if(fov != cube.fov or aspect != cube.aspect) {
                float halfd = glm::tan(fov / 2.0f);
                if(aspect > 1.0f) halfd *= aspect;
                glm::vec3 const pos(-halfd, -halfd, 0.0f);
                glm::vec3 const scale(2.0f * halfd);
                cube.matrix = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), scale) ;
                cube.fov = fov;
                cube.aspect = aspect;
            };

            if(vkWaitForFences(
                    device, 
                    1, 
                    &fences[currentFrame], 
                    VK_TRUE, 
                    std::numeric_limits<u64>::max()
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, std::format("Something went from while waiting on fence {}", currentFrame)};
            }
            //These semaphores indicate that we have successfully acquired an image on this frame
            Slice<VkSemaphore> imageAcquiredSemaphores = Slice<VkSemaphore>::make(semaphores, 0, maxConcurrentFrames);
            //These semaphores indicate that we have finished rendering a 2D scene on this frame
            Slice<VkSemaphore> scene2DReadySemaphores = Slice<VkSemaphore>::make(semaphores, maxConcurrentFrames, 2);
            //These semaphores indicate that the image acquired is ready to present
            Slice<VkSemaphore> presentReadySemaphores = Slice<VkSemaphore>::make(semaphores, 2 * maxConcurrentFrames);

            u32 imageIndex;
            VkResult res = vkAcquireNextImageKHR(
                device, 
                swapchain, 
                std::numeric_limits<u64>::max(), 
                imageAcquiredSemaphores[currentFrame], 
                nullptr, 
                &imageIndex
            );
            //Recreate the swapchain and try to draw the frame again
            if(res == VK_ERROR_OUT_OF_DATE_KHR or res == VK_SUBOPTIMAL_KHR or (flags & flagBits::recreateSwapchain)) {
                TRY(recreateSwapchain());
                vkDestroySemaphore(device, imageAcquiredSemaphores[currentFrame], nullptr);
                VkSemaphoreCreateInfo const semaphoreInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0
                };
                if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAcquiredSemaphores[currentFrame]) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to recreate semaphore while recreating swapchain during draw"};
                }
                depthImage2D.destroy(device);
                depthImage3D.destroy(device);
                TRY(createDepthImages());
                flags &= ~flagBits::recreateSwapchain;
                return drawFrame(fov, aspect, pixelsPerMeter, sysTransform, sysDraw);
            }
            //Fatal error has occurred
            else if(res != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to acquire next swapchain image index"};
            }

            if(vkResetFences(device, 1, &fences[currentFrame]) != VK_SUCCESS) {
                return {ErrorTag::fatalError, std::format("Failed to reset fence {}", currentFrame)};
            }

            //Acquire the command buffers to use for this frame
            VkCommandBuffer commandBuffer2D = commandBuffers[currentFrame];
            VkCommandBuffer commandBuffer3D = commandBuffers[2 + currentFrame];

            //Reset command buffers for the current frame, both for the 2D and 3D scene
            if(vkResetCommandBuffer(commandBuffer2D, 0) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to reset command buffer"};
            }
            if(vkResetCommandBuffer(commandBuffer3D, 0) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to reset command buffer"};
            }

            //Render 2D scene
            VkCommandBufferBeginInfo const beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffer2D, &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::fatalError, std::format("Failed to begin command buffer {}", currentFrame)};
            }

            std::array<VkImageMemoryBarrier2, 3> imageMemoryBarriers2D;
            setup2DMemoryBarriers(imageMemoryBarriers2D);

            VkDependencyInfo const colorWriteDependency2D = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = 0,
                .imageMemoryBarrierCount = imageMemoryBarriers2D.max_size(),
                .pImageMemoryBarriers = imageMemoryBarriers2D.data()
            };

            vkCmdPipelineBarrier2(commandBuffer2D, &colorWriteDependency2D);

            renderPass2D(cube.dim, pixelsPerMeter, commandBuffer2D, sysTransform, sysDraw);

            //Submit to create 2D scene
            if(vkEndCommandBuffer(commandBuffer2D) != VK_SUCCESS) {
                return {ErrorTag::fatalError, std::format("Failed to end command buffer {}", currentFrame)};
            }

            VkCommandBufferSubmitInfo const commandBufferInfo2D = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = commandBuffer2D,
                .deviceMask = 0
            };
            VkSemaphoreSubmitInfo const scene2DReady = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = scene2DReadySemaphores[currentFrame],
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .deviceIndex = 0
            };
            VkSubmitInfo2 const submitInfo2D = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = 0,
                .pWaitSemaphoreInfos = nullptr,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferInfo2D,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &scene2DReady
            };

            u32 const graphicsQueueIndex = currentFrame % graphicsQueues.size();
            if(vkQueueSubmit2(graphicsQueues[graphicsQueueIndex], 1, &submitInfo2D, nullptr) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to submit draw commands to queue"};
            }

            //Render 3D Scene
            if(vkBeginCommandBuffer(commandBuffer3D, &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::fatalError, std::format("Failed to begin command buffer {}", 2 + currentFrame)};
            }

            std::array<VkImageMemoryBarrier2, 3> imageMemoryBarriers3D;
            setup3DMemoryBarriers(imageIndex, imageMemoryBarriers3D);

            VkDependencyInfo const colorWriteDependency3D = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = 0,
                .imageMemoryBarrierCount = imageMemoryBarriers3D.max_size(),
                .pImageMemoryBarriers = imageMemoryBarriers3D.data()
            };

            vkCmdPipelineBarrier2(commandBuffer3D, &colorWriteDependency3D);

            renderPass3D(imageIndex, commandBuffer3D);

            //Transition image layout for presentation
            VkImageMemoryBarrier2 const presentSrcBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
                .dstAccessMask = VK_ACCESS_2_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = swapchainImages[imageIndex],
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            VkDependencyInfo const presentDependency = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = 0,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &presentSrcBarrier
            };
            vkCmdPipelineBarrier2(commandBuffer3D, &presentDependency);

            if(vkEndCommandBuffer(commandBuffer3D) != VK_SUCCESS) {
                return {ErrorTag::fatalError, std::format("Failed to end command buffer {}", currentFrame)};
            }

            std::array<VkSemaphoreSubmitInfo, 2> waitSemaphores;
            //Make sure we've acquired a swapchain image
            waitSemaphores[0] = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = imageAcquiredSemaphores[currentFrame],
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .deviceIndex = 0
            };
            //Make sure the 2D scene is actually ready
            waitSemaphores[1] = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = scene2DReadySemaphores[currentFrame],
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .deviceIndex = 0
            };
            VkCommandBufferSubmitInfo const commandBufferInfo3D = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = commandBuffer3D,
                .deviceMask = 0
            };
            VkSemaphoreSubmitInfo const presentationReady = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = presentReadySemaphores[imageIndex],
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .deviceIndex = 0
            };
            VkSubmitInfo2 const submitInfo3D = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = waitSemaphores.max_size(),
                .pWaitSemaphoreInfos = waitSemaphores.data(),
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferInfo3D,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &presentationReady
            };

            if(vkQueueSubmit2(graphicsQueues[graphicsQueueIndex], 1, &submitInfo3D, fences[currentFrame]) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to submit draw commands to queue"};
            }

            VkPresentInfoKHR const presentInfo = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &presentReadySemaphores[imageIndex],
                .swapchainCount = 1,
                .pSwapchains = &swapchain,
                .pImageIndices = &imageIndex,
                .pResults = nullptr
            };
            u32 const presentQueueIndex = currentFrame % presentQueues.size();
            if(vkQueuePresentKHR(presentQueues[presentQueueIndex], &presentInfo) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to present graphics queue"};
            }

            currentFrame = (currentFrame + 1) % maxConcurrentFrames;

            return success();
        }
        Error<noreturn> beginTransferOps() noexcept {
            const VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to begin single time command buffer"}; 
            }
            flags |= flagBits::beganTransferOps;
            return success();
        }

        Error<noreturn> createCube() noexcept {
            const VkDeviceSize verticesSize = Cube::vertices.max_size() * sizeof(Vertex);
            const VkDeviceSize indicesSize = Cube::indices.max_size() * sizeof(u32);
            auto buff = GPUBuffer::make(
                device, physicalDevice, 
                verticesSize + indicesSize,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
                    | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            if(!buff) return buff.moveError();
            else cube.buffer = buff.moveData();
            
            auto transferBuffer = GPUBuffer::make(
                device, physicalDevice,
                verticesSize + indicesSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            if(!transferBuffer) {
                return transferBuffer.moveError();
            }
            void* memory;
            if(vkMapMemory(device, transferBuffer.data().memory, 0, verticesSize, 0, &memory) != VK_SUCCESS) {
                transferBuffer.data().destroy(device);
                return {ErrorTag::fatalError, "Failed to map device memory"};
            }
            std::memcpy(memory, reinterpret_cast<void const*>(Cube::vertices.data()), verticesSize);
            vkUnmapMemory(device, transferBuffer.data().memory);

            if(vkMapMemory(device, transferBuffer.data().memory, verticesSize, indicesSize, 0, &memory) != VK_SUCCESS) {
                transferBuffer.data().destroy(device);
                return {ErrorTag::fatalError, "Failed to map device memory"};
            }
            std::memcpy(memory, reinterpret_cast<void const*>(Cube::indices.data()), indicesSize);
            vkUnmapMemory(device, transferBuffer.data().memory);

            const VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                transferBuffer.data().destroy(device);
                return {ErrorTag::fatalError, "Failed to begin single time command buffer"};
            }

            const VkBufferCopy region = {
                .srcOffset = 0, 
                .dstOffset = 0, 
                .size = verticesSize + indicesSize
            };
            vkCmdCopyBuffer(commandBuffers.back(), transferBuffer.data().handle, cube.buffer.handle, 1, &region);
            if(vkEndCommandBuffer(commandBuffers.back()) != VK_SUCCESS) {
                transferBuffer.data().destroy(device);
                return {ErrorTag::fatalError, "Failed to end transfer command buffer"};
            }
            const VkCommandBufferSubmitInfo commandBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, 
                .pNext = nullptr,
                .commandBuffer = commandBuffers.back(),
                .deviceMask = 0
            };
            const VkSubmitInfo2 submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = 0,
                .pWaitSemaphoreInfos = nullptr,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferInfo,
                .signalSemaphoreInfoCount = 0,
                .pSignalSemaphoreInfos = nullptr
            };
            if(vkQueueSubmit2(graphicsQueues[0], 1, &submitInfo, nullptr) != VK_SUCCESS) {
                transferBuffer.data().destroy(device);
                return {ErrorTag::fatalError, "Failed to submit to transfer queue while creating vertex buffer"};
            }
            vkQueueWaitIdle(graphicsQueues[0]);
            vkResetCommandBuffer(commandBuffers.back(), 0);
            transferBuffer.data().destroy(device);

            cube.matrix = glm::mat4(1.0f);
            cube.dim = std::max(swapchainImageExtent.width, swapchainImageExtent.height);
            cube.fov = 0.0f;
            cube.aspect = 0.0f;

            return success();
        }

        Error<noreturn> createRenderTargets(VkFormat format) noexcept {
            //The 2D render area is the face of the cube, so it has to be square
            const u32 d = cube.dim;
            //create 2D render targets
            renderTargets2D.resize(maxConcurrentFrames);
            for(GPUImage& target : renderTargets2D) {
                Error<GPUImage> t = GPUImage::make(
                    device, physicalDevice,
                    {d, d, 1},
                    //this image will be multisampled
                    msaaSampleCount, 
                    VK_IMAGE_TILING_OPTIMAL,
                    //this will be resolved to the textures
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    format, VK_IMAGE_ASPECT_COLOR_BIT
                );
                if(!t) return t.moveError();
                else target = t;
            }
            //create 2D textures - for mapping to the cube
            textures2DScene.resize(maxConcurrentFrames);
            for(GPUImage& tex : textures2DScene) {
                Error<GPUImage> t = GPUImage::make(
                    device, physicalDevice,
                    {d, d, 1},
                    VK_SAMPLE_COUNT_1_BIT, //texture does not store multiple samples
                    VK_IMAGE_TILING_OPTIMAL,
                    //Image will be sampled, but is also the resolve target for the 2D scene
                    // thus, initially a color attachment
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    format, VK_IMAGE_ASPECT_COLOR_BIT
                );
                if(!t) return t.moveError();
                else tex = t;
            }
            //create 3D render targets
            renderTargets3D.resize(maxConcurrentFrames);
            for(GPUImage& target : renderTargets3D) {
                Error<GPUImage> t = GPUImage::make(
                    device, physicalDevice,
                    {swapchainImageExtent.width, swapchainImageExtent.height, 1},
                    //this image will be multisampled
                    msaaSampleCount, 
                    VK_IMAGE_TILING_OPTIMAL,
                    //this will be resolved to the swapchain images
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    format, VK_IMAGE_ASPECT_COLOR_BIT
                );
                if(!t) return t.moveError();
                else target = t;
            }
            return success();
        }

        Error<noreturn> createDepthImages() noexcept {
            const u32 d = cube.dim;
            const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
            //2D depth image has the dimensions of the cube
            Error<GPUImage> dp = GPUImage::make(device, physicalDevice,
                {d, d, 1},
                msaaSampleCount, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
            if(!dp) return dp.moveError();
            depthImage2D = dp.moveData();
            
            //3D depth image has the dimensions of the swapchain images
            dp = GPUImage::make(device, physicalDevice,
                {swapchainImageExtent.width, swapchainImageExtent.height, 1},
                msaaSampleCount, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
            if(!dp) return dp.moveError();
            depthImage3D = dp.moveData();

            return success();
        }

        Error<noreturn> createDescriptorSetLayouts() noexcept {
            std::array<VkDescriptorSetLayoutBinding, 2> pushBindings = {
                //Camera matrix
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = nullptr
                },
                //Texture image sampler
                VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                }
            };
            const VkDescriptorSetLayoutCreateInfo pushLayout = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
                .bindingCount = pushBindings.max_size(),
                .pBindings = pushBindings.data()
            };
            if(vkCreateDescriptorSetLayout(device, &pushLayout, nullptr, &pushSetLayout) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create descriptor set layout"};
            }
            return success();
        }

        Error<noreturn> createSampler() noexcept {
            VkPhysicalDeviceProperties2 props{};
            props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(physicalDevice, &props);
            const VkSamplerCreateInfo samplerInfo = {
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .mipLodBias = 0.0f,
                .anisotropyEnable = VK_TRUE,
                .maxAnisotropy = props.properties.limits.maxSamplerAnisotropy,
                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,
                .minLod = 0.0f,
                .maxLod = 0.0f,
                .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates = VK_FALSE
            };
            if(vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create texture sampler"};
            }
            return success();
        }

        Error<noreturn> recreateSwapchain() noexcept {
            vkDeviceWaitIdle(device);

            Error<VkPresentModeKHR> presentMode = choosePresentMode(physicalDevice, surface); 
            if(!presentMode) return presentMode.moveError();

            VkSurfaceCapabilitiesKHR surfaceCapabilities{};
            if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get physical device surface capabilities while recreating the swapchain"};
            }

            Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(physicalDevice, surface);
            if(!surfaceFormat) return surfaceFormat.moveError();

            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceCapabilities);
            if(!imageExtent) return imageExtent.moveError();
            swapchainImageExtent = imageExtent;
            cube.dim = std::max(swapchainImageExtent.width, swapchainImageExtent.height);

            updateCamera();

            VkSwapchainKHR newSwapchain;
            const VkSwapchainCreateInfoKHR swapchainInfo = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .surface = surface,
                .minImageCount = surfaceCapabilities.minImageCount + 1,
                .imageFormat = surfaceFormat.data().format,
                .imageColorSpace = surfaceFormat.data().colorSpace,
                .imageExtent = swapchainImageExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = nullptr,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = presentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = swapchain
            };
            if(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &newSwapchain) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to recreate swapchain"};
            }
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = newSwapchain;

            for(VkImageView imageView : swapchainImageViews) vkDestroyImageView(device, imageView, nullptr);
            swapchainImageViews.clear();
            swapchainImages.clear();

            TRY(getSwapchainImages(surfaceFormat.data().format));

            return success();
        }

        void renderPass3D(u32 imageIndex, VkCommandBuffer commandBuffer) noexcept {
            //Render to the 3D render target, resolve to the swapchain image
            const VkRenderingAttachmentInfo renderAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = renderTargets3D[currentFrame].view,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
                .resolveImageView = swapchainImageViews[imageIndex],
                .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {.float32 = {1.0f, 1.0f, 1.0f, 1.0f}}}
            };
            //Carry the depth image
            const VkRenderingAttachmentInfo depthAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = depthImage3D.view,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = nullptr,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue = {.depthStencil = {.depth = 1.0f, .stencil = 0}}
            };
            const VkRenderingInfo renderingInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderArea = {
                    .offset = {0, 0},
                    .extent = swapchainImageExtent
                },
                .layerCount = 1,
                .viewMask = 0,
                .colorAttachmentCount = 1,
                .pColorAttachments = &renderAttachment,
                .pDepthAttachment = &depthAttachment,
                .pStencilAttachment = nullptr
            };
            vkCmdBeginRendering(commandBuffer, &renderingInfo);
            //Bind the 3D graphics pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[1]);

            //Viewport and scissor are the size of the swapchain image
            const VkViewport viewport = {
                .x = 0.0f, .y = 0.0f, 
                .width = static_cast<float>(swapchainImageExtent.width), 
                .height = static_cast<float>(swapchainImageExtent.height), 
                .minDepth = 0.0f, .maxDepth = 1.0f
            };
            vkCmdSetViewportWithCount(commandBuffer, 1, &viewport);
            const VkRect2D scissor = {
                .offset = {0, 0},
                .extent = swapchainImageExtent 
            };
            vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);

            //Write the 3D camera to the push descriptor
            const VkDescriptorBufferInfo camera3DBufferInfo = {
                .buffer = cameraMatrices.buffer.handle,
                //Offset correctly for the current frame's camera
                .offset = sizeof(glm::mat4) * (1 + currentFrame),
                .range = sizeof(glm::mat4)
            };
            const VkWriteDescriptorSet writeCamera = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = nullptr,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &camera3DBufferInfo,
                .pTexelBufferView = nullptr
            };
            vkCmdPushDescriptorSet(
                commandBuffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                pipelineLayout3D, 
                0, 
                1, 
                &writeCamera
            );
            //Push the 2D scene texture
            const VkDescriptorImageInfo materialInfo = {
                .sampler = sampler,
                .imageView = textures2DScene[currentFrame].view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };
            const VkWriteDescriptorSet writeMaterial = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = nullptr,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &materialInfo,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr
            };
            vkCmdPushDescriptorSet(
                commandBuffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                pipelineLayout3D, 
                0, 
                1, 
                &writeMaterial
            );
            //Bind the cube's mesh
            const VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cube.buffer.handle, &offset);
            //Push the cube's model matrix
            vkCmdPushConstants(commandBuffer, pipelineLayout3D, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &cube.matrix);
            //Bind the cube's index buffer
            vkCmdBindIndexBuffer(commandBuffer, cube.buffer.handle, Cube::vertices.size() * sizeof(Vertex), VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, Cube::indices.max_size(), 1, 0, 0, 0);
            vkCmdEndRendering(commandBuffer);
        }

        void renderPass2D(u32 d, float pixelsPerMeter, VkCommandBuffer commandBuffer, const ComponentSystem<Transform>& sysTransform, const ComponentSystem<Draw>& sysDraw) noexcept {
            //Render to the 2D render target, resolve to the 2D scene texture image
            //The texture image will be used as the texture for the cube
            const VkRenderingAttachmentInfo renderAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = renderTargets2D[currentFrame].view,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
                .resolveImageView = textures2DScene[currentFrame].view,
                .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}}
            };
            //Carry the depth attachment
            const VkRenderingAttachmentInfo depthAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = depthImage2D.view,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = nullptr,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue = {.depthStencil = {.depth = 1.0f, .stencil = 0}}
            };
            const VkRenderingInfo renderingInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .pNext = nullptr,
                .flags = 0,
                .renderArea = {
                    .offset = {0, 0},
                    .extent = {d, d}
                },
                .layerCount = 1,
                .viewMask = 0,
                .colorAttachmentCount = 1,
                .pColorAttachments = &renderAttachment,
                .pDepthAttachment = &depthAttachment,
                .pStencilAttachment = nullptr
            };
            vkCmdBeginRendering(commandBuffer, &renderingInfo);
            //Bind the 2D graphics pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[0]);

            //Viewport and scissor are the size of the cube
            const VkViewport viewport = {
                .x = 0.0f, .y = 0.0f, 
                .width = static_cast<float>(d), 
                .height = static_cast<float>(d), 
                .minDepth = 0.0f, .maxDepth = 1.0f
            };
            vkCmdSetViewportWithCount(commandBuffer, 1, &viewport);
            const VkRect2D scissor = {
                .offset = {0, 0},
                .extent = {d, d} 
            };
            vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);

            //Write the 2D camera to the push descriptor
            const VkDescriptorBufferInfo camera2DBufferInfo = {
                .buffer = cameraMatrices.buffer.handle,
                .offset = 0,
                .range = sizeof(glm::mat4)
            };
            const VkWriteDescriptorSet writeCamera = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = nullptr,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &camera2DBufferInfo,
                .pTexelBufferView = nullptr
            };
            vkCmdPushDescriptorSet(
                commandBuffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                pipelineLayout2D, 
                0, 
                1, 
                &writeCamera
            );

            //Bind the vertex buffers for the meshes
            HeapArray<VkDeviceSize> offsets;
            if(entityManager.sysMesh->size() != 0) {
                offsets.init(entityManager.sysMesh->size(), 0);
                vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<u32>(entityManager.sysMesh->size()), entityManager.sysMesh->handles(), offsets.data());
            }

            //Iterate through every drawable entity
            for(auto [d, entityID] : sysDraw) {
                //Push the descriptor for the texture
                const VkDescriptorImageInfo materialInfo = {
                    .sampler = sampler,
                    .imageView = (*entityManager.sysTexture)[d.textureID].view,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };
                const VkWriteDescriptorSet writeMaterial = {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext = nullptr,
                    .dstSet = nullptr,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo = &materialInfo,
                    .pBufferInfo = nullptr,
                    .pTexelBufferView = nullptr
                };
                vkCmdPushDescriptorSet(
                    commandBuffer, 
                    VK_PIPELINE_BIND_POINT_GRAPHICS, 
                    pipelineLayout2D, 
                    0, 
                    1, 
                    &writeMaterial
                );

                //Get the current entity's transform
                //Since draw implies transform, this is safe to do
                const Transform& t = sysTransform[entityID];
                //Create model matrix
                glm::mat4 modelMatrix(1.0f);
                modelMatrix[0][0] = t.scale.x * pixelsPerMeter;
                modelMatrix[1][1] = t.scale.y * pixelsPerMeter;
                modelMatrix[3] = glm::vec4(t.position * pixelsPerMeter, t.zLayer, 1.0f);
                //Push the model matrix to the shader
                vkCmdPushConstants(commandBuffer, pipelineLayout2D, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

                //Get the index for the current mesh within the array of vertex buffers
                const u64 meshIndex = entityManager.sysMesh->index(d.meshID);
                //Bind the index buffer at the end of the current mesh
                vkCmdBindIndexBuffer(commandBuffer, entityManager.sysMesh->handles()[meshIndex], entityManager.sysMesh->getIndexOffset(d.meshID), VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, entityManager.sysMesh->getNumIndices(d.meshID), 1, 0, meshIndex, 0);
            }

            vkCmdEndRendering(commandBuffer);
        }        

        Error<noreturn> createSyncObjects() noexcept {
            //One semaphore for each swapchain image - to tell which is presentable
            //Two semaphores for each frame - for 2D and 3D render targets 
            semaphores.resize(swapchainImages.size() + (2 * maxConcurrentFrames));
            const VkSemaphoreCreateInfo semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };
            for(VkSemaphore& semaphore : semaphores) {
                if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to create semaphores"};
                }
            }
            const VkFenceCreateInfo fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };
            for(VkFence& fence : fences) {
                if(vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to create fences!"};
                }
            }
            return success();
        }

        static Error<std::vector<char>> loadShaderFile(const std::string& filename) noexcept {
            std::ifstream shaderFile(filename, std::ios::binary | std::ios::ate);
            if(!shaderFile.is_open()) {
                return {ErrorTag::fatalError, "Failed to find shader code!"};
            }
            std::vector<char> code(shaderFile.tellg());
            shaderFile.seekg(0, std::ios::beg);
            shaderFile.read(code.data(), static_cast<std::streamsize>(code.size()));
            shaderFile.close();
            return code;
        }

        Error<VkShaderModule> createShaderModule(const std::string& filename) const noexcept {
            auto shader = loadShaderFile(filename);
            if(!shader) return shader.moveError<VkShaderModule>();
            const VkShaderModuleCreateInfo moduleInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .codeSize = shader.data().size(),
                .pCode = reinterpret_cast<const u32*>(shader.data().data())
            };
            VkShaderModule mod;
            if(vkCreateShaderModule(device, &moduleInfo, nullptr, &mod) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create shader module"};
            }
            return mod;
        }

        Error<noreturn> createGraphicsPipeline() noexcept {
            auto shaderMod2D = createShaderModule("shader2d.spv");
            if(!shaderMod2D) return shaderMod2D.moveError();
            auto shaderMod3D = createShaderModule("shader3d.spv");
            if(!shaderMod3D) return shaderMod3D.moveError();
            std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos2D;
            shaderStageInfos2D[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shaderMod2D,
                .pName = "vertMain",
                .pSpecializationInfo = nullptr
            };
            shaderStageInfos2D[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = shaderMod2D,
                .pName = "fragMain",
                .pSpecializationInfo = nullptr
            };
            std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos3D;
            shaderStageInfos3D[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shaderMod3D,
                .pName = "vertMain",
                .pSpecializationInfo = nullptr
            };
            shaderStageInfos3D[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = shaderMod3D,
                .pName = "fragMain",
                .pSpecializationInfo = nullptr
            };

            const VkVertexInputBindingDescription desc = Vertex::getInputBindingDescription();
            const auto attributeDescs = Vertex::getInputAttributeDescriptions();
            const VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &desc,
                .vertexAttributeDescriptionCount = attributeDescs.max_size(),
                .pVertexAttributeDescriptions = attributeDescs.data()
            };

            const VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE
            };

            const VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {
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

            const VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .rasterizationSamples = msaaSampleCount,
                .sampleShadingEnable = VK_FALSE,
                .minSampleShading = 0.0f,
                .pSampleMask = nullptr,
                .alphaToCoverageEnable = VK_FALSE,
                .alphaToOneEnable = VK_FALSE
            };

            const VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                .depthCompareOp = VK_COMPARE_OP_LESS,
                .depthBoundsTestEnable = VK_FALSE,
                .stencilTestEnable = VK_FALSE,
                .front = {},
                .back = {},
                .minDepthBounds = 0.0f,
                .maxDepthBounds = 1.0f
            };

            const VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
                .blendEnable = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
            };
            const VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .logicOpEnable = VK_FALSE,
                .logicOp = VK_LOGIC_OP_NO_OP,
                .attachmentCount = 1,
                .pAttachments = &colorBlendAttachmentState,
                .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
            };

            const std::array<VkFormat, 1> colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB};
            const VkPipelineRenderingCreateInfo pipelineRenderingInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext = nullptr,
                .viewMask = 0,
                .colorAttachmentCount = colorAttachmentFormats.max_size(),
                .pColorAttachmentFormats = colorAttachmentFormats.data(),
                .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED
            };

            const std::array<VkDynamicState, 2> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT, 
                VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT, 
            };
            const VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .dynamicStateCount = dynamicStates.max_size(),
                .pDynamicStates = dynamicStates.data()
            };

            const VkPushConstantRange pushConstantRanges2D[] = {
                //Model matrix being pushed
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                }
            };
            const VkDescriptorSetLayout descriptorSetLayouts2D[] = {
                pushSetLayout
            };
            const VkPipelineLayoutCreateInfo pipelineLayoutInfo2D = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = 1,
                .pSetLayouts = descriptorSetLayouts2D,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = pushConstantRanges2D
            };
            if(vkCreatePipelineLayout(device, &pipelineLayoutInfo2D, nullptr, &pipelineLayout2D) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create graphics pipeline layout!"};
            }
            const VkPushConstantRange pushConstantRanges3D[] = {
                //Model Matrix for the cube
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                }
            };
            const VkDescriptorSetLayout descriptorSetLayouts3D[] = {
                pushSetLayout
            };
            const VkPipelineLayoutCreateInfo pipelineLayoutInfo3D = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = 1,
                .pSetLayouts = descriptorSetLayouts3D,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = pushConstantRanges3D
            };
            if(vkCreatePipelineLayout(device, &pipelineLayoutInfo3D, nullptr, &pipelineLayout3D) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create graphics pipeline layout!"};
            }
             
            std::array<VkGraphicsPipelineCreateInfo, 2> graphicsPipelineInfos;
            //2D Pipeline Info
            graphicsPipelineInfos[0] = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &pipelineRenderingInfo,
                .flags = 0,
                .stageCount = shaderStageInfos2D.max_size(),
                .pStages = shaderStageInfos2D.data(),
                .pVertexInputState = &vertexInputStateInfo,
                .pInputAssemblyState = &inputAssemblyInfo,
                .pTessellationState = nullptr,
                .pViewportState = nullptr,
                .pRasterizationState = &rasterizationStateInfo,
                .pMultisampleState = &multisampleStateInfo,
                .pDepthStencilState = &depthStencilStateInfo,
                .pColorBlendState = &colorBlendStateInfo,
                .pDynamicState = &dynamicStateInfo,
                .layout = pipelineLayout2D, 
                .renderPass = nullptr,
                .subpass = 0,
                .basePipelineHandle = nullptr,
                .basePipelineIndex = 0
            };
            //3D Pipeline Info
            graphicsPipelineInfos[1] = graphicsPipelineInfos[0];
            graphicsPipelineInfos[1].stageCount = shaderStageInfos3D.max_size();
            graphicsPipelineInfos[1].pStages = shaderStageInfos3D.data();
            graphicsPipelineInfos[1].layout = pipelineLayout3D;

            if(vkCreateGraphicsPipelines(device, nullptr, graphicsPipelineInfos.max_size(), graphicsPipelineInfos.data(), nullptr, graphicsPipelines.data()) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create graphics pipeline!"};
            }
            vkDestroyShaderModule(device, shaderMod2D, nullptr);
            vkDestroyShaderModule(device, shaderMod3D, nullptr);

            return success();
        }
        Error<noreturn> getSwapchainImages(VkFormat format) noexcept {
            u32 imageCount;
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get swapchain images!"};
            }
            swapchainImages.resize(imageCount);
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get swapchain images!"};
            }
            swapchainImageViews.resize(imageCount);

            VkImageViewCreateInfo imageViewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = nullptr, //This will be updated for each image in a loop later
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = format,
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
            for(u32 i = 0; i < imageCount; i++) {
                imageViewInfo.image = swapchainImages[i];
                if(vkCreateImageView(device, &imageViewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, std::format("Failed to create swapchain image view {}!", i)};
                }
            }
            return success();
        }
        Error<noreturn> createCommandBuffers() noexcept {
            const VkCommandPoolCreateInfo poolInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = graphicsQueueFamilyIndex
            };
            if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create VkCommandPool!"};
            }
            const VkCommandBufferAllocateInfo allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = static_cast<u32>(commandBuffers.max_size())
            };
            if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to allocate command vertexBuffers!"};
            }
            return success();
        }
        Error<VkExtent2D> chooseImageExtent(VkSurfaceCapabilitiesKHR const& capabilities) noexcept {
            if(capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
                return capabilities.currentExtent;
            }
            int width = 0; 
            int height = 0;
            if(!SDL_GetWindowSize(window, &width, &height)) {
                return {ErrorTag::fatalError, SDL_GetError()};
            }
            return VkExtent2D{
                .width = std::clamp<u32>(width, 
                    capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                .height = std::clamp<u32>(height, 
                    capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };
        }


        Error<noreturn> createSwapchain(SurfaceInfo const& surfaceInfo) noexcept {
            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceInfo.capabilities);
            if(!imageExtent) return imageExtent.moveError();
            swapchainImageExtent = imageExtent;

            const VkSwapchainCreateInfoKHR swapchainInfo = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .surface = surface,
                .minImageCount = surfaceInfo.capabilities.minImageCount + 1,
                .imageFormat = surfaceInfo.format.format,
                .imageColorSpace = surfaceInfo.format.colorSpace,
                .imageExtent = swapchainImageExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 1,
                .pQueueFamilyIndices = nullptr,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = surfaceInfo.presentMode,
                .clipped = VK_TRUE,
                .oldSwapchain = nullptr
            };

            if(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create VkSwapchainKHR!"};
            }

            return success();
        }


        static Error<PickQueueFamilyIndexResult> pickQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
            PickQueueFamilyIndexResult result{};
            //Get queue family properties for the current physical device
            u32 queueFamilyPropertyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, nullptr);
            HeapArray<VkQueueFamilyProperties2> queueFamilyProperties(
                queueFamilyPropertyCount, 
                VkQueueFamilyProperties2{
                    .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2, 
                    .pNext = nullptr
                }
            );
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());
            //Check each queue family index for needed support
            for(u32 i = 0; i < queueFamilyProperties.size(); i++) {
                const u32 currentQueueCount = queueFamilyProperties[i].queueFamilyProperties.queueCount;
                if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & 
                        (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
                    if(currentQueueCount >= result.graphicsCount) {
                        result.graphicsIndex = i;
                        result.graphicsCount = currentQueueCount;
                    }
                }
                VkBool32 surfaceSupport;
                if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to get physical device surface support!"};
                }
                if(surfaceSupport == VK_TRUE and currentQueueCount >= result.presentCount) {
                    result.presentIndex = i;
                    result.presentCount = currentQueueCount;
                }
            }
            if(result.presentIndex == std::numeric_limits<u32>::max() 
                    or result.graphicsIndex == std::numeric_limits<u32>::max()) {
                return {ErrorTag::searchFail, "Physical Device did not have queue family with needed properties!"};
            }
            return result;
        }

        static Error<VkPresentModeKHR> choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
            VkPresentModeKHR presentMode{};
            //Get present modes for the surface supported by the physical device
            u32 presentModeCount = 0;
            if(vkGetPhysicalDeviceSurfacePresentModesKHR(
                    physicalDevice, 
                    surface, 
                    &presentModeCount, 
                    nullptr
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get physical device surface present modes!"};
            }
            HeapArray<VkPresentModeKHR> presentModes(presentModeCount);
            if(vkGetPhysicalDeviceSurfacePresentModesKHR(
                    physicalDevice, 
                    surface, 
                    &presentModeCount, 
                    presentModes.data()
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get physical device surface present modes!"};
            }
            //Select correct present mode
            for(VkPresentModeKHR mode : presentModes) {
                //This is our preferred present mode
                if(mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
            }
            //This mode is always supported, and is preferred over other options if mailbox isn't available
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        static Error<VkSurfaceFormatKHR> checkDeviceSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
            //Get physical device's surface formats
            u32 surfaceFormatCount = 0;
            if(vkGetPhysicalDeviceSurfaceFormatsKHR(
                    physicalDevice, surface, &surfaceFormatCount, nullptr
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get physical device surface formats!"};
            }
            HeapArray<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            if(vkGetPhysicalDeviceSurfaceFormatsKHR(
                    physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to get physical device surface formats!"};
            }
            for(int i = 0; i < surfaceFormatCount; i++) {
                if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB 
                    && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
                    ) {
                    return surfaceFormats[i];
                }
            }
            return surfaceFormats[0];
        }

        static bool checkDeviceFeatureSupport(VkPhysicalDevice physicalDevice) noexcept {
            //Set up chain of feature structs
            VkPhysicalDeviceVulkan14Features f14 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
                .pNext = nullptr
            };
            VkPhysicalDeviceVulkan13Features f13 = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .pNext = &f14
            };
            VkPhysicalDeviceFeatures2 deviceFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &f13
            };
            vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);
            //Check features
            if(deviceFeatures.features.samplerAnisotropy != VK_TRUE) return false;
            if(f13.dynamicRendering != VK_TRUE 
                or f13.maintenance4 != VK_TRUE 
                or f13.synchronization2 != VK_TRUE) return false;
            if(f14.pushDescriptor != VK_TRUE) return false;
            return true;
        }

        static Error<noreturn> checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) noexcept {
            //Check that the device supports the extensions needed by the application
            u32 deviceExtensionPropertyCount = 0;
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, 
                    &deviceExtensionPropertyCount, nullptr
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to enumerate physical device extension properties!"};
            }
            HeapArray<VkExtensionProperties> extensionProperties(deviceExtensionPropertyCount);
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, 
                &deviceExtensionPropertyCount, extensionProperties.data()
                ) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to enumerate physical device extension properties!"};
            }
            //Construct set of extension names
            std::set<std::string> unfoundExtensions;
            for(int i = 0; i < neededDeviceExtensions.size(); i++) {
                unfoundExtensions.emplace(neededDeviceExtensions[i]);
            }
            //Check if we have every needed extension
            for(auto& extensionProperty : extensionProperties) {
                //Remove the current extension name from the set, if we have it
                unfoundExtensions.erase(extensionProperty.extensionName);
                //Once we've "ticked off" each name from the set, we know the device supports all the needed
                //	extensions
                if(unfoundExtensions.empty()) return success();
            }
            return {ErrorTag::searchFail, "Physical Device did not support needed extensions!"};
        }

        static Error<PickPhysicalDeviceResult> pickPhysicalDevice(std::vector<VkPhysicalDevice>const& physicalDevices, VkSurfaceKHR surface) noexcept {
            PickPhysicalDeviceResult result{};
            for(VkPhysicalDevice const& currentPhysicalDevice : physicalDevices) {
                //Check device extension support for the current physical device
                switch(Error<noreturn> procResult = checkDeviceExtensionSupport(currentPhysicalDevice); procResult.tag()) {
                    case ErrorTag::searchFail: continue;
                    case ErrorTag::fatalError: return procResult.moveError<PickPhysicalDeviceResult>();
                    case ErrorTag::allOkay: break;
                }

                //Check physical device's support for needed features
                if(!checkDeviceFeatureSupport(currentPhysicalDevice)) {
                    continue;
                }

                //Get physical device's capabilities with the surface
                if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                        currentPhysicalDevice, 
                        surface, 
                        &result.surfaceInfo.capabilities
                    ) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to get physical device surface capabilities!"};
                }

                //Get a surface format to use
                if(Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(currentPhysicalDevice, surface)) {
                    result.surfaceInfo.format = surfaceFormat;
                }
                else return surfaceFormat.moveError<PickPhysicalDeviceResult>();

                //Choose a present mode to use
                switch(Error<VkPresentModeKHR> presentMode = choosePresentMode(currentPhysicalDevice, surface); presentMode.tag()) {
                    case ErrorTag::searchFail: continue;
                    case ErrorTag::fatalError: return presentMode.moveError<PickPhysicalDeviceResult>();
                    case ErrorTag::allOkay: result.surfaceInfo.presentMode = presentMode;
                }

                //Pick the desired queue family index
                switch(Error<PickQueueFamilyIndexResult> queueFamilyInfo = pickQueueFamilyIndex(currentPhysicalDevice, surface); queueFamilyInfo.tag()) {
                    case ErrorTag::searchFail: continue;
                    case ErrorTag::fatalError: return queueFamilyInfo.moveError<PickPhysicalDeviceResult>();
                    case ErrorTag::allOkay: result.queueFamilyInfo = queueFamilyInfo;
                }

                //At this point if all has succeeded, we are ready to use the current physical device and
                // create the logical device
                result.physicalDevice = currentPhysicalDevice;
                return result;
            }
            return {ErrorTag::searchFail, "Failed to find suitable physical device"};
        }

        Error<noreturn> createDevice(SurfaceInfo& surfaceInfo) noexcept {
            u32 graphicsQueueCount = 0;
            u32 presentQueueCount = 0;

            //Get all the physical devices installed on the system
            u32 physicalDeviceCount = 0;
            if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to enumerate physical devices!"};
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to enumerate physical devices!"};
            }

            //Pick a physical device to use
            if(const Error<PickPhysicalDeviceResult> pickResult = 
                    pickPhysicalDevice(physicalDevices, surface)
                ) {
                PickPhysicalDeviceResult const& res = pickResult.data();
                physicalDevice = res.physicalDevice;
                graphicsQueueFamilyIndex = res.queueFamilyInfo.graphicsIndex;
                presentQueueFamilyIndex = res.queueFamilyInfo.presentIndex;
                graphicsQueueCount = res.queueFamilyInfo.graphicsCount;
                presentQueueCount = res.queueFamilyInfo.presentCount;
                surfaceInfo = res.surfaceInfo;
            }
            //pickPhysicalDevice just returns searchFail if none of the devices are suitable, but 
            // I want this case to be fatal instead
            else return {ErrorTag::fatalError, pickResult.message()};

            VkPhysicalDeviceProperties2 props = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = nullptr
            };
            vkGetPhysicalDeviceProperties2(physicalDevice, &props);
            const VkSampleCountFlags availableSampleCounts = props.properties.limits.framebufferColorSampleCounts;
            if(availableSampleCounts & VK_SAMPLE_COUNT_64_BIT) msaaSampleCount = VK_SAMPLE_COUNT_64_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_32_BIT) msaaSampleCount = VK_SAMPLE_COUNT_32_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_16_BIT) msaaSampleCount = VK_SAMPLE_COUNT_16_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_8_BIT) msaaSampleCount = VK_SAMPLE_COUNT_8_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_4_BIT) msaaSampleCount = VK_SAMPLE_COUNT_4_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_2_BIT) msaaSampleCount = VK_SAMPLE_COUNT_2_BIT;
            else msaaSampleCount = VK_SAMPLE_COUNT_1_BIT;

            const bool differentQueueFamilies = graphicsQueueFamilyIndex != presentQueueFamilyIndex;
            std::array<VkDeviceQueueCreateInfo, 2> queueCreateInfos;
            u32 queueCreateInfoCount = 1U;
            HeapArray<float> queuePriorities;

            if(differentQueueFamilies) {
                queueCreateInfoCount = 2U;
                graphicsQueueCount = std::min(graphicsQueueCount, maxConcurrentFrames);
                presentQueueCount = std::min(presentQueueCount, maxConcurrentFrames);
                queuePriorities.init(graphicsQueueCount + presentQueueCount, 0.0f);
                queueCreateInfos[0] = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = graphicsQueueFamilyIndex,
                    .queueCount = graphicsQueueCount,
                    .pQueuePriorities = &queuePriorities[0]
                };
                queueCreateInfos[1] = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = presentQueueFamilyIndex,
                    .queueCount = presentQueueCount,
                    .pQueuePriorities = &queuePriorities[graphicsQueueCount]
                };
            }
            else {
                graphicsQueueCount = std::min(graphicsQueueCount, 2U * maxConcurrentFrames);
                queuePriorities.init(graphicsQueueCount, 0.0f);
                queueCreateInfos[0] = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .queueFamilyIndex = graphicsQueueFamilyIndex,
                    .queueCount = graphicsQueueCount,
                    .pQueuePriorities = queuePriorities.data()
                };
            }

            VkPhysicalDeviceVulkan14Features vulkan14Features{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
                .pNext = nullptr,
                .pushDescriptor = VK_TRUE
            };
            VkPhysicalDeviceVulkan13Features vulkan13Features{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                .pNext = &vulkan14Features,
                .synchronization2 = VK_TRUE,
                .dynamicRendering = VK_TRUE,
                .maintenance4 = VK_TRUE
            };
            VkPhysicalDeviceFeatures2 features2{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
                .pNext = &vulkan13Features,
                .features = {
                    .samplerAnisotropy = VK_TRUE
                }
            };

            const VkDeviceCreateInfo deviceInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &features2,
                .flags = 0,
                .queueCreateInfoCount = queueCreateInfoCount,
                .pQueueCreateInfos = queueCreateInfos.data(),
                .enabledExtensionCount = static_cast<u32>(neededDeviceExtensions.size()),
                .ppEnabledExtensionNames = neededDeviceExtensions.data(),
                .pEnabledFeatures = nullptr
            };

            if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create VkDevice!"};
            }

            //Acquire handles to all the GPU queues we just created
            if(differentQueueFamilies) {
                queues.resize(graphicsQueueCount + presentQueueCount);
                for(u32 i = 0; i < graphicsQueueCount; i++) {
                    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, i, &queues[i]);
                }
                for(u32 i = 0; i < presentQueueCount; i++) {
                    vkGetDeviceQueue(device, presentQueueFamilyIndex, i, &queues[graphicsQueueCount + i]);
                }
            }
            else {
                queues.resize(graphicsQueueCount);
                for(u32 i = 0; i < graphicsQueueCount; i++) {
                    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, i, &queues[i]);
                }
                //I want the truncating of the division result because it ensures that there are
                // never more queues allocated to presentation than graphics
                graphicsQueueCount -= (graphicsQueueCount / 2U);
            }

            Error<Slice<VkQueue>> slice = Slice<VkQueue>::make(queues, 0, graphicsQueueCount);
            if(!slice) return slice.moveError();
            graphicsQueues = slice;
            if(graphicsQueueCount == 1U) {
                presentQueues = graphicsQueues;
            }
            else {
                slice = Slice<VkQueue>::make(queues, graphicsQueueCount);
                if(!slice) return slice.moveError();
                presentQueues = slice;
            }

            return success();
        }

        Error<noreturn> createDebugUtilsMessenger() noexcept {
            vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
            if(vkCreateDebugUtilsMessengerEXT == nullptr) {
                return {ErrorTag::fatalError, "Failed to find function vkCreateDebugUtilsMessengerEXT!"};
            }
            vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            if(vkDestroyDebugUtilsMessengerEXT == nullptr) {
                return {ErrorTag::fatalError, "Failed to find function vkDestroyDebugUtilsMessengerEXT!"};
            }
            const VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {
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

            if(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create vkDebugUtilsMessengerEXT!"};
            }
            return success();
        }

        Error<noreturn> createVkInstance(const std::string& appName) noexcept {
            if constexpr(enableValidationLayers) {
                u32 layerPropertyCount = 0;
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to enumerate instance layer properties!"};
                }
                std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data()) != VK_SUCCESS) {
                    return {ErrorTag::fatalError, "Failed to enumerate instance layer properties!"};
                }
                for(char const* layer : validationLayers) {
                    bool foundLayer = false;
                    for(VkLayerProperties const& property : layerProperties) {
                        if(std::strcmp(layer, property.layerName) == 0) {
                            foundLayer = true;
                            break;
                        }
                    }
                    if(!foundLayer) {
                        return {ErrorTag::fatalError, std::format("Needed layer \"{}\" not found", layer)};
                    }
                }
            }
            u32 extCount = 0;
            char const*const* sdlExtNames = SDL_Vulkan_GetInstanceExtensions(&extCount);
            std::vector<char const*> extNames;
            extNames.reserve(extCount);
            extNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            for(int i = 0; i < extCount; i++) {
                extNames.push_back(sdlExtNames[i]);
            }

            VkApplicationInfo const appInfo = {
                .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pNext = nullptr,
                .pApplicationName = appName.c_str(),
                .applicationVersion = 1,
                .pEngineName = nullptr,
                .engineVersion = 0,
                .apiVersion = VK_MAKE_API_VERSION(1, 4, 335, 0)
            };
            VkInstanceCreateInfo instanceInfo = {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .pApplicationInfo = &appInfo,
                .enabledLayerCount = 0,
                .ppEnabledLayerNames = nullptr,
                .enabledExtensionCount = static_cast<u32>(extNames.size()),
                .ppEnabledExtensionNames = extNames.data()
            };
            if constexpr(enableValidationLayers) {
                instanceInfo.enabledLayerCount = validationLayers.size();
                instanceInfo.ppEnabledLayerNames = validationLayers.data();
            }

            if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
                return {ErrorTag::fatalError, "Failed to create VkInstance!"};
            }
            return success();
        }

        Error<noreturn> createSurface(std::string const& name) noexcept {
            int numDisplays;
            SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
            if(displays == nullptr) {
                return {ErrorTag::fatalError, SDL_GetError()};
            }
            //Be lazy and just use the first display
            SDL_Rect displayBounds{};
            if(!SDL_GetDisplayBounds(displays[0], &displayBounds)) {
                return {ErrorTag::fatalError, SDL_GetError()};
            }
            //SDL_WINDOW_MOUSE_GRABBED : mouse cannot escape window bounds - allows using relative
            // mouse mode
            //SDL_WINDOW_HIDDEN : hide the window before we're ready to display images to it
            window = SDL_CreateWindow(name.c_str(), displayBounds.w, displayBounds.h, 
                SDL_WINDOW_VULKAN | SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIDDEN);
            //Instead of tracking live mouse inputs and having on-screen cursor, just track
            // changes in mouse position
            if(!SDL_SetWindowRelativeMouseMode(window, true)) {
                return {ErrorTag::fatalError, SDL_GetError()};
            }
            if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
                return {ErrorTag::fatalError, SDL_GetError()};
            }
            SDL_free(displays);
            return success();
        }
        public:
        RendererFlags flags;
        static Error<Renderer*> make(const std::string& name, ID& squareID) noexcept {
            Renderer* r = new Renderer;
            #define RTRY(proc) \
            if(auto res = proc; !res) do{\
                delete r;\
                return res.moveError<Renderer*>();\
            } while(false)
            if(auto res = r->createVkInstance(name); !res) {
                r->flags |= flagBits::instanceInvalid;
                delete r;
                return res.moveError<Renderer*>();
            }
            if constexpr(enableValidationLayers) RTRY(r->createDebugUtilsMessenger());
            RTRY(r->createSurface(name));
            SurfaceInfo surfaceInfo{};
            if(Error<noreturn> res = r->createDevice(surfaceInfo); !res) {
                r->flags |= flagBits::deviceInvalid;
                delete r;
                return res.moveError<Renderer*>();
            }
            RTRY(r->createSwapchain(surfaceInfo));
            RTRY(r->getSwapchainImages(surfaceInfo.format.format));
            RTRY(r->createCommandBuffers());
            RTRY(r->createCube());
            RTRY(r->createSampler());
            RTRY(r->createRenderTargets(surfaceInfo.format.format));
            RTRY(r->createDepthImages());
            RTRY(r->createSyncObjects());
            RTRY(r->createDescriptorSetLayouts());
            RTRY(r->createGraphicsPipeline());
            RTRY(r->createCamera());
            if(!SDL_ShowWindow(r->window)) {
                delete r;
                return {ErrorTag::fatalError, "Failed to show window"};
            }

            //Create square mesh
            auto sq = r->makeMesh(Square::vertices, Square::indices);
            if(!sq) {
                delete r;
                return sq.moveError<Renderer*>();
            }
            //Give ID number of square mesh back to the game
            else squareID = sq;

            r->currentFrame = 0;
            r->flags = {};
            return r;
            #undef RTRY
        }
        //Destructor
        ~Renderer() noexcept {
            if(!(flags & flagBits::deviceInvalid)) [[likely]] {
                vkDeviceWaitIdle(device);
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                for(VkImageView view : swapchainImageViews) {
                    vkDestroyImageView(device, view, nullptr);
                }
                cube.buffer.destroy(device);
                entityManager.sysMesh->destroySystem(device);
                for(u64 i = 0; i < entityManager.sysTexture->size(); i++) {
                    Texture& t = entityManager.sysTexture->data()[i];
                    vkDestroyImage(device, t.handle, nullptr);
                    vkFreeMemory(device, t.memory, nullptr);
                    vkDestroyImageView(device, t.view, nullptr);
                }
                vkDestroyDescriptorSetLayout(device, pushSetLayout, nullptr);
                for(VkSemaphore semaphore : semaphores) {
                    vkDestroySemaphore(device, semaphore, nullptr);
                }
                for(VkFence fence : fences) {
                    vkDestroyFence(device, fence, nullptr);
                }
                depthImage2D.destroy(device);
                depthImage3D.destroy(device);
                for(GPUImage& target : renderTargets2D) target.destroy(device);
                for(GPUImage& texture : textures2DScene) texture.destroy(device);
                for(GPUImage& target : renderTargets3D) target.destroy(device);
                vkDestroySampler(device, sampler, nullptr);
                vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
                vkDestroyCommandPool(device, commandPool, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout2D, nullptr);
                vkDestroyPipelineLayout(device, pipelineLayout3D, nullptr);
                for(VkPipeline pipeline : graphicsPipelines) vkDestroyPipeline(device, pipeline, nullptr);
                cameraMatrices.destroy(device);
                vkDestroyDevice(device, nullptr);
            }
            if(!(flags & flagBits::instanceInvalid)) [[likely]] {
                SDL_Vulkan_DestroySurface(instance, surface, nullptr);
                SDL_DestroyWindow(window);
                if constexpr(enableValidationLayers) {
                    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
                }
                vkDestroyInstance(instance, nullptr);
            }
        }

        Error<noreturn> draw(Camera camera, float pixelsPerMeter, const ComponentSystem<Transform>& sysTransform, const ComponentSystem<Draw>& sysDraw) noexcept {
            if(camera.aspect == Camera::autoAspect) {
                camera.aspect = static_cast<float>(swapchainImageExtent.width) / swapchainImageExtent.height;
            }
            cameraMatrices.mappedMemory[1 + currentFrame] = camera.loadMatrices();
            return drawFrame(camera.fov, camera.aspect, pixelsPerMeter, sysTransform, sysDraw);
        }

        Error<ID> makeMesh(ConstSlice<Vertex> vertices, ConstSlice<u32> indices) noexcept {
            const VkDeviceSize verticesSize = vertices.size() * sizeof(Vertex);
            const VkDeviceSize indicesSize = indices.size() * sizeof(u32);
            const VkDeviceSize size = verticesSize + indicesSize;
            Error<GPUBuffer> buffer = GPUBuffer::make(device, physicalDevice, size, 
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if(!buffer) return buffer.moveError<std::size_t>();
            GPUBuffer vertexBuffer = buffer.moveData();
            
            buffer = GPUBuffer::make(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if(!buffer) {
                vertexBuffer.destroy(device);
                return buffer.moveError<std::size_t>();
            }
            GPUBuffer transferBuffer = buffer.moveData();

            void* memory;
            if(vkMapMemory(device, transferBuffer.memory, 0, verticesSize, 0, &memory) != VK_SUCCESS) {
                vertexBuffer.destroy(device);
                transferBuffer.destroy(device);
                return fatal<std::size_t>("Failed to map device memory");
            }
            std::memcpy(memory, reinterpret_cast<void const*>(vertices.data()), verticesSize);
            vkUnmapMemory(device, transferBuffer.memory);

            if(vkMapMemory(device, transferBuffer.memory, verticesSize, indicesSize, 0, &memory) != VK_SUCCESS) {
                vertexBuffer.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to map device memory"};
            }
            std::memcpy(memory, reinterpret_cast<void const*>(indices.data()), indicesSize);
            vkUnmapMemory(device, transferBuffer.memory);

            if(!(flags & flagBits::beganTransferOps)) {
                Error<noreturn> res = beginTransferOps();
                if(!res) {
                    vertexBuffer.destroy(device);
                    transferBuffer.destroy(device);
                    return res.moveError<std::size_t>();
                }
            }

            const VkBufferCopy region = {
                .srcOffset = 0, 
                .dstOffset = 0, 
                .size = size
            };
            vkCmdCopyBuffer(commandBuffers.back(), transferBuffer.handle, vertexBuffer.handle, 1, &region);

            if(vkEndCommandBuffer(commandBuffers.back()) != VK_SUCCESS) {
                vertexBuffer.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to end transfer command buffer"};
            }
            const VkCommandBufferSubmitInfo commandBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, 
                .pNext = nullptr,
                .commandBuffer = commandBuffers.back(),
                .deviceMask = 0
            };
            const VkSubmitInfo2 submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = 0,
                .pWaitSemaphoreInfos = nullptr,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferInfo,
                .signalSemaphoreInfoCount = 0,
                .pSignalSemaphoreInfos = nullptr
            };
            if(vkQueueSubmit2(graphicsQueues[0], 1, &submitInfo, nullptr) != VK_SUCCESS) {
                vertexBuffer.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to submit to transfer queue while creating vertex buffer"};
            }
            vkQueueWaitIdle(graphicsQueues[0]);
            vkResetCommandBuffer(commandBuffers.back(), 0);
            flags &= ~flagBits::beganTransferOps;
            transferBuffer.destroy(device);

            return entityManager.insertMesh(vertexBuffer.handle, vertexBuffer.memory, verticesSize, indicesSize / sizeof(u32));
        }

        Error<ID> makeTexture(std::string const& texturePath) noexcept {
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = nullptr;
            pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if(pixels == nullptr) {
                return {ErrorTag::fatalError, "Failed to find/load texture file"};
            }
            const VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha; 
            GPUImage textureImage;

            if(Error<GPUImage> image = GPUImage::make(
                    device, physicalDevice, 
                    {static_cast<u32>(texWidth), static_cast<u32>(texHeight), 1},
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT
                ); !image) {
                stbi_image_free(pixels);
                return image.moveError<std::size_t>();
            }
            else textureImage = image;

            GPUBuffer transferBuffer;
            if(Error<GPUBuffer> tb = GPUBuffer::make(
                    device, 
                    physicalDevice, 
                    imageSize, 
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                ); !tb) {
                stbi_image_free(pixels);
                textureImage.destroy(device);
                return tb.moveError<std::size_t>();
            }
            else transferBuffer = tb;

            void* memory;
            if(vkMapMemory(device, transferBuffer.memory, 0, imageSize, 0, &memory) != VK_SUCCESS) {
                stbi_image_free(pixels);
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to map buffer memory to the host"};
            }
            std::memcpy(memory, pixels, imageSize);
            vkUnmapMemory(device, transferBuffer.memory);

            stbi_image_free(pixels);

            const VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to begin single time command buffer"};
            }

            const VkImageMemoryBarrier2 firstTransition = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = textureImage.handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            const VkDependencyInfo dep1 = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &firstTransition
            };
            vkCmdPipelineBarrier2(commandBuffers.back(), &dep1);

            const VkBufferImageCopy2 copyRegion = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
                .pNext = nullptr,
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                .imageOffset = {0,0,0},
                .imageExtent = {static_cast<u32>(texWidth), static_cast<u32>(texHeight), 1}
            };
            const VkCopyBufferToImageInfo2 bufferToImageInfo = {
                .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                .pNext = nullptr,
                .srcBuffer = transferBuffer.handle,
                .dstImage = textureImage.handle,
                .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount = 1,
                .pRegions = &copyRegion
            };
            vkCmdCopyBufferToImage2(commandBuffers.back(), &bufferToImageInfo);

            const VkImageMemoryBarrier2 preShaderRead = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = textureImage.handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            const VkDependencyInfo dep2 = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = nullptr,
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &preShaderRead
            };
            vkCmdPipelineBarrier2(commandBuffers.back(), &dep2);

            if(vkEndCommandBuffer(commandBuffers.back()) != VK_SUCCESS) {
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to end command buffer while creating texture image"};
            }

            const VkCommandBufferSubmitInfo commandInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = commandBuffers.back(),
                .deviceMask = 0
            };
            const VkSubmitInfo2 submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = 0,
                .pWaitSemaphoreInfos = nullptr,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandInfo,
                .signalSemaphoreInfoCount = 0,
                .pSignalSemaphoreInfos = nullptr
            };
            if(vkQueueSubmit2(graphicsQueues[0], 1, &submitInfo, nullptr) != VK_SUCCESS) {
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to submit to queue while creating texture image"};
            }
            if(vkQueueWaitIdle(graphicsQueues[0]) != VK_SUCCESS) {
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to wait for queue while creating texture image"};
            }
            if(vkResetCommandBuffer(commandBuffers.back(), 0) != VK_SUCCESS) {
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return {ErrorTag::fatalError, "Failed to reset command buffer while creating texture image"};
            }

            transferBuffer.destroy(device);
            Texture t;
            t.handle = textureImage.handle;
            t.memory = textureImage.memory;
            t.view = textureImage.view;
            return entityManager.insertTexture(t);
        }
    };
}
