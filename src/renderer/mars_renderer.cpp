#include "mars_renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


namespace mars {
    Error<ID> Renderer::makeTexture(const std::string& texturePath) noexcept {
        int texWidth{0}, texHeight{0}, texChannels{0};
        stbi_uc* pixels = nullptr;
        pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if(pixels == nullptr) {
            return fatal<ID>(std::format("Failed to find/load texture file at path \"{}\"", texturePath));
        }
        const VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha; 
        GPUImage textureImage;

        //Initialize destination image
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

        //Initialize transfer buffer
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
            return tb.moveError<ID>();
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

        if(!(flags & rendererFlags::beganTransferOps)) {
            auto res = beginTransferOps();
            if(!res) {
                textureImage.destroy(device);
                transferBuffer.destroy(device);
                return res.moveError<ID>();
            }
        }
        transferBuffers.push(transferBuffer);

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
        vkCmdPipelineBarrier2(transferCommandBuffers[currentFrame], &dep1);

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
        vkCmdCopyBufferToImage2(transferCommandBuffers[currentFrame], &bufferToImageInfo);

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
        vkCmdPipelineBarrier2(transferCommandBuffers[currentFrame], &dep2);

        Texture t;
        t.handle = textureImage.handle;
        t.memory = textureImage.memory;
        t.view = textureImage.view;
        return entityManager.insertTexture(t);
    }
}