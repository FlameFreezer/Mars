const c = @import("c");
const std = @import("std");
const Utils = @import("Utils");

const Data = @import("data.zig");

pub fn drawFrame(state: *Utils.State) !void {
    const fenceWaitResult = c.vkWaitForFences(state.device, 1, &state.fences[state.currentFrame], c.VK_TRUE, std.math.maxInt(u64));
    if(fenceWaitResult != c.VK_SUCCESS and fenceWaitResult != c.VK_TIMEOUT) {
        return error.failedWaitingOnFence;
    }
    if(c.vkResetFences(state.device, 1, &state.fences[state.currentFrame]) != c.VK_SUCCESS) {
        return error.failedToResetFence;
    }

    try Utils.updateUniformBufferObject(&state.uniformBufferMapped[state.currentFrame], state.startTime.*);

    //The image acquisition semaphores are stored at the front of this array. Each frame has one
    //  semaphore for this purpose
    const imageAcquiredSemaphores = state.semaphores[0..Utils.MAX_FRAMES_IN_FLIGHT];
    //The presentation semaphores are stored at the back of this array. Each swapchain image has
    //  one semaphore for this purpose
    const presentReadySemaphores = state.semaphores[Utils.MAX_FRAMES_IN_FLIGHT..];

    var imageViewIndex: u32 = undefined;
    if(c.vkAcquireNextImageKHR(state.device, state.swapchain, std.math.maxInt(u64), 
        imageAcquiredSemaphores[state.currentFrame], null, &imageViewIndex) != c.VK_SUCCESS) 
    {
        return error.failedToAcquireSwapchainImage;
    }

    if(c.vkResetCommandBuffer(state.commandBuffers[state.currentFrame], 0) != c.VK_SUCCESS) {
        return error.failedToResetCommandBuffer;
    }
    const cmdBeginInfo = c.VkCommandBufferBeginInfo{
        .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = c.VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    if(c.vkBeginCommandBuffer(state.commandBuffers[state.currentFrame], &cmdBeginInfo) != c.VK_SUCCESS) {
        return error.failedToBeginCommandBuffer;
    }
    
    const colorWriteDependency = c.VkDependencyInfo{
        .sType = c.VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = 0,
        .memoryBarrierCount = 0,
        .bufferMemoryBarrierCount = 0,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &c.VkImageMemoryBarrier2{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcAccessMask = 0,
            .dstAccessMask = c.VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .srcStageMask = c.VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            .dstStageMask = c.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .oldLayout = c.VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = c.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .image = state.swapchainImages[imageViewIndex],
            .subresourceRange = .{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1
            }
        }
    };
    c.vkCmdPipelineBarrier2(state.commandBuffers[state.currentFrame], &colorWriteDependency);

    doRenderPass(state, imageViewIndex);

    const presentDependencyInfo = c.VkDependencyInfo{
        .sType = c.VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .dependencyFlags = c.VK_DEPENDENCY_BY_REGION_BIT,
        .memoryBarrierCount = 0,
        .bufferMemoryBarrierCount = 0,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &c.VkImageMemoryBarrier2{
            .sType = c.VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcAccessMask = c.VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .srcStageMask = c.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = 0,
            .dstStageMask = c.VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
            .oldLayout = c.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = c.VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = state.swapchainImages[imageViewIndex],
            .subresourceRange = .{
                .aspectMask = c.VK_IMAGE_ASPECT_COLOR_BIT,
                .layerCount = 1,
                .levelCount = 1
            }
        }
    };
    c.vkCmdPipelineBarrier2(state.commandBuffers[state.currentFrame], &presentDependencyInfo);

    if(c.vkEndCommandBuffer(state.commandBuffers[state.currentFrame]) != c.VK_SUCCESS) {
        return error.failedToEndCommandBuffer;
    }
    const submitInfo = c.VkSubmitInfo2{
        .sType = c.VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &c.VkSemaphoreSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = imageAcquiredSemaphores[state.currentFrame],
            .stageMask = c.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        },
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &c.VkSemaphoreSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = presentReadySemaphores[imageViewIndex],
            .stageMask = c.VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        },
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &c.VkCommandBufferSubmitInfo{
            .sType = c.VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = state.commandBuffers[state.currentFrame]
        }
    };
    if(c.vkQueueSubmit2(state.queues.graphics, 1, &submitInfo, state.fences[state.currentFrame]) != c.VK_SUCCESS) {
        return error.failedToSubmitToQueue;
    }
    const presentInfo = c.VkPresentInfoKHR{
        .sType = c.VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &presentReadySemaphores[imageViewIndex],
        .swapchainCount = 1,
        .pSwapchains = &state.swapchain,
        .pImageIndices = &imageViewIndex,
        .pResults = null
    };
    if(c.vkQueuePresentKHR(state.queues.present, &presentInfo) != c.VK_SUCCESS) {
        return error.failedToPresent;
    }

    state.currentFrame = (state.currentFrame + 1) % Utils.MAX_FRAMES_IN_FLIGHT;
}

