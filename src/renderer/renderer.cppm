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
#include <cstdint>
#include <limits>
#include <chrono>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "mars_macros.h"

#define MAX_CONCURRENT_FRAMES 2U

export module mars:renderer;
import maps;
import gpubuffer;
import gpuimage;
import error;
import heap_array;
import flag_bits;
import vkhelper;

namespace mars {
    constexpr std::array<char const*, 2> neededDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
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

    VkBool32 debugCallback(
	    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	    VkDebugUtilsMessengerCallbackDataEXT const*	pCallbackData,
	    void* pUserData
    ) {
        std::println("Validation Layer:\n\tSeverity: {}", vkhelper::messageSeverityToString(messageSeverity));
        std::println("\tMessage: {}\n", pCallbackData->pMessage);
        return VK_FALSE;
    }

    struct Vertex {
        alignas(4) glm::vec3 pos;
        alignas(4) glm::vec3 color;
        alignas(4) glm::vec2 texCoord;

        constexpr Vertex(glm::vec3 inPos, glm::vec2 inTexCoord) noexcept : pos(inPos), color(glm::vec3{0.0f}), texCoord(inTexCoord) {}

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

    constexpr glm::vec3 topleft{0.0f, 0.0f, 0.0f};
    constexpr glm::vec3 topright{1.0f, 0.0f, 0.0f};
    constexpr glm::vec3 bottomright{1.0f, 1.0f, 0.0f};
    constexpr glm::vec3 bottomleft{0.0f, 1.0f, 0.0f};
    constexpr glm::vec3 toprightback{1.0f, 0.0f, 1.0f};
    constexpr glm::vec3 topleftback{0.0f, 0.0f, 1.0f};
    constexpr glm::vec3 bottomleftback{0.0f, 1.0f, 1.0f};
    constexpr glm::vec3 bottomrightback{1.0f, 1.0f, 1.0f};
    export namespace Cube {
        constexpr std::array<Vertex, 24> vertices = {
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
        constexpr std::array<std::uint32_t, 36> indices = {
            0, 1, 2, 0, 2, 3, //FRONT FACE
            4, 5, 6, 4, 6, 7, //RIGHT FACE
            8, 9, 10, 8, 10, 11, //BACK FACE
            12, 13, 14, 12, 14, 15, //LEFT FACE
            16, 17, 18, 16, 18, 19, //TOP FACE
            20, 21, 22, 20, 22, 23 //BOTTOM FACE
        };
    }

    export class Renderer {
        friend class Game;
        VertexBuffers vertexBuffers;
        Textures textures;
        UniformBuffer<glm::mat4> cameraMatrices;
    	HeapArray<VkImage> swapchainImages;
    	HeapArray<VkImageView> swapchainImageViews;	
        HeapArray<GPUImage> renderTargets;
        HeapArray<VkQueue> queues;
        Slice<VkQueue> graphicsQueues;
        Slice<VkQueue> presentQueues;
        HeapArray<VkSemaphore> semaphores;
        GPUImage depthImage;
    	std::array<VkCommandBuffer, MAX_CONCURRENT_FRAMES + 1> commandBuffers;
        std::array<VkFence, MAX_CONCURRENT_FRAMES> fences;
        VkDescriptorSetLayout descriptorSetLayout;
        VkInstance instance;
        SDL_Window* window;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VkDevice device;
        VkPhysicalDevice physicalDevice;
        VkSwapchainKHR swapchain;
        VkExtent2D swapchainImageExtent;
    	VkCommandPool commandPool;
    	VkPipeline graphicsPipeline;
        VkPipelineLayout graphicsPipelineLayout;
        VkSampler sampler;
        std::uint32_t currentFrame;
        std::uint32_t graphicsQueueFamilyIndex;
        std::uint32_t presentQueueFamilyIndex;
        VkSampleCountFlagBits msaaSampleCount;
        RendererFlags flags;

