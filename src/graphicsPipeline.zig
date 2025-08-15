const c = @import("c");
const std = @import("std");
const Utils = @import("utils.zig");

const shaderModule = struct {
    module: c.VkShaderModule,
    code: []const u8
};

pub fn init(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) !void {
    //These dynamic states will be configured during the render pass
    const dynamicStates = [_]c.VkDynamicState{
        c.VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT, 
        c.VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT
    };
    const dynamicState = c.VkPipelineDynamicStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStates.len,
        .pDynamicStates = dynamicStates[0..]
    };

    const shaderMod: shaderModule = try createShaderModule(state.device, allocator);
    defer std.heap.page_allocator.free(shaderMod.code);
    defer c.vkDestroyShaderModule(state.device, shaderMod.module, null);

    const shaderStageInfos = [_]c.VkPipelineShaderStageCreateInfo{
        .{
            .sType = c.VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = c.VK_SHADER_STAGE_VERTEX_BIT,
            .module = shaderMod.module,
            .pName = "vertMain"
        },
        .{
            .sType = c.VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = c.VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shaderMod.module,
            .pName = "fragMain"
        }
    };

    const vertexInputState = c.VkPipelineVertexInputStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &Utils.Vertex.inputBindingDescription(),
        .vertexAttributeDescriptionCount = @intCast(@typeInfo(Utils.Vertex).@"struct".fields.len),
        .pVertexAttributeDescriptions = &Utils.Vertex.inputAttributeDescriptions()
    };

    const inputAssemblyState = c.VkPipelineInputAssemblyStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = c.VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = c.VK_FALSE
    };

    const rasterizationState = c.VkPipelineRasterizationStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .rasterizerDiscardEnable = c.VK_FALSE,
        .polygonMode = c.VK_POLYGON_MODE_FILL,
        .depthClampEnable = c.VK_FALSE,
        .cullMode = c.VK_CULL_MODE_NONE,
        .frontFace = c.VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0
    };

    const multisampleState = c.VkPipelineMultisampleStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = c.VK_SAMPLE_COUNT_1_BIT,
        .alphaToOneEnable = c.VK_FALSE,
        .alphaToCoverageEnable = c.VK_FALSE,
        .sampleShadingEnable = c.VK_FALSE
    };

    const depthStencilState = c.VkPipelineDepthStencilStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = c.VK_TRUE,
        .depthWriteEnable = c.VK_TRUE,
        .depthCompareOp = c.VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = c.VK_FALSE,
        .stencilTestEnable = c.VK_FALSE
    };

    const colorBlendState = c.VkPipelineColorBlendStateCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = c.VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &c.VkPipelineColorBlendAttachmentState{
            .blendEnable = c.VK_FALSE,
            .colorWriteMask = c.VK_COLOR_COMPONENT_R_BIT
                | c.VK_COLOR_COMPONENT_G_BIT
                | c.VK_COLOR_COMPONENT_B_BIT
                | c.VK_COLOR_COMPONENT_A_BIT
        },
        .blendConstants = .{0.0} ** 4
    };

    const colorAttachmentFormats = [_]c.VkFormat{c.VK_FORMAT_B8G8R8A8_SRGB};
    const pipelineRenderingInfo = c.VkPipelineRenderingCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = colorAttachmentFormats.len,
        .pColorAttachmentFormats = &colorAttachmentFormats,
        .depthAttachmentFormat = c.VK_FORMAT_D32_SFLOAT
    };

    const pipelineLayoutInfo = c.VkPipelineLayoutCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state.descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &c.VkPushConstantRange{
            .stageFlags = c.VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = @sizeOf(Utils.CameraPushConstant)
        }
    };

    if(c.vkCreatePipelineLayout(state.device, &pipelineLayoutInfo, allocator, &state.graphicsPipelineLayout) != c.VK_SUCCESS) {
        return error.failedToCreatePipelineLayout;
    }

    const graphicsPipelineInfo = c.VkGraphicsPipelineCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = shaderStageInfos.len,
        .pStages = &shaderStageInfos,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = null, //ignored for now
        .pViewportState = null, //dynamic
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .pNext = &pipelineRenderingInfo,
        .layout = state.graphicsPipelineLayout
    };

    if(c.vkCreateGraphicsPipelines(state.device, null, 1, &graphicsPipelineInfo, allocator, &state.graphicsPipeline) != c.VK_SUCCESS) {
        return error.failedToCreateGraphicsPipeline;
    }
}

pub fn destroy(state: *Utils.State, allocator: ?*c.VkAllocationCallbacks) void {
    c.vkDestroyPipeline(state.device, state.graphicsPipeline, allocator);
    c.vkDestroyPipelineLayout(state.device, state.graphicsPipelineLayout, allocator);
}

fn createShaderModule(device: c.VkDevice, allocator: ?*c.VkAllocationCallbacks) !shaderModule {
    const shaderFile = try std.fs.cwd().openFile("shaders/slang.spv", .{.mode = .read_only});
    var shaderMod: shaderModule = undefined;
    shaderMod.code = try shaderFile.readToEndAlloc(std.heap.page_allocator, std.math.maxInt(usize));
    shaderFile.close();
    const shaderModuleInfo =  c.VkShaderModuleCreateInfo{
        .sType = c.VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderMod.code.len,
        .pCode = @alignCast(@ptrCast(shaderMod.code.ptr))
    };
    if(c.vkCreateShaderModule(device, &shaderModuleInfo, allocator, &shaderMod.module) != c.VK_SUCCESS) {
        return error.failed_to_create_shader_module;
    }
    return shaderMod;
}