fn doRenderPass(state: *Utils.State, imageViewIndex: u32) void {
    const renderingInfo = c.VkRenderingInfo{
        .sType = c.VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = c.VkRect2D{
            .offset = .{.x = 0.0, .y = 0.0},
            .extent = state.swapchainExtent
        },
        .layerCount = 1,
        .viewMask = 0,
        .pDepthAttachment = null,
        .pStencilAttachment = null,
        .colorAttachmentCount = 1,
        .pColorAttachments = &c.VkRenderingAttachmentInfo{
            .sType = c.VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = state.swapchainImageViews[imageViewIndex],
            .imageLayout = c.VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = c.VK_ATTACHMENT_LOAD_OP_CLEAR,
            .clearValue = c.VkClearValue{
                .color = c.VkClearColorValue{.float32 = .{1.0, 1.0, 1.0, 1.0}}
            },
            .storeOp = c.VK_ATTACHMENT_STORE_OP_STORE
        }
    };
    c.vkCmdBeginRendering(state.commandBuffers[state.currentFrame], &renderingInfo);

    c.vkCmdBindPipeline(state.commandBuffers[state.currentFrame], c.VK_PIPELINE_BIND_POINT_GRAPHICS, 
        state.graphicsPipeline);

    const viewport = c.VkViewport{
        .x = 0.0,
        .y = 0.0,
        .width = @floatFromInt(state.swapchainExtent.width),
        .height = @floatFromInt(state.swapchainExtent.height),
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };
    c.vkCmdSetViewportWithCount(state.commandBuffers[state.currentFrame], 1, &viewport);

    const scissor = c.VkRect2D{
        .offset = c.VkOffset2D{.x = 0, .y = 0},
        .extent = state.swapchainExtent
    };
    c.vkCmdSetScissorWithCount(state.commandBuffers[state.currentFrame], 1, &scissor);

    c.vkCmdBindVertexBuffers(state.commandBuffers[state.currentFrame], 0, 1, &state.buffer.handle, 
        &([1]u64{0})[0]);
    c.vkCmdBindIndexBuffer(state.commandBuffers[state.currentFrame], state.buffer.handle, 
        @sizeOf(@TypeOf(Data.Vertices)), c.VK_INDEX_TYPE_UINT32);
    c.vkCmdBindDescriptorSets(state.commandBuffers[state.currentFrame], 
        c.VK_PIPELINE_BIND_POINT_GRAPHICS, state.graphicsPipelineLayout, 0, 1, 
        &state.descriptorSets[state.currentFrame], 0, 0);
    c.vkCmdDrawIndexed(state.commandBuffers[state.currentFrame], Data.Indices.len, 1, 0, 0, 0);
    c.vkCmdEndRendering(state.commandBuffers[state.currentFrame]);
}
