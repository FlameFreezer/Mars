module;

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

export module mars:renderer;
import gpubuffer;
import error;
import heap_array;
import flag_bits;
import vkhelper;

#define INIT_TRY(proc) do {\
    internal::procResult = proc;\
    if(!internal::procResult.okay()) {\
        flags |= flagBits::failedInitialization;\
        return internal::procResult;\
    } \
} while(false)

#define TRY(proc) do {\
    internal::procResult = proc;\
    if(!internal::procResult.okay()) return internal::procResult;\
} while(false)

namespace mars {
    namespace internal {
        Error<noreturn> procResult;
    }
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

    constexpr std::array<Vertex, 4> vertices = {
        Vertex{glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)},
        Vertex{glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)}
    };
    constexpr std::array<std::uint32_t, 6> indices = {
        0, 1, 2, 0, 2, 3
    };

    export class Renderer {
        friend class Game;
    	HeapArray<VkImage> swapchainImages;
    	HeapArray<VkImageView> swapchainImageViews;	
        HeapArray<VkQueue> queues;
        HeapArray<VkSemaphore> semaphores;
    	std::array<VkCommandBuffer, maxConcurrentFrames + 1> commandBuffers;
        std::array<VkFence, maxConcurrentFrames> fences;
    	GPUBuffer vertexBuffer;
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
        std::uint32_t currentFrame;
        RendererFlags flags;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;

        Error<noreturn> createTexture() noexcept {
            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = nullptr;
            std::string texturePath(MARS_TEXTURE_PATH);
            texturePath.append("/texture.jpg");
            pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if(pixels == nullptr) {
                return {ErrorTag::FATAL_ERROR, "Failed to find/load texture file"};
            }
            VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha; 

            Error<GPUBuffer> transferBuffer = GPUBuffer::make(
                device, 
                physicalDevice, 
                imageSize, 
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            if(!transferBuffer.okay()) return transferBuffer.moveError<noreturn>();

            void* memory;
            if(vkMapMemory(device, transferBuffer.data().memory, 0, imageSize, 0, &memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map buffer memory to the host"};
            }
            std::memcpy(memory, pixels, imageSize);
            vkUnmapMemory(device, transferBuffer.data().memory);

            stbi_image_free(pixels);

            VkImageCreateInfo const imageInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .extent = {.width = static_cast<std::uint32_t>(texWidth), .height = static_cast<std::uint32_t>(texHeight), .depth = 1},
                .mipLevels = 1, 
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            };
            if(vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create texture image"};
            }
            VkMemoryRequirements memRequirements = {};
            vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
            Error<std::uint32_t> memType = findPhysicalDeviceMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if(!memType.okay()) return memType.moveError<noreturn>();
            VkMemoryAllocateInfo const allocInfo = {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .pNext = nullptr,
                .allocationSize = memRequirements.size,
                .memoryTypeIndex = memType.data()
            };
            if(vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to allocate texture image memory"};
            }
            if(vkBindImageMemory(device, textureImage, textureImageMemory, 0) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to bind texture image memory"};
            }

            VkCommandBufferBeginInfo const beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to begin command buffer while creating texture image"};
            }

            VkImageMemoryBarrier2 const preBufferCopy = {
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
                .image = textureImage,
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
                .pImageMemoryBarriers = &preBufferCopy
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
                .srcBuffer = transferBuffer.data().handle,
                .dstImage = textureImage,
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
                .image = textureImage,
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
            VkSubmitInfo2 const submitInfo = {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                .pNext = nullptr,
                .flags = 0,
                .waitSemaphoreInfoCount = 0,
                .pWaitSemaphoreInfos = nullptr,
                .signalSemaphoreInfoCount = 0,
                .pSignalSemaphoreInfos = nullptr
            };
            if(vkQueueSubmit2(queues[0], 1, &submitInfo, nullptr) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to submit to queue while creating texture image"};
            }

            VkImageViewCreateInfo const textureViewInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = textureImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_SRGB,
                .components = {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            if(vkCreateImageView(device, &textureViewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create texture image view"};
            }

            transferBuffer.data().destroy(device);
            return success();
        }

        Error<noreturn> recreateSwapchain() noexcept {
            vkDeviceWaitIdle(device);
            SurfaceInfo surfaceInfo{};

            Error<VkPresentModeKHR> presentMode = choosePresentMode(physicalDevice); 
            if(!presentMode.okay()) return presentMode.moveError<noreturn>();
            surfaceInfo.presentMode = presentMode.data();

            if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceInfo.capabilities) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface capabilities while recreating the swapchain"};
            }

            Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(physicalDevice);
            if(!surfaceFormat.okay()) return surfaceFormat.moveError<noreturn>();
            surfaceInfo.format = surfaceFormat.data();

            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceInfo.capabilities);
            if(!imageExtent.okay()) return imageExtent.moveError<noreturn>();
            swapchainImageExtent = imageExtent.data();

            VkSwapchainKHR newSwapchain;
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

            internal::procResult = getSwapchainImages(surfaceInfo);
            if(!internal::procResult.okay()) return internal::procResult;

            return success();
        }

        void renderPass(VkImageView imageView, VkCommandBuffer commandBuffer) noexcept {
            VkRenderingAttachmentInfo const colorAttachment = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .pNext = nullptr,
                .imageView = imageView,
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode = VK_RESOLVE_MODE_NONE,
                .resolveImageView = nullptr,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
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
                .pColorAttachments = &colorAttachment,
                .pDepthAttachment = nullptr,
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

            VkDeviceSize offset = 0ULL;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.handle, &offset);
            vkCmdBindIndexBuffer(commandBuffer, vertexBuffer.handle, vertices.max_size() * sizeof(Vertex), VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indices.max_size(), 1, 0, 0, 0);

            vkCmdEndRendering(commandBuffer);

        }        

        Error<noreturn> createSyncObjects() noexcept {
            semaphores = HeapArray<VkSemaphore>(swapchainImages.size() + maxConcurrentFrames);
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

        Error<noreturn> createVertexBuffer() noexcept {
            VkCommandBufferBeginInfo const beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers.back(), &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to begin transfer command buffer!"};
            }
            Error<GPUBuffer> buff = GPUBuffer::make(
                device, 
                physicalDevice,
                vertices.max_size() * sizeof(Vertex) + indices.max_size() * sizeof(std::uint32_t),
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            if(!buff.okay()) return buff.moveError<noreturn>();
            vertexBuffer = buff.moveData();

            buff = GPUBuffer::make(
                device,
                physicalDevice,
                vertices.max_size() * sizeof(Vertex) + indices.max_size() * sizeof(std::uint32_t),
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            if(!buff.okay()) return buff.moveError<noreturn>();
            GPUBuffer transferBuffer(buff.moveData());

            void* memory = nullptr;
            if(vkMapMemory(device, transferBuffer.memory, 0, vertices.max_size() * sizeof(Vertex), 0, &memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map memory while creating transfer buffer"};
            }
            std::memcpy(memory, static_cast<void const*>(vertices.data()), vertices.max_size() * sizeof(Vertex));
            vkUnmapMemory(device, transferBuffer.memory);
            if(vkMapMemory(device, transferBuffer.memory, vertices.max_size() * sizeof(Vertex), indices.max_size() * sizeof(std::uint32_t), 0, &memory) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to map memory while creating transfer buffer"};
            }
            std::memcpy(memory, static_cast<void const*>(indices.data()), indices.max_size() * sizeof(std::uint32_t));
            vkUnmapMemory(device, transferBuffer.memory);

            VkBufferCopy const region = {
                .srcOffset = 0, .dstOffset = 0, .size = vertices.max_size() * sizeof(Vertex) + indices.max_size() * sizeof(std::uint32_t)
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
            if(vkQueueSubmit2(queues[0], 1, &submitInfo, nullptr) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to submit to transfer queue while creating vertex buffer"};
            }
            vkQueueWaitIdle(queues[0]);
            vkResetCommandBuffer(commandBuffers.back(), 0);
            transferBuffer.destroy(device);
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
            std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos;
            Error<VkShaderModule> shaderModule = createShaderModule();
            if(!shaderModule.okay()) return shaderModule.moveError<noreturn>();

            shaderStageInfos[0] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr, 
                .flags = 0,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shaderModule.data(),
                .pName = "vertMain",
                .pSpecializationInfo = nullptr
            };
            shaderStageInfos[1] = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext = nullptr, 
                .flags = 0,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = shaderModule.data(),
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

            if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create graphics pipeline layout!"};
            }
           
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
                .layout = graphicsPipelineLayout, 
                .renderPass = nullptr,
                .subpass = 0,
                .basePipelineHandle = nullptr,
                .basePipelineIndex = 0
            };

            if(vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create graphics pipeline!"};
            }

            vkDestroyShaderModule(device, shaderModule.data(), nullptr);
            return success();
        }
        Error<noreturn> getSwapchainImages(SurfaceInfo const& surfaceInfo) noexcept {
            std::uint32_t imageCount;
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get swapchain images!"};
            }
            swapchainImages = HeapArray<VkImage>(imageCount, nullptr);
            if(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get swapchain images!"};
            }
            swapchainImageViews = HeapArray<VkImageView>(imageCount, nullptr);

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
                    return {ErrorTag::FATAL_ERROR, std::format("Failed to create swapchain image view {}!", i)};
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
                return {ErrorTag::FATAL_ERROR, "Failed to allocate command buffers!"};
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

        Error<noreturn> createVkSwapchainKHR(SurfaceInfo const& surfaceInfo) noexcept {
            Error<VkExtent2D> imageExtent = chooseImageExtent(surfaceInfo.capabilities);
            if(!imageExtent.okay()){
                return imageExtent.moveError<noreturn>();
            }
            swapchainImageExtent = imageExtent.data();
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
                        return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface support!"};
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
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface present modes!"};
            }
            std::vector<VkPresentModeKHR> presentModes(presentModeCount);
            if(vkGetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice, 
                surface, 
                &presentModeCount, 
                presentModes.data()
            ) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface present modes!"};
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
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface formats!"};
            }
            std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
            if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface formats!"};
            }
            for(int i = 0; i < surfaceFormatCount; i++) {
                if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return surfaceFormats[i];
                }
            }
            return surfaceFormats[0];
        }

        Error<noreturn> checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)  {
            //Check that the device supports the extensions needed by the application
            std::uint32_t deviceExtensionPropertyCount = 0;
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, nullptr) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical device extension properties!"};
            }
            std::vector<VkExtensionProperties> extensionProperties(deviceExtensionPropertyCount);
            if(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionPropertyCount, extensionProperties.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical device extension properties!"};
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
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical devices!"};
            }
            std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            if(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to enumerate physical devices!"};
            }

            //Iterate through each of these devices
            for(int i = 0; i < physicalDeviceCount; i++) {
                //Check device extension support for the current physical device
                internal::procResult = checkDeviceExtensionSupport(physicalDevices[i]);
                if(internal::procResult.tag() == ErrorTag::SEARCH_FAIL) continue;
                else if(!internal::procResult.okay()) return internal::procResult;
                //Get physical device's capabilities with the surface
                if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                    physicalDevices[i], 
                    surface, 
                    &surfaceInfo.capabilities
                ) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to get physical device surface capabilities!"};
                }

                //Get a surface format to use
                Error<VkSurfaceFormatKHR> surfaceFormat = checkDeviceSurfaceFormats(physicalDevices[i]);
                if(surfaceFormat.okay()) {
                    surfaceInfo.format = surfaceFormat.data();
                }
                else {
                    return surfaceFormat.moveError<noreturn>();
                }
                //Choose a present mode to use
                Error<VkPresentModeKHR> presentMode = choosePresentMode(physicalDevices[i]);
                if(presentMode.tag() == ErrorTag::SEARCH_FAIL) continue;
                else if(presentMode.okay()) {
                    surfaceInfo.presentMode = presentMode.data();
                }
                else {
                    return presentMode.moveError<noreturn>();
                }
                //Pick the desired queue family index
                internal::procResult = pickQueueFamilyIndex(queueFamilyIndex, queueCount, physicalDevices[i]);
                if(internal::procResult.tag() == ErrorTag::SEARCH_FAIL) continue;
                else if(!internal::procResult.okay()) return internal::procResult;
                //At this point if all has succeeded, we are ready to use the current physical device and
                // create the logical device
                physicalDevice = physicalDevices[i];
                goto Device_Creation;
            } 
            //If we got here, none of the physical devices supported the features we needed
            return {ErrorTag::FATAL_ERROR, "Failed to find a suitable GPU!"};

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
                return {ErrorTag::FATAL_ERROR, "Failed to create VkDevice!"};
            }

            //Acquire handles to all the GPU queues we just created
            queues = HeapArray<VkQueue>(queueCount);
            for(std::uint32_t i = 0; i < queueCount; i++) {
                vkGetDeviceQueue(device, queueFamilyIndex, i, &queues[i]);
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
                return {ErrorTag::FATAL_ERROR, "Failed to find function vkCreateDebugUtilsMessengerEXT!"};
            }
            if(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to create vkDebugUtilsMessengerEXT!"};
            }
            vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                instance, "vkDestroyDebugUtilsMessengerEXT"
            );
            if(vkDestroyDebugUtilsMessengerEXT == nullptr) {
                return {ErrorTag::FATAL_ERROR, "Failed to find function vkDestroyDebugUtilsMessengerEXT!"};
            }
            return success();
        }

        Error<noreturn> createVkInstance(const std::string& appName) noexcept {
            if constexpr(enableValidationLayers) {
                std::uint32_t layerPropertyCount = 0;
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, nullptr) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to enumerate instance layer properties!"};
                }
                std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
                if(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties.data()) != VK_SUCCESS) {
                    return {ErrorTag::FATAL_ERROR, "Failed to enumerate instance layer properties!"};
                }
                std::println("Available Layers:");
                for(char const* layer : validationLayers) {
                    for(VkLayerProperties const& property : layerProperties) {
                        std::println("\t{}", property.layerName);
                        if(std::strcmp(layer, property.layerName) == 0) {
                            goto Next_Layer;
                        }
                    }
                    return {ErrorTag::FATAL_ERROR, "Needed instance layers not found!"};
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
        public:
        Error<noreturn> init(std::string const& name) noexcept {
            INIT_TRY(createVkInstance(name));
            if constexpr(enableValidationLayers) {
                INIT_TRY(createVkDebugUtilsMessenger());
            }
            window = SDL_CreateWindow(name.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
            if(window == nullptr) {
                flags |= flagBits::failedInitialization;
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            if(!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
                flags |= flagBits::failedInitialization;
                return {ErrorTag::FATAL_ERROR, SDL_GetError()};
            }
            SurfaceInfo surfaceInfo{};
            std::uint32_t queueFamilyIndex;
            INIT_TRY(createVkDevice(surfaceInfo, queueFamilyIndex));
            INIT_TRY(createVkSwapchainKHR(surfaceInfo));
            INIT_TRY(getSwapchainImages(surfaceInfo));
            INIT_TRY(createCommandBuffers(queueFamilyIndex));
            INIT_TRY(createGraphicsPipeline());
            INIT_TRY(createVertexBuffer());
            INIT_TRY(createSyncObjects());
            INIT_TRY(createTexture());

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
            graphicsPipeline(nullptr),
            textureImage(nullptr),
            textureImageMemory(nullptr),
            textureImageView(nullptr),
            currentFrame(0),
            flags(0)
        {}
        ~Renderer() noexcept {
            //If something went wrong during initialization, we can't destory vulkan objects, so 
            // we'll quickly end program execution
            if(flags & flagBits::failedInitialization) return;
            vkDeviceWaitIdle(device);
            vkDestroySwapchainKHR(device, swapchain, nullptr);
            for(VkImageView view : swapchainImageViews) {
                vkDestroyImageView(device, view, nullptr);
            }
            for(VkSemaphore semaphore : semaphores) {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
            vkDestroyImage(device, textureImage, nullptr);
            vkFreeMemory(device, textureImageMemory, nullptr);
            vkDestroyImageView(device, textureImageView, nullptr);
            for(VkFence fence : fences) {
                vkDestroyFence(device, fence, nullptr);
            }
            vkFreeCommandBuffers(device, commandPool, commandBuffers.size(), commandBuffers.data());
            vkDestroyCommandPool(device, commandPool, nullptr);
            vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vertexBuffer.destroy(device);
            vkDestroyDevice(device, nullptr);
            SDL_Vulkan_DestroySurface(instance, surface, nullptr);
            SDL_DestroyWindow(window);
            if constexpr(enableValidationLayers) {
                vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }
            vkDestroyInstance(instance, nullptr);
        }

        Error<noreturn> drawFrame() noexcept {
            if(vkWaitForFences(device, 1, &fences[currentFrame], VK_TRUE, std::numeric_limits<std::uint64_t>::max()) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, std::format("Something went from while waiitng on fence {}", currentFrame)};
            }
            //These semaphores indicate that we have successfully acquired an image on this frame
            Slice<VkSemaphore> const imageAcquiredSemaphores(semaphores, 0, maxConcurrentFrames);
            //These semaphores indicate that the image acquired is ready to present
            Slice<VkSemaphore> const presentReadySemaphores(semaphores, maxConcurrentFrames);

            std::uint32_t imageViewIndex;
            VkResult res = vkAcquireNextImageKHR(
                device, 
                swapchain, 
                std::numeric_limits<std::uint64_t>::max(), 
                imageAcquiredSemaphores[currentFrame], 
                nullptr, 
                &imageViewIndex
            );
            if(res == VK_ERROR_OUT_OF_DATE_KHR or res == VK_SUBOPTIMAL_KHR or flags & flagBits::recreateSwapchain) {
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
                flags &= ~flagBits::recreateSwapchain;
                return drawFrame();
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
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr
            };
            if(vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, std::format("Failed to begin command buffer {}", currentFrame)};
            }

            //Transition image layout for color writing
            VkImageMemoryBarrier2 const colorWriteBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = swapchainImages[imageViewIndex],
                .subresourceRange = VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
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
                .imageMemoryBarrierCount = 1,
                .pImageMemoryBarriers = &colorWriteBarrier
            };

            vkCmdPipelineBarrier2(commandBuffers[currentFrame], &colorWriteDependency);

            renderPass(swapchainImageViews[imageViewIndex], commandBuffers[currentFrame]);

            //Transition image layout for presentation
            VkImageMemoryBarrier2 const presentSrcBarrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .pNext = nullptr,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .dstAccessMask = VK_ACCESS_2_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = swapchainImages[imageViewIndex],
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
                .semaphore = presentReadySemaphores[imageViewIndex],
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
            std::uint32_t const queueIndex = queues.size() > 1 ? 1 : 0;

            if(vkQueueSubmit2(queues[queueIndex], 1, &submitInfo, fences[currentFrame]) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to submit draw commands to queue"};
            }

            VkPresentInfoKHR const presentInfo = {
                .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext = nullptr,
                .waitSemaphoreCount = 1,
                .pWaitSemaphores = &presentReadySemaphores[imageViewIndex],
                .swapchainCount = 1,
                .pSwapchains = &swapchain,
                .pImageIndices = &imageViewIndex,
                .pResults = nullptr
            };
            if(vkQueuePresentKHR(queues[queueIndex], &presentInfo) != VK_SUCCESS) {
                return {ErrorTag::FATAL_ERROR, "Failed to present graphics queue"};
            }

            currentFrame = (currentFrame + 1) % maxConcurrentFrames;

            return success();
        }
    };
}