        //Due to the circular dependency of the Mesh and Renderer classes, this has to be a renderer method
        Error<std::size_t> makeMesh(ConstSlice<Vertex> vertices, ConstSlice<std::uint32_t> indices, std::uint64_t timeNanos) noexcept {
            VkDeviceSize const verticesSize = vertices.size() * sizeof(Vertex);
            VkDeviceSize const indicesSize = indices.size() * sizeof(std::uint32_t);
            VkDeviceSize const size = verticesSize + indicesSize;
            Error<GPUBuffer> buffer = GPUBuffer::make(device, physicalDevice, size, 
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if(!buffer) return buffer.moveError<std::size_t>();
            GPUBuffer vertexBuffer = buffer.moveData();
            
            buffer = GPUBuffer::make(device, physicalDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if(!buffer) return buffer.moveError<std::size_t>();
            GPUBuffer transferBuffer = buffer.moveData();

            void* memory;
            if(vkMapMemory(device, transferBuffer.memory, 0, verticesSize, 0, &memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map device memory"};
            }
            std::memcpy(memory, reinterpret_cast<void const*>(vertices.data()), verticesSize);
            vkUnmapMemory(device, transferBuffer.memory);

            if(vkMapMemory(device, transferBuffer.memory, verticesSize, indicesSize, 0, &memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map device memory"};
            }
            std::memcpy(memory, reinterpret_cast<void const*>(indices.data()), indicesSize);
            vkUnmapMemory(device, transferBuffer.memory);

            if(!(flags & flagBits::beganTransferOps)) {
                Error<noreturn> res = beginTransferOps();
                if(!res) return res.moveError<std::size_t>();
            }

            VkBufferCopy const region = {
                .srcOffset = 0, 
                .dstOffset = 0, 
                .size = size
            };
            vkCmdCopyBuffer(commandBuffers.back(), transferBuffer.handle, vertexBuffer.handle, 1, &region);
            if(vkEndCommandBuffer(commandBuffers.back()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to end transfer command buffer"};
            }
            VkCommandBufferSubmitInfo const commandBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, 
                .pNext = nullptr,
                .commandBuffer = commandBuffers.back(),
                .deviceMask = 0
            };
            VkSubmitInfo2 const submitInfo = {
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
                return {ErrorTag::FATAL_ERROR, "Failed to submit to transfer queue while creating vertex buffer"};
            }
            vkQueueWaitIdle(graphicsQueues[0]);
            vkResetCommandBuffer(commandBuffers.back(), 0);
            flags &= ~flagBits::beganTransferOps;
            transferBuffer.destroy(device);

            return vertexBuffers.append(vertexBuffer.handle, vertexBuffer.memory, {verticesSize, indicesSize});
        }

        struct Objects {
            glm::mat4 const* modelMatrices;
            std::size_t const* meshIDs;
            std::size_t const* textureIDs;
            std::size_t size;
        };

        struct SurfaceInfo {
            VkSurfaceCapabilitiesKHR capabilities;
            VkPresentModeKHR presentMode;
        	VkSurfaceFormatKHR format;
        };

        struct PickQueueFamilyIndexResult {
            std::uint32_t presentIndex = std::numeric_limits<std::uint32_t>::max();
            std::uint32_t presentCount = 0U;
            std::uint32_t graphicsIndex = std::numeric_limits<std::uint32_t>::max();
            std::uint32_t graphicsCount = 0U;
        };

        struct PickPhysicalDeviceResult {
            VkPhysicalDevice physicalDevice = nullptr;
            PickQueueFamilyIndexResult queueFamilyInfo;
            SurfaceInfo surfaceInfo;
        };

        Error<noreturn> beginTransferOps() noexcept {
            VkCommandBufferBeginInfo const beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to begin single time command buffer"}; 
            }
            flags |= flagBits::beganTransferOps;
            return success();
        }

        Error<noreturn> createRenderTargets(VkFormat format) noexcept {
            renderTargets.resize(swapchainImages.size());
            for(GPUImage& target : renderTargets) {
                Error<GPUImage> t = GPUImage::make(
                    device, 
                    physicalDevice, 
                    {swapchainImageExtent.width, swapchainImageExtent.height, 1U}, 
                    msaaSampleCount, 
                    VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    format, VK_IMAGE_ASPECT_COLOR_BIT
                );
                if(!t) return t.moveError();
                target = t;
            }
            return success();
        }

        Error<noreturn> createDepthImage() noexcept {
            Error<GPUImage> dp = GPUImage::make(device, physicalDevice,
                {swapchainImageExtent.width, swapchainImageExtent.height, 1},
                msaaSampleCount, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
            if(!dp) return dp.moveError();
            depthImage = dp;
            return success();
        }

        Error<noreturn> createDescriptorSetLayouts() noexcept {
            std::array<VkDescriptorSetLayoutBinding, 2> constexpr pushBindings = {
                VkDescriptorSetLayoutBinding{
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = nullptr
                },
                VkDescriptorSetLayoutBinding{
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                }
            };
            VkDescriptorSetLayoutCreateInfo const pushLayout = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
                .bindingCount = pushBindings.max_size(),
                .pBindings = pushBindings.data()
            };
            if(vkCreateDescriptorSetLayout(device, &pushLayout, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create descriptor set layout"};
            }
            return success();
        }

        Error<noreturn> createSampler() noexcept {
            VkPhysicalDeviceProperties2 props{};
            props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(physicalDevice, &props);
            VkSamplerCreateInfo const samplerInfo = {
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
                return {ErrorTag::FATAL_ERROR, "Failed to create texture sampler"};
            }
            return success();
        }

        Error<std::size_t> createTexture(std::string const& texturePath, std::uint64_t timeNanos) noexcept {
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = nullptr;
            pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if(pixels == nullptr) {
                return {ErrorTag::FATAL_ERROR, "Failed to find/load texture file"};
            }
            VkDeviceSize const imageSize = texWidth * texHeight * STBI_rgb_alpha; 
            GPUImage textureImage;

            if(Error<GPUImage> image = GPUImage::make(
                    device, physicalDevice, 
                    {static_cast<std::uint32_t>(texWidth), static_cast<std::uint32_t>(texHeight), 1},
                    VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT
                ); !image) {
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
                return tb.moveError<std::size_t>();
            }
            else transferBuffer = tb;

            void* memory;
            if(vkMapMemory(device, transferBuffer.memory, 0, imageSize, 0, &memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map buffer memory to the host"};
            }
            std::memcpy(memory, pixels, imageSize);
            vkUnmapMemory(device, transferBuffer.memory);

            stbi_image_free(pixels);

            VkCommandBufferBeginInfo const beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to begin single time command buffer"};
            }

            VkImageMemoryBarrier2 const firstTransition = {
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
            VkDependencyInfo const dep1 = {
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

            VkBufferImageCopy2 const copyRegion = {
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
                .imageExtent = {static_cast<std::uint32_t>(texWidth), static_cast<std::uint32_t>(texHeight), 1}
            };
            VkCopyBufferToImageInfo2 const bufferToImageInfo = {
                .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
                .pNext = nullptr,
                .srcBuffer = transferBuffer.handle,
                .dstImage = textureImage.handle,
                .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .regionCount = 1,
                .pRegions = &copyRegion
            };
            vkCmdCopyBufferToImage2(commandBuffers.back(), &bufferToImageInfo);

            VkImageMemoryBarrier2 const preShaderRead = {
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
            VkDependencyInfo const dep2 = {
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
                return {ErrorTag::FATAL_ERROR, "Failed to end command buffer while creating texture image"};
            }

            VkCommandBufferSubmitInfo const commandInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = commandBuffers.back(),
                .deviceMask = 0
            };
            VkSubmitInfo2 const submitInfo = {
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
                return {ErrorTag::FATAL_ERROR, "Failed to submit to queue while creating texture image"};
            }
            if(vkQueueWaitIdle(graphicsQueues[0]) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to wait for queue while creating texture image"};
            }
            if(vkResetCommandBuffer(commandBuffers.back(), 0) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to reset command buffer while creating texture image"};
            }

            transferBuffer.destroy(device);
            return textures.append(textureImage.handle, textureImage.memory, textureImage.view);
        }

        Error<noreturn> recreateSwapchain() noexcept {
            vkDeviceWaitIdle(device);

            Error<VkPresentModeKHR> presentMode = choosePresentMode(physicalDevice, surface); 
            if(!presentMode) return presentMode.moveError();

            VkSurfaceCapabilitiesKHR surfaceCapabilities{};
            if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface capabilities while recreating the swapchain"};
            }

            Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(physicalDevice, surface);
            if(!surfaceFormat) return surfaceFormat.moveError();

            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceCapabilities);
            if(!imageExtent) return imageExtent.moveError();
            swapchainImageExtent = imageExtent;

            VkSwapchainKHR newSwapchain;
            VkSwapchainCreateInfoKHR const swapchainInfo = {
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
                return {ErrorTag::FATAL_ERROR, "Failed to recreate swapchain"};
            }
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            swapchain = newSwapchain;

            for(VkImageView imageView : swapchainImageViews) vkDestroyImageView(device, imageView, nullptr);
            swapchainImageViews.clear();
            swapchainImages.clear();

            TRY(getSwapchainImages(surfaceFormat.data().format));

            return success();
        }

        void doRenderPass(std::uint32_t imageIndex, VkCommandBuffer commandBuffer, Objects const& objects) noexcept {
            VkRenderingAttachmentInfo renderAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = renderTargets[imageIndex].view,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT,
                .resolveImageView = swapchainImageViews[imageIndex],
                .resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {.float32 = {1.0f, 1.0f, 1.0f, 1.0f}}}
            };
            if(msaaSampleCount == VK_SAMPLE_COUNT_1_BIT) {
                renderAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
            }
            VkRenderingAttachmentInfo const depthAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = depthImage.view,
                .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = nullptr,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .clearValue = {.depthStencil = {.depth = 1.0f, .stencil = 0U}}
            };
            VkRenderingInfo const renderingInfo = {
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
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            VkViewport const viewport = {
                .x = 0.0f, .y = 0.0f, 
                .width = static_cast<float>(swapchainImageExtent.width), 
                .height = static_cast<float>(swapchainImageExtent.height), 
                .minDepth = 0.0f, .maxDepth = 1.0f
            };
            vkCmdSetViewportWithCount(commandBuffer, 1, &viewport);
            VkRect2D const scissor = {
                .offset = {0, 0},
                .extent = swapchainImageExtent 
            };
            vkCmdSetScissorWithCount(commandBuffer, 1, &scissor);

            VkDescriptorBufferInfo const cameraBufferInfo = {
                .buffer = cameraMatrices.buffer.handle,
                .offset = sizeof(glm::mat4) * currentFrame,
                .range = sizeof(glm::mat4)
            };
            VkWriteDescriptorSet const writeCamera = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = nullptr,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &cameraBufferInfo,
                .pTexelBufferView = nullptr
            };
            vkCmdPushDescriptorSet(
                commandBuffer, 
                VK_PIPELINE_BIND_POINT_GRAPHICS, 
                graphicsPipelineLayout, 
                0, 
                1, 
                &writeCamera
            );

            HeapArray<VkDeviceSize> offsets(vertexBuffers.size(), 0);
            if(vertexBuffers.size() != 0) {
                vkCmdBindVertexBuffers(commandBuffer, 0, static_cast<std::uint32_t>(vertexBuffers.size()), vertexBuffers.handles, offsets.data());
            }

            for(std::size_t i = 0; i < objects.size; i++) {
                VkDescriptorImageInfo const materialInfo = {
                    .sampler = sampler,
                    .imageView = textures.at(textures.views, objects.textureIDs[i]),
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                };
                VkWriteDescriptorSet const writeMaterial = {
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
                    graphicsPipelineLayout, 
                    0, 
                    1, 
                    &writeMaterial
                );

                vkCmdPushConstants(commandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &objects.modelMatrices[i]);

                std::size_t const meshIndex = vertexBuffers.getIndex(objects.meshIDs[i]);
                vkCmdBindIndexBuffer(commandBuffer, vertexBuffers.handles[meshIndex], vertexBuffers.sizes[meshIndex].vertices, VK_INDEX_TYPE_UINT32);
                std::uint32_t const numIndices = vertexBuffers.sizes[meshIndex].indices / sizeof(std::uint32_t);
                vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, meshIndex, 0);
            }

            vkCmdEndRendering(commandBuffer);
        }        

