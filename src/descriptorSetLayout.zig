const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    const descriptorSetLayoutBindings = [_]c.VkDescriptorSetLayoutBinding{
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
        .bindingCount = descriptorSetLayoutBindings.len,
        .pBindings = &descriptorSetLayoutBindings
    };

    if(c.vkCreateDescriptorSetLayout(state.device, &descriptorSetLayoutInfo, allocator, 
        &state.descriptorSetLayout) != c.VK_SUCCESS
    ) {
        return error.failedToCreateDescriptorSetLayout;
    }

    const descriptorPoolSizes = [_]c.VkDescriptorPoolSize{
        .{
            .type = c.VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = Utils.MAX_OBJECTS * Utils.MAX_FRAMES_IN_FLIGHT
        }
    };

    const descriptorPoolInfo = c.VkDescriptorPoolCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = Utils.MAX_OBJECTS * Utils.MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = descriptorPoolSizes.len,
        .pPoolSizes = &descriptorPoolSizes
    };

    if(c.vkCreateDescriptorPool(state.device, &descriptorPoolInfo, allocator, 
        &state.descriptorPool) != c.VK_SUCCESS
    ) {
        return error.failedToCreateDescriptorPool;
    }
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroyDescriptorPool(state.device, state.descriptorPool, allocator);
    c.vkDestroyDescriptorSetLayout(state.device, state.descriptorSetLayout, allocator);
}