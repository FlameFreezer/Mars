const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    const descriptorSetLayoutBindings = comptime [_]c.VkDescriptorSetLayoutBinding{
        .{
            .binding = 0,
            .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = c.VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = null
        }
    };
    const descriptorSetLayoutInfo = c.VkDescriptorSetLayoutCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = comptime descriptorSetLayoutBindings.len,
        .pBindings = &descriptorSetLayoutBindings
    };

    if(c.vkCreateDescriptorSetLayout(state.device, &descriptorSetLayoutInfo, allocator, 
        &state.descriptorSetLayout) != c.VK_SUCCESS
    ) {
        return error.failedToCreateDescriptorSetLayout;
    }

    const descriptorPoolSizes = comptime [_]c.VkDescriptorPoolSize{
        .{
            .type = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = Utils.MAX_FRAMES_IN_FLIGHT
        }
    };

    const descriptorPoolInfo = c.VkDescriptorPoolCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = Utils.MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = descriptorPoolSizes.len,
        .pPoolSizes = &descriptorPoolSizes
    };

    if(c.vkCreateDescriptorPool(state.device, &descriptorPoolInfo, allocator, 
        &state.descriptorPool) != c.VK_SUCCESS
    ) {
        return error.failedToCreateDescriptorPool;
    }

    var layouts: [Utils.MAX_FRAMES_IN_FLIGHT]c.VkDescriptorSetLayout = undefined;
    @memset(layouts[0..], state.descriptorSetLayout);

    const allocInfo = c.VkDescriptorSetAllocateInfo{
        .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state.descriptorPool,
        .descriptorSetCount = state.descriptorSets.len,
        .pSetLayouts = layouts[0..]
    };
    if(c.vkAllocateDescriptorSets(state.device, &allocInfo, &state.descriptorSets) != c.VK_SUCCESS) {
        return error.failedToAllocateDescriptorSets;
    }

    for(0..Utils.MAX_FRAMES_IN_FLIGHT) |i| {
        const writeDescriptorSet = c.VkWriteDescriptorSet{
            .sType = c.VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state.descriptorSets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &.{
                .buffer = state.uniformBuffer.handle,
                .offset = @sizeOf(Utils.UniformBufferObject) * i,
                .range = @sizeOf(Utils.UniformBufferObject)
            }
        };
        c.vkUpdateDescriptorSets(state.device, 1, &writeDescriptorSet, 0, null);
    }
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroyDescriptorPool(state.device, state.descriptorPool, allocator);
    c.vkDestroyDescriptorSetLayout(state.device, state.descriptorSetLayout, allocator);
}