        Error<noreturn> createSyncObjects() noexcept {
            semaphores.resize(swapchainImages.size() + MAX_CONCURRENT_FRAMES);
            VkSemaphoreCreateInfo const semaphoreInfo = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0
            };
            for(VkSemaphore& semaphore : semaphores) {
                if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to create semaphores"};
                }
            }
            VkFenceCreateInfo const fenceInfo = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            };
            for(VkFence& fence : fences) {
                if(vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to create fences!"};
                }
            }
            return success();
        }

        Error<VkShaderModule> createShaderModule() noexcept {
            std::ifstream shaderFile("slang.spv", std::ios::binary | std::ios::ate);
            if(!shaderFile.is_open()) {
                return {ErrorTag::FATAL_ERROR, "Failed to find shader code!"};
            }
            std::vector<char> code(shaderFile.tellg());
            shaderFile.seekg(0, std::ios::beg);
            shaderFile.read(code.data(), static_cast<std::streamsize>(code.size()));
            shaderFile.close();

            VkShaderModuleCreateInfo const shaderModuleInfo = {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,                                                     
                .codeSize = code.size(),                                  
                .pCode = reinterpret_cast<std::uint32_t*>(code.data())
            };
            VkShaderModule result;
            if(vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &result) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create shader module!"};
            }
            return result;
        }

        Error<noreturn> createGraphicsPipeline() noexcept {
            Error<VkShaderModule> shaderModule = createShaderModule();
            if(!shaderModule) return shaderModule.moveError();

            std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos;
            shaderStageInfos[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr, 
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shaderModule,
                .pName = "vertMain",
                .pSpecializationInfo = nullptr
            };
            shaderStageInfos[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr, 
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = shaderModule,
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
                .rasterizationSamples = msaaSampleCount,
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
                .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
            };

            std::array<VkFormat, 1> const colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB};
            VkPipelineRenderingCreateInfo const pipelineRenderingInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                .pNext = nullptr,
                .viewMask = 0,
                .colorAttachmentCount = colorAttachmentFormats.max_size(),
                .pColorAttachmentFormats = colorAttachmentFormats.data(),
                .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
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

            VkPushConstantRange const pushConstantRange = {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(glm::mat4)
            };
            VkPipelineLayoutCreateInfo const pipelineLayoutInfo = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = 1,
                .pSetLayouts = &descriptorSetLayout,
                .pushConstantRangeCount = 1,
                .pPushConstantRanges = &pushConstantRange
            };
            if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create graphics pipeline layout!"};
            }
           
            VkGraphicsPipelineCreateInfo const pipelineInfo = {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .pNext = &pipelineRenderingInfo,
                .flags = 0,
                .stageCount = shaderStageInfos.max_size(),
                .pStages = shaderStageInfos.data(),
                .pVertexInputState = &vertexInputStateInfo,
                .pInputAssemblyState = &inputAssemblyInfo,
                .pTessellationState = nullptr,
                .pViewportState = nullptr,
                .pRasterizationState = &rasterizationStateInfo,
                .pMultisampleState = &multisampleStateInfo,
                .pDepthStencilState = &depthStencilStateInfo,
                .pColorBlendState = &colorBlendStateInfo,
                .pDynamicState = &dynamicStateInfo,
                .layout = graphicsPipelineLayout, 
                .renderPass = nullptr,
                .subpass = 0,
                .basePipelineHandle = nullptr,
                .basePipelineIndex = 0
            };

            if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create graphics pipeline!"};
            }

            vkDestroyShaderModule(device, shaderModule, nullptr);
            return success();
        }
        Error<noreturn> getSwapchainImages(VkFormat format) noexcept {
            std::uint32_t imageCount;
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get swapchain images!"};
            }
            swapchainImages.resize(imageCount);
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get swapchain images!"};
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
            for(std::uint32_t i = 0; i < imageCount; i++) {
                imageViewInfo.image = swapchainImages[i];
                if(vkCreateImageView(device, &imageViewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, std::format("Failed to create swapchain image view {}!", i)};
                }
            }
            return success();
        }
        Error<noreturn> createCommandBuffers() noexcept {
            VkCommandPoolCreateInfo const poolInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = graphicsQueueFamilyIndex
            };
            if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create VkCommandPool!"};
            }
            VkCommandBufferAllocateInfo const allocInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .pNext = nullptr,
                .commandPool = commandPool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = static_cast<std::uint32_t>(commandBuffers.max_size())
            };
            if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to allocate command vertexBuffers!"};
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
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            return VkExtent2D{
                .width = std::clamp<std::uint32_t>(width, 
                    capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                .height = std::clamp<std::uint32_t>(height, 
                    capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
            };
        }


        Error<noreturn> createSwapchain(SurfaceInfo const& surfaceInfo) noexcept {
            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceInfo.capabilities);
            if(!imageExtent) return imageExtent.moveError();
            swapchainImageExtent = imageExtent;

            VkSwapchainCreateInfoKHR const swapchainInfo = {
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
                return {ErrorTag::FATAL_ERROR, "Failed to create VkSwapchainKHR!"};
            }

            return success();
        }


        static Error<PickQueueFamilyIndexResult> pickQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
            PickQueueFamilyIndexResult result{};
            //Get queue family properties for the current physical device
            std::uint32_t queueFamilyPropertyCount = 0;
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
            for(std::uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
                std::uint32_t const currentQueueCount = queueFamilyProperties[i].queueFamilyProperties.queueCount;
                if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & 
                        (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
                    if(currentQueueCount >= result.graphicsCount) {
                        result.graphicsIndex = i;
                        result.graphicsCount = currentQueueCount;
                    }
                }
                VkBool32 surfaceSupport;
                if(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupport) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface support!"};
                }
                if(surfaceSupport == VK_TRUE and currentQueueCount >= result.presentCount) {
                    result.presentIndex = i;
                    result.presentCount = currentQueueCount;
                }
            }
            if(result.presentIndex == std::numeric_limits<std::uint32_t>::max() 
                    or result.graphicsIndex == std::numeric_limits<std::uint32_t>::max()) {
                return {ErrorTag::SEARCH_FAIL, "Physical Device did not have queue family with needed properties!"};
            }
            return result;
        }

        static Error<VkPresentModeKHR> choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept {
            VkPresentModeKHR presentMode{};
            //Get present modes for the surface supported by the physical device
            std::uint32_t presentModeCount = 0;
            if(vkGetPhysicalDeviceSurfacePresentModesKHR(
                    physicalDevice, 
                    surface, 
                    &presentModeCount, 
                    nullptr
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface present modes!"};
            }
            HeapArray<VkPresentModeKHR> presentModes(presentModeCount);
            if(vkGetPhysicalDeviceSurfacePresentModesKHR(
                    physicalDevice, 
                    surface, 
                    &presentModeCount, 
                    presentModes.data()
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface present modes!"};
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
            std::uint32_t surfaceFormatCount = 0;
            if(vkGetPhysicalDeviceSurfaceFormatsKHR(
                    physicalDevice, surface, &surfaceFormatCount, nullptr
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface formats!"};
            }
            HeapArray<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            if(vkGetPhysicalDeviceSurfaceFormatsKHR(
                    physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface formats!"};
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
            std::uint32_t deviceExtensionPropertyCount = 0;
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, 
                    &deviceExtensionPropertyCount, nullptr
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical device extension properties!"};
            }
            HeapArray<VkExtensionProperties> extensionProperties(deviceExtensionPropertyCount);
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, 
                &deviceExtensionPropertyCount, extensionProperties.data()
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical device extension properties!"};
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
            return {ErrorTag::SEARCH_FAIL, "Physical Device did not support needed extensions!"};
        }



        static Error<PickPhysicalDeviceResult> pickPhysicalDevice(std::vector<VkPhysicalDevice>const& physicalDevices, VkSurfaceKHR surface) noexcept {
            PickPhysicalDeviceResult result{};
            for(VkPhysicalDevice const& currentPhysicalDevice : physicalDevices) {
                //Check device extension support for the current physical device
                switch(Error<noreturn> procResult = checkDeviceExtensionSupport(currentPhysicalDevice); procResult.tag()) {
                    case ErrorTag::SEARCH_FAIL: goto Continue_Search;
                    case ErrorTag::FATAL_ERROR: return procResult.moveError<PickPhysicalDeviceResult>();
                    case ErrorTag::ALL_OKAY: break;
                }

                //Check physical device's support for needed features
                if(!checkDeviceFeatureSupport(currentPhysicalDevice)) goto Continue_Search;

                //Get physical device's capabilities with the surface
                if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                        currentPhysicalDevice, 
                        surface, 
                        &result.surfaceInfo.capabilities
                    ) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface capabilities!"};
                }

                //Get a surface format to use
                if(Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(currentPhysicalDevice, surface)) {
                    result.surfaceInfo.format = surfaceFormat;
                }
                else return surfaceFormat.moveError<PickPhysicalDeviceResult>();

                //Choose a present mode to use
                switch(Error<VkPresentModeKHR> presentMode = choosePresentMode(currentPhysicalDevice, surface); presentMode.tag()) {
                    case ErrorTag::SEARCH_FAIL: goto Continue_Search;
                    case ErrorTag::FATAL_ERROR: return presentMode.moveError<PickPhysicalDeviceResult>();
                    case ErrorTag::ALL_OKAY: result.surfaceInfo.presentMode = presentMode;
                }

                //Pick the desired queue family index
                switch(Error<PickQueueFamilyIndexResult> queueFamilyInfo = pickQueueFamilyIndex(currentPhysicalDevice, surface); queueFamilyInfo.tag()) {
                    case ErrorTag::SEARCH_FAIL: goto Continue_Search;
                    case ErrorTag::FATAL_ERROR: return queueFamilyInfo.moveError<PickPhysicalDeviceResult>();
                    case ErrorTag::ALL_OKAY: result.queueFamilyInfo = queueFamilyInfo;
                }

                //At this point if all has succeeded, we are ready to use the current physical device and
                // create the logical device
                result.physicalDevice = currentPhysicalDevice;
                return result;

                //This label simply causes the loop to continue. I'm doing this because I don't
                // like using the `continue` keyword in switch statements due to `break` having
                // different behavior within them.
                Continue_Search:
            }
            return {ErrorTag::SEARCH_FAIL, "Failed to find suitable physical device"};
        }

        Error<noreturn> createDevice(SurfaceInfo& surfaceInfo) noexcept {
            std::uint32_t graphicsQueueCount = 0;
            std::uint32_t presentQueueCount = 0;

            //Get all the physical devices installed on the system
            std::uint32_t physicalDeviceCount = 0;
            if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical devices!"};
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical devices!"};
            }

            //Pick a physical device to use
            if(Error<PickPhysicalDeviceResult> const pickResult = 
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
            //pickPhysicalDevice just returns SEARCH_FAIL if none of the devices are suitable, but 
            // I want this case to be fatal instead
            else return {ErrorTag::FATAL_ERROR, pickResult.message()};

            VkPhysicalDeviceProperties2 props = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = nullptr
            };
            vkGetPhysicalDeviceProperties2(physicalDevice, &props);
            VkSampleCountFlags const availableSampleCounts = props.properties.limits.framebufferColorSampleCounts;
            if(availableSampleCounts & VK_SAMPLE_COUNT_64_BIT) msaaSampleCount = VK_SAMPLE_COUNT_64_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_32_BIT) msaaSampleCount = VK_SAMPLE_COUNT_32_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_16_BIT) msaaSampleCount = VK_SAMPLE_COUNT_16_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_8_BIT) msaaSampleCount = VK_SAMPLE_COUNT_8_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_4_BIT) msaaSampleCount = VK_SAMPLE_COUNT_4_BIT;
            else if(availableSampleCounts & VK_SAMPLE_COUNT_2_BIT) msaaSampleCount = VK_SAMPLE_COUNT_2_BIT;
            else msaaSampleCount = VK_SAMPLE_COUNT_1_BIT;

            bool const differentQueueFamilies = graphicsQueueFamilyIndex != presentQueueFamilyIndex;
            std::array<VkDeviceQueueCreateInfo, 2> queueCreateInfos;
            std::uint32_t queueCreateInfoCount = 1U;
            HeapArray<float> queuePriorities;

            if(differentQueueFamilies) {
                queueCreateInfoCount = 2U;
                graphicsQueueCount = std::min(graphicsQueueCount, MAX_CONCURRENT_FRAMES);
                presentQueueCount = std::min(presentQueueCount, MAX_CONCURRENT_FRAMES);
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
                graphicsQueueCount = std::min(graphicsQueueCount, 2U * MAX_CONCURRENT_FRAMES);
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

            VkDeviceCreateInfo const deviceInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = &features2,
                .flags = 0,
                .queueCreateInfoCount = queueCreateInfoCount,
                .pQueueCreateInfos = queueCreateInfos.data(),
                .enabledExtensionCount = static_cast<std::uint32_t>(neededDeviceExtensions.size()),
                .ppEnabledExtensionNames = neededDeviceExtensions.data(),
                .pEnabledFeatures = nullptr
            };

            if(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create VkDevice!"};
            }

            //Acquire handles to all the GPU queues we just created
            if(differentQueueFamilies) {
                queues.resize(graphicsQueueCount + presentQueueCount);
                for(std::uint32_t i = 0; i < graphicsQueueCount; i++) {
                    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, i, &queues[i]);
                }
                for(std::uint32_t i = 0; i < presentQueueCount; i++) {
                    vkGetDeviceQueue(device, presentQueueFamilyIndex, i, &queues[graphicsQueueCount + i]);
                }
            }
            else {
                queues.resize(graphicsQueueCount);
                for(std::uint32_t i = 0; i < graphicsQueueCount; i++) {
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
                return {ErrorTag::FATAL_ERROR, "Failed to find function vkCreateDebugUtilsMessengerEXT!"};
            }
            vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            if(vkDestroyDebugUtilsMessengerEXT == nullptr) {
                return {ErrorTag::FATAL_ERROR, "Failed to find function vkDestroyDebugUtilsMessengerEXT!"};
            }
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

            if(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create vkDebugUtilsMessengerEXT!"};
            }
            return success();
        }

        Error<noreturn> createInstance(const std::string& appName) noexcept {
            if constexpr(enableValidationLayers) {
                std::uint32_t layerPropertyCount = 0;
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to enumerate instance layer properties!"};
                }
                std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data()) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to enumerate instance layer properties!"};
                }
                for(char const* layer : validationLayers) {
                    for(VkLayerProperties const& property : layerProperties) {
                        if(std::strcmp(layer, property.layerName) == 0) {
                            goto Next_Layer;
                        }
                    }
                    return {ErrorTag::FATAL_ERROR, std::format("Needed layer \"{}\" not found", layer)};
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
                .enabledExtensionCount = static_cast<std::uint32_t>(extNames.size()),
                .ppEnabledExtensionNames = extNames.data()
            };
            if constexpr(enableValidationLayers) {
                instanceInfo.enabledLayerCount = validationLayers.size();
                instanceInfo.ppEnabledLayerNames = validationLayers.data();
            }

            if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create VkInstance!"};
            }
            return success();
        }

        Error<noreturn> createSurface(std::string const& name) noexcept {
            int numDisplays;
            SDL_DisplayID* displays = SDL_GetDisplays(&numDisplays);
            if(displays == nullptr) {
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            //Be lazy and just use the first display
            SDL_Rect displayBounds{};
            if(!SDL_GetDisplayBounds(displays[0], &displayBounds)) {
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            //SDL_WINDOW_MOUSE_GRABBED : mouse cannot escape window bounds - allows using relative
            // mouse mode
            //SDL_WINDOW_HIDDEN : hide the window before we're ready to display images to it
            window = SDL_CreateWindow(name.c_str(), displayBounds.w, displayBounds.h, 
                SDL_WINDOW_VULKAN | SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIDDEN);
            //Instead of tracking live mouse inputs and having on-screen cursor, just track
            // changes in mouse position
            if(!SDL_SetWindowRelativeMouseMode(window, true)) {
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            SDL_free(displays);
            return success();
        }
        public:
        Error<noreturn> init(std::string const& name) noexcept {
            if(Error<noreturn> res = createInstance(name); !res) {
                flags |= flagBits::instanceInvalid;
                return res;
            }
            if constexpr(enableValidationLayers) TRY(createDebugUtilsMessenger());
            TRY(createSurface(name));
            SurfaceInfo surfaceInfo{};
            if(Error<noreturn> res = createDevice(surfaceInfo); !res) {
                flags |= flagBits::deviceInvalid;
                return res;
            }
            TRY(createSwapchain(surfaceInfo));
            TRY(getSwapchainImages(surfaceInfo.format.format));
            TRY(createCommandBuffers());
            TRY(createSampler());
            TRY(createRenderTargets(surfaceInfo.format.format));
            TRY(createDepthImage());
            TRY(createSyncObjects());
            TRY(createDescriptorSetLayouts());
            TRY(createGraphicsPipeline());
            if(!SDL_ShowWindow(window)) {
                return {ErrorTag::FATAL_ERROR, "Failed to show window"};
            }
            currentFrame = 0;
            flags = {};
            if(Error<UniformBuffer<glm::mat4>> res = UniformBuffer<glm::mat4>::make(
                    device, physicalDevice, sizeof(glm::mat4) * MAX_CONCURRENT_FRAMES); 
                    !res) {
                return res.moveError();
            }
            else cameraMatrices = res.moveData(); 
            return success();
        }
        //Destructor
        ~Renderer() noexcept {
            if((flags & flagBits::deviceInvalid) == 0) [[likely]] {
                vkDeviceWaitIdle(device);
                vkDestroySwapchainKHR(device, swapchain, nullptr);
                for(std::size_t i = 0; i < vertexBuffers.size(); i++) {
                    vkDestroyBuffer(device, vertexBuffers.handles[i], nullptr);
                    vkFreeMemory(device, vertexBuffers.memories[i], nullptr);
                }
                for(std::size_t i = 0; i < textures.size(); i++) {
                    vkDestroyImage(device, textures.handles[i], nullptr);
                    vkFreeMemory(device, textures.memories[i], nullptr);
                    vkDestroyImageView(device, textures.views[i], nullptr);
                }
                for(VkImageView view : swapchainImageViews) {
                    vkDestroyImageView(device, view, nullptr);
                }
                vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                for(VkSemaphore semaphore : semaphores) {
                    vkDestroySemaphore(device, semaphore, nullptr);
                }
                for(VkFence fence : fences) {
                    vkDestroyFence(device, fence, nullptr);
                }
                depthImage.destroy(device);
                for(GPUImage& target: renderTargets) target.destroy(device);
                vkDestroySampler(device, sampler, nullptr);
                vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
                vkDestroyCommandPool(device, commandPool, nullptr);
                vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
                vkDestroyPipeline(device, graphicsPipeline, nullptr);
                cameraMatrices.destroy(device);
                vkDestroyDevice(device, nullptr);
            }
            if((flags & flagBits::instanceInvalid) == 0) [[likely]] {
                SDL_Vulkan_DestroySurface(instance, surface, nullptr);
                SDL_DestroyWindow(window);
                if constexpr(enableValidationLayers) {
                    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
                }
                vkDestroyInstance(instance, nullptr);
            }
        }

        Error<noreturn> drawFrame(std::chrono::nanoseconds deltaTime, Objects& objects) noexcept {
            if(vkWaitForFences(
                    device, 
                    1, 
                    &fences[currentFrame], 
                    VK_TRUE, 
                    std::numeric_limits<std::uint64_t>::max()
                ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, std::format("Something went from while waiting on fence {}", currentFrame)};
            }
            //These semaphores indicate that we have successfully acquired an image on this frame
            Slice<VkSemaphore> imageAcquiredSemaphores = Slice<VkSemaphore>::make(semaphores, 0, MAX_CONCURRENT_FRAMES);
            //These semaphores indicate that the image acquired is ready to present
            Slice<VkSemaphore> presentReadySemaphores = Slice<VkSemaphore>::make(semaphores, MAX_CONCURRENT_FRAMES);

            std::uint32_t imageIndex;
            VkResult res = vkAcquireNextImageKHR(
                device, 
                swapchain, 
                std::numeric_limits<std::uint64_t>::max(), 
                imageAcquiredSemaphores[currentFrame], 
                nullptr, 
                &imageIndex
            );
            if(res == VK_ERROR_OUT_OF_DATE_KHR or res == VK_SUBOPTIMAL_KHR or (flags & flagBits::recreateSwapchain)) {
                TRY(recreateSwapchain());
                vkDestroySemaphore(device, imageAcquiredSemaphores[currentFrame], nullptr);
                VkSemaphoreCreateInfo const semaphoreInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0
                };
                if(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAcquiredSemaphores[currentFrame]) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to recreate semaphore while recreating swapchain during draw"};
                }
                depthImage.destroy(device);
                TRY(createDepthImage());
                flags &= ~flagBits::recreateSwapchain;
                return drawFrame(deltaTime, objects);
            }
            else if(res != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to acquire next swapchain image index"};
            }

            if(vkResetFences(device, 1, &fences[currentFrame]) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, std::format("Failed to reset fence {}", currentFrame)};
            }

            if(vkResetCommandBuffer(commandBuffers[currentFrame], 0) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to reset command buffer"};
            }
            VkCommandBufferBeginInfo const beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, std::format("Failed to begin command buffer {}", currentFrame)};
            }

            std::array<VkImageMemoryBarrier2, 2> imageMemoryBarriers;
            //Transition image layout for color writing
            imageMemoryBarriers[0] = {
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
                .image = renderTargets[imageIndex].handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            //Transition image layout for depth buffering
            imageMemoryBarriers[1] = {
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
                .image = depthImage.handle,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            VkDependencyInfo const colorWriteDependency = {
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .pNext = nullptr,
                .dependencyFlags = 0,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 0,
                .pBufferMemoryBarriers = 0,
                .imageMemoryBarrierCount = imageMemoryBarriers.max_size(),
                .pImageMemoryBarriers = imageMemoryBarriers.data()
            };

            vkCmdPipelineBarrier2(commandBuffers[currentFrame], &colorWriteDependency);

            doRenderPass(imageIndex, commandBuffers[currentFrame], objects);

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
            vkCmdPipelineBarrier2(commandBuffers[currentFrame], &presentDependency);

            if(vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, std::format("Failed to end command buffer {}", currentFrame)};
            }

            VkSemaphoreSubmitInfo const imageAcquisition = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .pNext = nullptr,
                .semaphore = imageAcquiredSemaphores[currentFrame],
                .value = 0,
                .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .deviceIndex = 0
            };
            VkCommandBufferSubmitInfo const commandBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .pNext = nullptr,
                .commandBuffer = commandBuffers[currentFrame],
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
            VkSubmitInfo2 const submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = 1,
                .pWaitSemaphoreInfos = &imageAcquisition,
                .commandBufferInfoCount = 1,
                .pCommandBufferInfos = &commandBufferInfo,
                .signalSemaphoreInfoCount = 1,
                .pSignalSemaphoreInfos = &presentationReady
            };

            std::uint32_t const graphicsQueueIndex = currentFrame % graphicsQueues.size();
            if(vkQueueSubmit2(graphicsQueues[graphicsQueueIndex], 1, &submitInfo, fences[currentFrame]) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to submit draw commands to queue"};
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
            std::uint32_t const presentQueueIndex = currentFrame % presentQueues.size();
            if(vkQueuePresentKHR(presentQueues[presentQueueIndex], &presentInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to present graphics queue"};
            }

            currentFrame = (currentFrame + 1) % MAX_CONCURRENT_FRAMES;

            return success();
        }
    };
}
